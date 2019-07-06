/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "CallgrindProfileResult.h"

#include <errno.h>
#include <sys/stat.h>

#include <algorithm>
#include <new>

#include "Options.h"
#include "ProfiledEntity.h"


// #pragma mark - CallgrindImageProfileResult


CallgrindImageProfileResult::CallgrindImageProfileResult(SharedImage* image,
	image_id id)
	:
	ImageProfileResult(image, id),
	fFunctions(NULL),
	fOutputIndex(0)
{
}


CallgrindImageProfileResult::~CallgrindImageProfileResult()
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
CallgrindImageProfileResult::Init()
{
	int32 symbolCount = fImage->SymbolCount();
	fFunctions = new(std::nothrow) CallgrindFunction[symbolCount];
	if (fFunctions == NULL)
		return B_NO_MEMORY;

	memset(fFunctions, 0, sizeof(CallgrindFunction) * symbolCount);

	return B_OK;
}


void
CallgrindImageProfileResult::AddSymbolHit(int32 symbolIndex,
	CallgrindImageProfileResult* calledImage, int32 calledSymbol)

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
CallgrindImageProfileResult::Functions() const
{
	return fFunctions;
}


int32
CallgrindImageProfileResult::OutputIndex() const
{
	return fOutputIndex;
}


void
CallgrindImageProfileResult::SetOutputIndex(int32 index)
{
	fOutputIndex = index;
}


// #pragma mark - CallgrindProfileResult


CallgrindProfileResult::CallgrindProfileResult()
	:
	fTotalTicks(0),
	fUnkownTicks(0),
	fDroppedTicks(0),
	fNextImageOutputIndex(1),
	fNextFunctionOutputIndex(1)
{
}


void
CallgrindProfileResult::AddSamples(ImageProfileResultContainer* container,
	addr_t* samples, int32 sampleCount)
{
	int32 unknownSamples = 0;
	CallgrindImageProfileResult* previousImage = NULL;
	int32 previousSymbol = -1;

	// TODO: That probably doesn't work with recursive functions.
	for (int32 i = 0; i < sampleCount; i++) {
		addr_t address = samples[i];
		addr_t loadDelta;
		CallgrindImageProfileResult* image
			= static_cast<CallgrindImageProfileResult*>(
				container->FindImage(address, loadDelta));
		int32 symbol = -1;
		if (image != NULL) {
			symbol = image->GetImage()->FindSymbol(address - loadDelta);
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
CallgrindProfileResult::AddDroppedTicks(int32 dropped)
{
	fDroppedTicks += dropped;
}


void
CallgrindProfileResult::PrintResults(ImageProfileResultContainer* container)
{
	// create output file

	// create output dir
	mkdir(gOptions.callgrind_directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

	// get the entity name and replace slashes by hyphens
	char entityName[B_OS_NAME_LENGTH];
	strlcpy(entityName, fEntity->EntityName(), sizeof(entityName));
	char* slash = entityName;
	while ((slash = strchr(slash, '/')) != NULL)
		*slash = '-';

	// create the file name
	char fileName[B_PATH_NAME_LENGTH];
	snprintf(fileName, sizeof(fileName),
		"%s/callgrind.out.%" B_PRId32 ".%s.%" B_PRId64 "ms",
		gOptions.callgrind_directory, fEntity->EntityID(), entityName,
		fTotalTicks * fInterval);

	// create the file
	FILE* out = fopen(fileName, "w+");
	if (out == NULL) {
		fprintf(stderr, "%s: Failed to open output file \"%s\": %s\n",
			kCommandName, fileName, strerror(errno));
		return;
	}

	// write the header
	fprintf(out, "version: 1\n");
	fprintf(out, "creator: Haiku profile\n");
	fprintf(out, "pid: %" B_PRId32 "\n", fEntity->EntityID());
	fprintf(out, "cmd: %s\n", fEntity->EntityName());
	fprintf(out, "part: 1\n\n");

	fprintf(out, "positions: line\n");
	fprintf(out, "events: Ticks Time\n");
	fprintf(out, "summary: %" B_PRId64 " %" B_PRId64 "\n",
		fTotalTicks, fTotalTicks * fInterval);

	// get hit images
	CallgrindImageProfileResult* images[container->CountImages()];
	int32 imageCount = GetHitImages(container, images);

	for (int32 i = 0; i < imageCount; i++) {
		CallgrindImageProfileResult* image = images[i];

		CallgrindFunction* functions = image->Functions();
		int32 imageSymbolCount = image->GetImage()->SymbolCount();
		for (int32 k = 0; k < imageSymbolCount; k++) {
			CallgrindFunction& function = functions[k];
			if (function.hits == 0 && function.calledFunctions == NULL)
				continue;

			fprintf(out, "\n");
			_PrintFunction(out, image, k, false);
			fprintf(out, "0 %" B_PRId64 " %" B_PRId64 "\n", function.hits,
				function.hits * fInterval);

			CallgrindCalledFunction* calledFunction = function.calledFunctions;
			while (calledFunction != NULL) {
				_PrintFunction(out, calledFunction->image,
					calledFunction->function, true);
				fprintf(out, "calls=%" B_PRId64 " 0\n", calledFunction->hits);
				fprintf(out, "0 %" B_PRId64 " %" B_PRId64 "\n",
					calledFunction->hits, calledFunction->hits * fInterval);
				calledFunction = calledFunction->next;
			}
		}
	}

	// print pseudo-functions for unknown and dropped ticks
	if (fUnkownTicks + fDroppedTicks > 0) {
		fprintf(out, "\nob=<pseudo>\n");

		if (fUnkownTicks > 0) {
			fprintf(out, "\nfn=unknown\n");
			fprintf(out, "0 %" B_PRId64 "\n", fUnkownTicks);
		}

		if (fDroppedTicks > 0) {
			fprintf(out, "\nfn=dropped\n");
			fprintf(out, "0 %" B_PRId64 "\n", fDroppedTicks);
		}
	}

	fprintf(out, "\ntotals: %" B_PRId64 " %" B_PRId64 "\n",
		fTotalTicks, fTotalTicks * fInterval);

	fclose(out);
}


status_t
CallgrindProfileResult::GetImageProfileResult(SharedImage* image, image_id id,
	ImageProfileResult*& _imageResult)
{
	CallgrindImageProfileResult* result
		= new(std::nothrow) CallgrindImageProfileResult(image, id);
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


void
CallgrindProfileResult::_PrintFunction(FILE* out,
	CallgrindImageProfileResult* image, int32 functionIndex, bool called)
{
	if (image->OutputIndex() == 0) {
		// need to print the image name
		int32 index = fNextImageOutputIndex++;
		image->SetOutputIndex(index);
		fprintf(out,
			"%sob=(%" B_PRId32 ") %s:%" B_PRId32 "\n", called ? "c" : "",
			index, image->GetImage()->Name(), image->ID());
	} else {
		// image is already known
		// TODO: We may not need to print it at all!
		fprintf(out,
			"%sob=(%" B_PRId32 ")\n", called ? "c" : "", image->OutputIndex());
	}

	CallgrindFunction& function = image->Functions()[functionIndex];
	if (function.outputIndex == 0) {
		// need to print the function name
		function.outputIndex = fNextFunctionOutputIndex++;
		fprintf(out,
			"%sfn=(%" B_PRId32 ") %s\n", called ? "c" : "",
			function.outputIndex,
			image->GetImage()->Symbols()[functionIndex]->Name());
	} else {
		// function is already known
		fprintf(out,
			"%sfn=(%" B_PRId32 ")\n", called ? "c" : "", function.outputIndex);
	}
}
