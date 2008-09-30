/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "CallgrindThreadProfileResult.h"

#include <errno.h>
#include <sys/stat.h>

#include <algorithm>
#include <new>

#include "Options.h"


// #pragma mark - CallgrindThreadImage


CallgrindThreadImage::CallgrindThreadImage(Image* image)
	:
	ThreadImage(image),
	fFunctions(NULL),
	fOutputIndex(0)
{
}


CallgrindThreadImage::~CallgrindThreadImage()
{
	int32 symbolCount = fImage->SymbolCount();
	for (int32 i = 0; i < symbolCount; i++) {
		while (CallgrindCalledFunction* calledFunction
				= fFunctions[i].calledFunctions) {
			fFunctions[i].calledFunctions = calledFunction->next;
			delete calledFunction;
		}
	}

	delete[] fFunctions;
}


status_t
CallgrindThreadImage::Init()
{
	int32 symbolCount = fImage->SymbolCount();
	fFunctions = new(std::nothrow) CallgrindFunction[symbolCount];
	if (fFunctions == NULL)
		return B_NO_MEMORY;

	memset(fFunctions, 0, sizeof(CallgrindFunction) * symbolCount);

	return B_OK;
}


void
CallgrindThreadImage::AddSymbolHit(int32 symbolIndex,
	CallgrindThreadImage* calledImage, int32 calledSymbol)

{
	fTotalHits++;

	CallgrindFunction& function = fFunctions[symbolIndex];
	if (calledImage != NULL) {
		// check whether the called function is known already
		CallgrindCalledFunction* calledFunction = function.calledFunctions;
		while (calledFunction != NULL) {
			if (calledFunction->image == calledImage
				&& calledFunction->function == calledSymbol) {
				break;
			}
			calledFunction = calledFunction->next;
		}

		// create a new CallgrindCalledFunction object, if not known
		if (calledFunction == NULL) {
			calledFunction = new(std::nothrow) CallgrindCalledFunction(
				calledImage, calledSymbol);
			if (calledFunction == NULL)
				return;

			calledFunction->next = function.calledFunctions;
			function.calledFunctions = calledFunction;
		}

		calledFunction->hits++;
	} else
		function.hits++;
}


CallgrindFunction*
CallgrindThreadImage::Functions() const
{
	return fFunctions;
}


int32
CallgrindThreadImage::OutputIndex() const
{
	return fOutputIndex;
}


void
CallgrindThreadImage::SetOutputIndex(int32 index)
{
	fOutputIndex = index;
}


// #pragma mark - CallgrindThreadProfileResult


CallgrindThreadProfileResult::CallgrindThreadProfileResult()
	:
	fTotalTicks(0),
	fUnkownTicks(0),
	fDroppedTicks(0),
	fNextImageOutputIndex(1),
	fNextFunctionOutputIndex(1)
{
}


void
CallgrindThreadProfileResult::AddSamples(addr_t* samples, int32 sampleCount)
{
	int32 unknownSamples = 0;
	CallgrindThreadImage* previousImage = NULL;
	int32 previousSymbol = -1;

	// TODO: That probably doesn't work with recursive functions.
	for (int32 i = 0; i < sampleCount; i++) {
		addr_t address = samples[i];
		CallgrindThreadImage* image = FindImage(address);
		int32 symbol = -1;
		if (image != NULL) {
			symbol = image->GetImage()->FindSymbol(address);
			if (symbol >= 0) {
				image->AddSymbolHit(symbol, previousImage, previousSymbol);
				previousImage = image;
				previousSymbol = symbol;
			}
		} else
			unknownSamples++;
	}

	if (unknownSamples == sampleCount)
		fUnkownTicks++;

	fTotalTicks++;
}


void
CallgrindThreadProfileResult::AddDroppedTicks(int32 dropped)
{
	fDroppedTicks += dropped;
}


