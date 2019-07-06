/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "BasicProfileResult.h"

#if __GNUC__ > 2
#include <cxxabi.h>
#endif
#include <stdio.h>

#include <algorithm>
#include <new>

#include "Options.h"
#include "ProfiledEntity.h"


struct HitSymbol {
	int64		hits;
	Symbol*		symbol;
	image_id	imageID;

	inline bool operator<(const HitSymbol& other) const
	{
		return hits > other.hits;
	}
};


// #pragma mark - BasicImageProfileResult


BasicImageProfileResult::BasicImageProfileResult(SharedImage* image,
	image_id id)
	:
	ImageProfileResult(image, id),
	fSymbolHits(NULL),
	fUnknownHits(0)
{
}


BasicImageProfileResult::~BasicImageProfileResult()
{
}


status_t
BasicImageProfileResult::Init()
{
	int32 symbolCount = fImage->SymbolCount();
	fSymbolHits = new(std::nothrow) int64[symbolCount];
	if (fSymbolHits == NULL)
		return B_NO_MEMORY;

	memset(fSymbolHits, 0, 8 * symbolCount);

	return B_OK;
}


bool
BasicImageProfileResult::AddHit(addr_t address)
{
	int32 symbolIndex = fImage->FindSymbol(address);
	if (symbolIndex < 0)
		return false;

	fSymbolHits[symbolIndex]++;
	fTotalHits++;

	return true;
}


void
BasicImageProfileResult::AddUnknownHit()
{
	fUnknownHits++;
	fTotalHits++;
}


void
BasicImageProfileResult::AddSymbolHit(int32 symbolIndex)
{
	fSymbolHits[symbolIndex]++;
}


void
BasicImageProfileResult::AddImageHit()
{
	fTotalHits++;
}


const int64*
BasicImageProfileResult::SymbolHits() const
{
	return fSymbolHits;
}


int64
BasicImageProfileResult::UnknownHits() const
{
	return fUnknownHits;
}


// #pragma mark - BasicProfileResult


BasicProfileResult::BasicProfileResult()
	:
	fTotalTicks(0),
	fUnkownTicks(0),
	fDroppedTicks(0),
	fTotalSampleCount(0)
{
}


void
BasicProfileResult::AddDroppedTicks(int32 dropped)
{
	fDroppedTicks += dropped;
}


void
BasicProfileResult::PrintResults(ImageProfileResultContainer* container)
{
	// get hit images
	BasicImageProfileResult* images[container->CountImages()];
	int32 imageCount = GetHitImages(container, images);

	// count symbols
	int32 symbolCount = 0;
	for (int32 k = 0; k < imageCount; k++) {
		BasicImageProfileResult* image = images[k];
		if (image->TotalHits() > image->UnknownHits())
			symbolCount += image->GetImage()->SymbolCount();
	}

	// find and sort the hit symbols
	HitSymbol hitSymbols[symbolCount];
	int32 hitSymbolCount = 0;

	for (int32 k = 0; k < imageCount; k++) {
		BasicImageProfileResult* image = images[k];
		if (image->TotalHits() > image->UnknownHits()) {
			Symbol** symbols = image->GetImage()->Symbols();
			const int64* symbolHits = image->SymbolHits();
			int32 imageSymbolCount = image->GetImage()->SymbolCount();
			for (int32 i = 0; i < imageSymbolCount; i++) {
				if (symbolHits[i] > 0) {
					HitSymbol& hitSymbol = hitSymbols[hitSymbolCount++];
					hitSymbol.hits = symbolHits[i];
					hitSymbol.symbol = symbols[i];
					hitSymbol.imageID = image->ID();
				}
			}
		}
	}

	if (hitSymbolCount > 1)
		std::sort(hitSymbols, hitSymbols + hitSymbolCount);

	int64 totalTicks = fTotalTicks;
	fprintf(gOptions.output, "\nprofiling results for %s \"%s\" "
		"(%" B_PRId32 "):\n", fEntity->EntityType(), fEntity->EntityName(),
		fEntity->EntityID());
	fprintf(gOptions.output, "  tick interval:  %" B_PRIdBIGTIME " us\n",
		fInterval);
	fprintf(gOptions.output,
		"  total ticks:    %" B_PRId64 " (%" B_PRId64 " us)\n",
		totalTicks, totalTicks * fInterval);
	if (totalTicks == 0)
		totalTicks = 1;
	fprintf(gOptions.output,
		"  unknown ticks:  %" B_PRId64 " (%" B_PRId64 " us, %6.2f%%)\n",
		fUnkownTicks, fUnkownTicks * fInterval,
		100.0 * fUnkownTicks / totalTicks);
	fprintf(gOptions.output,
		"  dropped ticks:  %" B_PRId64 " (%" B_PRId64 " us, %6.2f%%)\n",
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
			BasicImageProfileResult* image = images[k];
			fprintf(gOptions.output,
				"  %10" B_PRId64 "  %10" B_PRId64 "  %7" B_PRId32 " %s\n",
				image->TotalHits(), image->UnknownHits(),
				image->ID(), image->GetImage()->Name());
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
#if __GNUC__ > 2
			int status;
			const char* symbolName = __cxxabiv1::__cxa_demangle(symbol->Name(),
				NULL, NULL, &status);
			if (symbolName == NULL)
				symbolName = symbol->Name();
#else
			const char* symbolName = symbol->Name();
#endif
			fprintf(gOptions.output,
				"  %10" B_PRId64 "  %10" B_PRId64 "  %6.2f  %6" B_PRId32
				"  %s\n", hitSymbol.hits, hitSymbol.hits * fInterval,
				100.0 * hitSymbol.hits / totalTicks, hitSymbol.imageID,
				symbolName);
#if __GNUC__ > 2
			if (status == 0)
				free(const_cast<char*>(symbolName));
#endif
		}
	} else
		fprintf(gOptions.output, "  no functions were hit\n");
}


