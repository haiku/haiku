/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Thread.h"

#include <algorithm>
#include <new>

#include <debug_support.h>

#include "debug_utils.h"

#include "Options.h"
#include "Team.h"


struct HitSymbol {
	int64	hits;
	Symbol*	symbol;

	inline bool operator<(const HitSymbol& other) const
	{
		return hits > other.hits;
	}
};


class ThreadImage : public DoublyLinkedListLinkImpl<ThreadImage> {
public:
	ThreadImage(Image* image)
		:
		fImage(image),
		fSymbolHits(NULL),
		fTotalHits(0),
		fUnknownHits(0)
	{
		fImage->AddReference();
	}

	virtual ~ThreadImage()
	{
		fImage->RemoveReference();
	}

	status_t Init()
	{
		int32 symbolCount = fImage->SymbolCount();
		fSymbolHits = new(std::nothrow) int64[symbolCount];
		if (fSymbolHits == NULL)
			return B_NO_MEMORY;

		memset(fSymbolHits, 0, 8 * symbolCount);

		return B_OK;
	}

	image_id ID() const
	{
		return fImage->ID();
	}

	bool ContainsAddress(addr_t address) const
	{
		return fImage->ContainsAddress(address);
	}

	void AddHit(addr_t address)
	{
		int32 symbolIndex = fImage->FindSymbol(address);
		if (symbolIndex >= 0)
			fSymbolHits[symbolIndex]++;
		else
			fUnknownHits++;

		fTotalHits++;
	}

	void AddSymbolHit(int32 symbolIndex)
	{
		fSymbolHits[symbolIndex]++;
	}

	void AddImageHit()
	{
		fTotalHits++;
	}

	Image* GetImage() const
	{
		return fImage;
	}

	const int64* SymbolHits() const
	{
		return fSymbolHits;
	}

	int64 TotalHits() const
	{
		return fTotalHits;
	}

	int64 UnknownHits() const
	{
		return fUnknownHits;
	}

private:
	Image*	fImage;
	int64*	fSymbolHits;
	int64	fTotalHits;
	int64	fUnknownHits;
};


// #pragma mark -


Thread::Thread(const thread_info& info, Team* team)
	:
	fInfo(info),
	fTeam(team),
	fSampleArea(-1),
	fSamples(NULL),
	fProfileResult(NULL)
{
}


Thread::~Thread()
{
	if (fSampleArea >= 0)
		delete_area(fSampleArea);

	delete fProfileResult;
}


void
Thread::SetProfileResult(ThreadProfileResult* result)
{
	delete fProfileResult;
	fProfileResult = result;
}


void
Thread::UpdateInfo()
{
	thread_info info;
	if (get_thread_info(ID(), &info) == B_OK)
		fInfo = info;
}


void
Thread::SetSampleArea(area_id area, addr_t* samples)
{
	fSampleArea = area;
	fSamples = samples;
}


void
Thread::SetInterval(bigtime_t interval)
{
	fProfileResult->SetInterval(interval);
}


status_t
Thread::AddImage(Image* image)
{
	return fProfileResult->AddImage(image);
}


void
Thread::AddSamples(int32 count, int32 dropped, int32 stackDepth,
	bool variableStackDepth, int32 event)
{
	fProfileResult->SynchronizeImages(event);

	if (variableStackDepth) {
		addr_t* samples = fSamples;

		while (count > 0) {
			addr_t sampleCount = *(samples++);

			if (sampleCount >= B_DEBUG_PROFILE_EVENT_BASE) {
				int32 eventParameterCount
					= sampleCount & B_DEBUG_PROFILE_EVENT_PARAMETER_MASK;
				if (sampleCount == B_DEBUG_PROFILE_IMAGE_EVENT) {
					fProfileResult->SynchronizeImages((int32)samples[0]);
				} else {
					fprintf(stderr, "unknown profile event: %#lx\n",
						sampleCount);
				}

				samples += eventParameterCount;
				count -= eventParameterCount + 1;
				continue;
			}

			fProfileResult->AddSamples(samples, sampleCount);

			samples += sampleCount;
			count -= sampleCount + 1;
		}
	} else {
		count = count / stackDepth * stackDepth;

		for (int32 i = 0; i < count; i += stackDepth)
			fProfileResult->AddSamples(fSamples + i, stackDepth);
	}

	fProfileResult->AddDroppedTicks(dropped);
}


void
Thread::PrintResults() const
{
	fProfileResult->PrintResults();
}


// #pragma mark - ThreadProfileResult