void
CallgrindThreadProfileResult::PrintResults()
{
	// create output file
	mkdir(gOptions.callgrind_directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	char fileName[B_PATH_NAME_LENGTH];
	snprintf(fileName, sizeof(fileName), "%s/callgrind.out.%ld",
		gOptions.callgrind_directory, fThread->ID());

	FILE* out = fopen(fileName, "w+");
	if (out == NULL) {
		fprintf(stderr, "%s: Failed to open output file \"%s\": %s\n",
			kCommandName, fileName, strerror(errno));
		return;
	}

	// write the header
	fprintf(out, "version: 1\n");
	fprintf(out, "creator: Haiku profile\n");
	fprintf(out, "pid: %ld\n", fThread->ID());
	fprintf(out, "cmd: %s\n", fThread->Name());
	fprintf(out, "part: 1\n\n");

	fprintf(out, "positions: line\n");
	fprintf(out, "events: Ticks Time\n");
	fprintf(out, "summary: %lld\n", fTotalTicks);

	// get hit images
	CallgrindThreadImage* images[fOldImages.Size() + fImages.Size()];
	int32 imageCount = GetHitImages(images);

	for (int32 i = 0; i < imageCount; i++) {
		CallgrindThreadImage* image = images[i];

		CallgrindFunction* functions = image->Functions();
		int32 imageSymbolCount = image->GetImage()->SymbolCount();
		for (int32 k = 0; k < imageSymbolCount; k++) {
			CallgrindFunction& function = functions[k];
			if (function.hits == 0 && function.calledFunctions == NULL)
				continue;

			fprintf(out, "\n");
			_PrintFunction(out, image, k, false);
			fprintf(out, "0 %lld %lld\n", function.hits,
				function.hits * fInterval);

			CallgrindCalledFunction* calledFunction = function.calledFunctions;
			while (calledFunction != NULL) {
				_PrintFunction(out, calledFunction->image,
					calledFunction->function, true);
				fprintf(out, "calls=%lld 0\n", calledFunction->hits);
				fprintf(out, "0 %lld %lld\n", calledFunction->hits,
					calledFunction->hits * fInterval);
				calledFunction = calledFunction->next;
			}
		}
	}

	// print pseudo-functions for unknown and dropped ticks
	if (fUnkownTicks + fDroppedTicks > 0) {
		fprintf(out, "\nob=<pseudo>\n");

		if (fUnkownTicks > 0) {
			fprintf(out, "\nfn=unknown\n");
			fprintf(out, "0 %lld\n", fUnkownTicks);
		}

		if (fDroppedTicks > 0) {
			fprintf(out, "\nfn=dropped\n");
			fprintf(out, "0 %lld\n", fDroppedTicks);
		}
	}

	fclose(out);
}


CallgrindThreadImage*
CallgrindThreadProfileResult::CreateThreadImage(Image* image)
{
	return new(std::nothrow) CallgrindThreadImage(image);
}


void
CallgrindThreadProfileResult::_PrintFunction(FILE* out,
	CallgrindThreadImage* image, int32 functionIndex, bool called)
{
	if (image->OutputIndex() == 0) {
		// need to print the image name
		int32 index = fNextImageOutputIndex++;
		image->SetOutputIndex(index);
		fprintf(out, "%sob=(%ld) %s:%ld\n", called ? "c" : "", index,
			image->GetImage()->Info().name, image->ID());
	} else {
		// image is already known
		// TODO: We may not need to print it at all!
		fprintf(out, "%sob=(%ld)\n", called ? "c" : "", image->OutputIndex());
	}

	CallgrindFunction& function = image->Functions()[functionIndex];
	if (function.outputIndex == 0) {
		// need to print the function name
		function.outputIndex = fNextFunctionOutputIndex++;
		fprintf(out, "%sfn=(%ld) %s\n", called ? "c" : "", function.outputIndex,
			image->GetImage()->Symbols()[functionIndex]->Name());
	} else {
		// function is already known
		fprintf(out, "%sfn=(%ld)\n", called ? "c" : "", function.outputIndex);
	}
}