status_t
BasicProfileResult::GetImageProfileResult(SharedImage* image, image_id id,
	ImageProfileResult*& _imageResult)
{
	BasicImageProfileResult* result
		= new(std::nothrow) BasicImageProfileResult(image, id);
	if (result == NULL)
		return B_NO_MEMORY;

	status_t error = result->Init();
	if (error != B_OK) {
		delete result;
		return error;
	}

	_imageResult = result;
	return B_OK;
}


// #pragma mark - InclusiveProfileResult


void
InclusiveProfileResult::AddSamples(ImageProfileResultContainer* container,
	addr_t* samples, int32 sampleCount)
{
	// Sort the samples. This way hits of the same symbol are
	// successive and we can avoid incrementing the hit count of the
	// same symbol twice. Same for images.
	std::sort(samples, samples + sampleCount);

	int32 unknownSamples = 0;
	BasicImageProfileResult* previousImage = NULL;
	int32 previousSymbol = -1;

	for (int32 i = 0; i < sampleCount; i++) {
		addr_t address = samples[i];
		addr_t loadDelta;
		BasicImageProfileResult* image = static_cast<BasicImageProfileResult*>(
			container->FindImage(address, loadDelta));
		int32 symbol = -1;
		if (image != NULL) {
			symbol = image->GetImage()->FindSymbol(address - loadDelta);
			if (symbol < 0) {
				// TODO: Count unknown image hits?
			} else if (image != previousImage || symbol != previousSymbol)
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


// #pragma mark - ExclusiveProfileResult


void
ExclusiveProfileResult::AddSamples(ImageProfileResultContainer* container,
	addr_t* samples, int32 sampleCount)
{
	BasicImageProfileResult* image = NULL;
		// the image in which we hit a symbol
	BasicImageProfileResult* firstImage = NULL;
		// the first image we hit, != image if no symbol was hit

	for (int32 k = 0; k < sampleCount; k++) {
		addr_t address = samples[k];
		addr_t loadDelta;
		image = static_cast<BasicImageProfileResult*>(
			container->FindImage(address, loadDelta));
		if (image != NULL) {
			if (image->AddHit(address - loadDelta))
				break;
			if (firstImage == NULL)
				firstImage = image;
		}
	}

	if (image == NULL) {
		if (firstImage != NULL)
			firstImage->AddUnknownHit();
		else
			fUnkownTicks++;
	}

	fTotalTicks++;
	fTotalSampleCount += sampleCount;
}