ThreadProfileResult::ThreadProfileResult()
	:
	fThread(NULL),
	fInterval(1)
{
}


ThreadProfileResult::~ThreadProfileResult()
{
}


status_t
ThreadProfileResult::Init(Thread* thread)
{
	fThread = thread;
	return B_OK;
}


void
ThreadProfileResult::SetInterval(bigtime_t interval)
{
	fInterval = interval;
}


// #pragma mark - AbstractThreadProfileResult


AbstractThreadProfileResult::AbstractThreadProfileResult()
	:
	fImages(),
	fNewImages(),
	fOldImages()
{
}


AbstractThreadProfileResult::~AbstractThreadProfileResult()
{
	while (ThreadImage* image = fImages.RemoveHead())
		delete image;
	while (ThreadImage* image = fOldImages.RemoveHead())
		delete image;
}


status_t
AbstractThreadProfileResult::AddImage(Image* image)
{
	ThreadImage* threadImage = CreateThreadImage(image);
	if (threadImage == NULL)
		return B_NO_MEMORY;

	status_t error = threadImage->Init();
	if (error != B_OK) {
		delete threadImage;
		return error;
	}

	fNewImages.Add(threadImage);

	return B_OK;
}


void
AbstractThreadProfileResult::SynchronizeImages(int32 event)
{
	// remove obsolete images
	ImageList::Iterator it = fImages.GetIterator();
	while (ThreadImage* image = it.Next()) {
		int32 deleted = image->GetImage()->DeletionEvent();
		if (deleted >= 0 && event >= deleted) {
			it.Remove();
			if (image->TotalHits() > 0)
				fOldImages.Add(image);
			else
				delete image;
		}
	}

	// add new images
	it = fNewImages.GetIterator();
	while (ThreadImage* image = it.Next()) {
		if (image->GetImage()->CreationEvent() >= event) {
			it.Remove();
			fImages.Add(image);
		}
	}
}


ThreadImage*
AbstractThreadProfileResult::FindImage(addr_t address) const
{
	ImageList::ConstIterator it = fImages.GetIterator();
	while (ThreadImage* image = it.Next()) {
		if (image->ContainsAddress(address))
			return image;
	}
	return NULL;
}


// #pragma mark - BasicThreadProfileResult


BasicThreadProfileResult::BasicThreadProfileResult()
	:
	fTotalTicks(0),
	fUnkownTicks(0),
	fDroppedTicks(0),
	fTotalSampleCount(0)
{
}


void
BasicThreadProfileResult::AddDroppedTicks(int32 dropped)
{
	fDroppedTicks += dropped;
}


