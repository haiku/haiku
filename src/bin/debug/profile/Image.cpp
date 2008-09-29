/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Image.h"

#include <stdio.h>

#include <algorithm>
#include <new>

#include <debug_support.h>
#include <ObjectList.h>

#include "Options.h"


Image::Image(const image_info& info, team_id owner, int32 creationEvent)
	:
	fInfo(info),
	fOwner(owner),
	fSymbols(NULL),
	fSymbolCount(0),
	fCreationEvent(creationEvent),
	fDeletionEvent(-1)
{
}


Image::~Image()
{
	if (fSymbols != NULL) {
		for (int32 i = 0; i < fSymbolCount; i++)
			delete fSymbols[i];
		delete[] fSymbols;
	}
}


status_t
Image::LoadSymbols(debug_symbol_lookup_context* lookupContext)
{
//	fprintf(sOptions.output, "Loading symbols of image \"%s\" (%ld)...\n",
//		fInfo.name, fInfo.id);

	// create symbol iterator
	debug_symbol_iterator* iterator;
	status_t error = debug_create_image_symbol_iterator(lookupContext,
		fInfo.id, &iterator);
	if (error != B_OK) {
		fprintf(stderr, "Failed to init symbol iterator: %s\n",
			strerror(error));
		return error;
	}

	// iterate through the symbols
	BObjectList<Symbol>	symbols(512, true);
	char symbolName[1024];
	int32 symbolType;
	void* symbolLocation;
	size_t symbolSize;
	while (debug_next_image_symbol(iterator, symbolName, sizeof(symbolName),
			&symbolType, &symbolLocation, &symbolSize) == B_OK) {
//		printf("  %s %p (%6lu) %s\n",
//			symbolType == B_SYMBOL_TYPE_TEXT ? "text" : "data",
//			symbolLocation, symbolSize, symbolName);
		if (symbolSize > 0 && symbolType == B_SYMBOL_TYPE_TEXT) {
			Symbol* symbol = new(std::nothrow) Symbol(this,
				(addr_t)symbolLocation, symbolSize, symbolName);
			if (symbol == NULL || !symbols.AddItem(symbol)) {
				delete symbol;
				fprintf(stderr, "%s: Out of memory\n", kCommandName);
				debug_delete_image_symbol_iterator(iterator);
				return B_NO_MEMORY;
			}
		}
	}

	debug_delete_image_symbol_iterator(iterator);

	// sort the symbols
	fSymbolCount = symbols.CountItems();
	fSymbols = new(std::nothrow) Symbol*[fSymbolCount];
	if (fSymbols == NULL)
		return B_NO_MEMORY;

	for (int32 i = fSymbolCount - 1; i >= 0 ; i--)
		fSymbols[i] = symbols.RemoveItemAt(i);

	std::sort(fSymbols, fSymbols + fSymbolCount, SymbolComparator());

	return B_OK;
}


int32
Image::FindSymbol(addr_t address) const
{
	// binary search the function
	int32 lower = 0;
	int32 upper = fSymbolCount;

	while (lower < upper) {
		int32 mid = (lower + upper) / 2;
		if (address >= fSymbols[mid]->base + fSymbols[mid]->size)
			lower = mid + 1;
		else
			upper = mid;
	}

	if (lower == fSymbolCount)
		return -1;

	const Symbol* symbol = fSymbols[lower];
	if (address >= symbol->base && address < symbol->base + symbol->size)
		return lower;
	return -1;
}
