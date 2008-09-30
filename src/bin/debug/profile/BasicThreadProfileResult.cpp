/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "BasicThreadProfileResult.h"

#include <stdio.h>

#include <algorithm>
#include <new>

#include "Options.h"


struct HitSymbol {
	int64	hits;
	Symbol*	symbol;

	inline bool operator<(const HitSymbol& other) const
	{
		return hits > other.hits;
	}
};


// #pragma mark - BasicThreadImage


BasicThreadImage::BasicThreadImage(Image* image)
	:
	ThreadImage(image),
	fSymbolHits(NULL),
	fUnknownHits(0)
{
}


BasicThreadImage::~BasicThreadImage()
{
}


status_t
BasicThreadImage::Init()
{
	int32 symbolCount = fImage->SymbolCount();
	fSymbolHits = new(std::nothrow) int64[symbolCount];
	if (fSymbolHits == NULL)
		return B_NO_MEMORY;

	memset(fSymbolHits, 0, 8 * symbolCount);

	return B_OK;
}


void
BasicThreadImage::AddHit(addr_t address)
{
	int32 symbolIndex = fImage->FindSymbol(address);
	if (symbolIndex >= 0)
		fSymbolHits[symbolIndex]++;
	else
		fUnknownHits++;

	fTotalHits++;
}


void
BasicThreadImage::AddSymbolHit(int32 symbolIndex)
{
	fSymbolHits[symbolIndex]++;
}


void
BasicThreadImage::AddImageHit()
{
	fTotalHits++;
}


const int64*
BasicThreadImage::SymbolHits() const
{
	return fSymbolHits;
}


int64
BasicThreadImage::UnknownHits() const
{
	return fUnknownHits;
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
	// get hit images
	BasicThreadImage* images[fOldImages.Size() + fImages.Size()];
	int32 imageCount = GetHitImages(images);

	// count symbols
	int32 symbolCount = 0;
	for (int32 k = 0; k < imageCount; k++) {
		BasicThreadImage* image = images[k];
		if (image->TotalHits() > image->UnknownHits())
			symbolCount += image->GetImage()->SymbolCount();
	}

	// find and sort the hit symbols
	HitSymbol hitSymbols[symbolCount];
	int32 hitSymbolCount = 0;

	for (int32 k = 0; k < imageCount; k++) {
		BasicThreadImage* image = images[k];
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
			BasicThreadImage* image = images[k];
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


BasicThreadImage*
BasicThreadProfileResult::CreateThreadImage(Image* image)
{
	return new(std::nothrow) BasicThreadImage(image);
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
	BasicThreadImage* previousImage = NULL;
	int32 previousSymbol = -1;

	for (int32 i = 0; i < sampleCount; i++) {
		addr_t address = samples[i];
		BasicThreadImage* image = FindImage(address);
		int32 symbol = -1;
		if (image != NULL) {
			symbol = image->GetImage()->FindSymbol(address);
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


// #pragma mark - ExclusiveThreadProfileResult


void
ExclusiveThreadProfileResult::AddSamples(addr_t* samples, int32 sampleCount)
{
	BasicThreadImage* image = NULL;
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