void
BasicThreadProfileResult::PrintResults()
{
	// count images and symbols
	int32 imageCount = 0;
	int32 symbolCount = 0;

	ImageList::Iterator it = fOldImages.GetIterator();
	while (ThreadImage* image = it.Next()) {
		if (image->TotalHits() > 0) {
			imageCount++;
			if (image->TotalHits() > image->UnknownHits())
				symbolCount += image->GetImage()->SymbolCount();
		}
	}

	it = fImages.GetIterator();
	while (ThreadImage* image = it.Next()) {
		if (image->TotalHits() > 0) {
			imageCount++;
			if (image->TotalHits() > image->UnknownHits())
				symbolCount += image->GetImage()->SymbolCount();
		}
	}

	ThreadImage* images[imageCount];
	imageCount = 0;

	it = fOldImages.GetIterator();
	while (ThreadImage* image = it.Next()) {
		if (image->TotalHits() > 0)
			images[imageCount++] = image;
	}

	it = fImages.GetIterator();
	while (ThreadImage* image = it.Next()) {
		if (image->TotalHits() > 0)
			images[imageCount++] = image;
	}

	// find and sort the hit symbols
	HitSymbol hitSymbols[symbolCount];
	int32 hitSymbolCount = 0;

	for (int32 k = 0; k < imageCount; k++) {
		ThreadImage* image = images[k];
		if (image->TotalHits() > image->UnknownHits()) {
			Symbol** symbols = image->GetImage()->Symbols();
			const int64* symbolHits = image->SymbolHits();
			int32 imageSymbolCount = image->GetImage()->SymbolCount();
			for (int32 i = 0; i < imageSymbolCount; i++) {
				if (symbolHits[i] > 0) {
					HitSymbol& hitSymbol = hitSymbols[hitSymbolCount++];
					hitSymbol.hits = symbolHits[i];
					hitSymbol.symbol = symbols[i];
				}
			}
		}
	}

	if (hitSymbolCount > 1)
		std::sort(hitSymbols, hitSymbols + hitSymbolCount);

	int64 totalTicks = fTotalTicks;
	fprintf(gOptions.output, "\nprofiling results for thread \"%s\" "
		"(%ld):\n", fThread->Name(), fThread->ID());
	fprintf(gOptions.output, "  tick interval:  %lld us\n", fInterval);
	fprintf(gOptions.output, "  total ticks:    %lld (%lld us)\n",
		totalTicks, totalTicks * fInterval);
	if (totalTicks == 0)
		totalTicks = 1;
	fprintf(gOptions.output, "  unknown ticks:  %lld (%lld us, %6.2f%%)\n",
		fUnkownTicks, fUnkownTicks * fInterval,
		100.0 * fUnkownTicks / totalTicks);
	fprintf(gOptions.output, "  dropped ticks:  %lld (%lld us, %6.2f%%)\n",
		fDroppedTicks, fDroppedTicks * fInterval,
		100.0 * fDroppedTicks / totalTicks);
	if (gOptions.analyze_full_stack) {
		fprintf(gOptions.output, "  samples/tick:   %.1f\n",
			(double)fTotalSampleCount / totalTicks);
	}

	if (imageCount > 0) {
		fprintf(gOptions.output, "\n");
		fprintf(gOptions.output, "        hits     unknown    image\n");
		fprintf(gOptions.output, "  ---------------------------------------"
			"---------------------------------------\n");
		for (int32 k = 0; k < imageCount; k++) {
			ThreadImage* image = images[k];
			const image_info& imageInfo = image->GetImage()->Info();
			fprintf(gOptions.output, "  %10lld  %10lld  %7ld %s\n",
				image->TotalHits(), image->UnknownHits(), imageInfo.id,
				imageInfo.name);
		}
	}

	if (hitSymbolCount > 0) {
		fprintf(gOptions.output, "\n");
		fprintf(gOptions.output, "        hits       in us    in %%   "
			"image  function\n");
		fprintf(gOptions.output, "  ---------------------------------------"
			"---------------------------------------\n");
		for (int32 i = 0; i < hitSymbolCount; i++) {
			const HitSymbol& hitSymbol = hitSymbols[i];
			const Symbol* symbol = hitSymbol.symbol;
			fprintf(gOptions.output, "  %10lld  %10lld  %6.2f  %6ld  %s\n",
				hitSymbol.hits, hitSymbol.hits * fInterval,
				100.0 * hitSymbol.hits / totalTicks, symbol->image->ID(),
				symbol->Name());
		}
	} else
		fprintf(gOptions.output, "  no functions were hit\n");
}


ThreadImage*
BasicThreadProfileResult::CreateThreadImage(Image* image)
{
	return new(std::nothrow) ThreadImage(image);
}


// #pragma mark - InclusiveThreadProfileResult


void
InclusiveThreadProfileResult::AddSamples(addr_t* samples, int32 sampleCount)
{
	// Sort the samples. This way hits of the same symbol are
	// successive and we can avoid incrementing the hit count of the
	// same symbol twice. Same for images.
	std::sort(samples, samples + sampleCount);

	int32 unknownSamples = 0;
	ThreadImage* previousImage = NULL;
	int32 previousSymbol = -1;

	for (int32 i = 0; i < sampleCount; i++) {
		addr_t address = samples[i];
		ThreadImage* image = FindImage(address);
		int32 symbol = -1;
		if (image != NULL) {
			symbol = image->GetImage()->FindSymbol(address);
			if (symbol < 0) {
				// TODO: Count unknown image hits?
			} else if (symbol != previousSymbol)
				image->AddSymbolHit(symbol);

			if (image != previousImage)
				image->AddImageHit();
		} else
			unknownSamples++;

		previousImage = image;
		previousSymbol = symbol;
	}

	if (unknownSamples == sampleCount)
		fUnkownTicks++;

	fTotalTicks++;
	fTotalSampleCount += sampleCount;
}


// #pragma mark - ExclusiveThreadProfileResult


void
ExclusiveThreadProfileResult::AddSamples(addr_t* samples, int32 sampleCount)
{
	ThreadImage* image = NULL;
	for (int32 k = 0; k < sampleCount; k++) {
		addr_t address = samples[k];
		image = FindImage(address);
		if (image != NULL) {
			image->AddHit(address);
			break;
		}
	}

	if (image == NULL)
		fUnkownTicks++;

	fTotalTicks++;
	fTotalSampleCount += sampleCount;
}
