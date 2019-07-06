/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "SharedImage.h"

#include <stdio.h>

#include <algorithm>
#include <new>

#include <debug_support.h>
#include <ObjectList.h>

#include "Options.h"


SharedImage::SharedImage()
	:
	fSymbols(NULL),
	fSymbolCount(0)
{
}


SharedImage::~SharedImage()
{
	if (fSymbols != NULL) {
		for (int32 i = 0; i < fSymbolCount; i++)
			delete fSymbols[i];
		delete[] fSymbols;
	}
}


status_t
SharedImage::Init(team_id owner, image_id imageID)
{
	// we need a temporary symbol lookup context
	debug_symbol_lookup_context* lookupContext;
	status_t error = debug_create_symbol_lookup_context(owner, imageID,
		&lookupContext);
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to create symbol lookup context "
			"for team %" B_PRId32 ": %s\n",
			kCommandName, owner, strerror(error));
		return error;
	}

	// TODO: Creating a symbol lookup just for loading the symbols of a single
	// image is unnecessarily expensive.

	// create a symbol iterator
	debug_symbol_iterator* iterator;
	error = debug_create_image_symbol_iterator(lookupContext, imageID,
		&iterator);
	if (error != B_OK) {
		fprintf(stderr,
			"Failed to init symbol iterator for image %" B_PRId32 ": %s\n",
			imageID, strerror(error));
		debug_delete_symbol_lookup_context(lookupContext);
		return error;
	}

	// init
	error = _Init(iterator);

	// cleanup
	debug_delete_symbol_iterator(iterator);
	debug_delete_symbol_lookup_context(lookupContext);

	return error;
}


status_t
SharedImage::Init(const char* path)
{
	// create a symbol iterator
	debug_symbol_iterator* iterator;
	status_t error = debug_create_file_symbol_iterator(path, &iterator);
	if (error != B_OK) {
		fprintf(stderr, "Failed to init symbol iterator for \"%s\": %s\n",
			path, strerror(error));
		return error;
	}

	error = _Init(iterator);

	debug_delete_symbol_iterator(iterator);

	return error;
}


int32
SharedImage::FindSymbol(addr_t address) const
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


status_t
SharedImage::_Init(debug_symbol_iterator* iterator)
{
	// get an image info
	status_t error = debug_get_symbol_iterator_image_info(iterator, &fInfo);
	if (error != B_OK)
		return error;

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
				return B_NO_MEMORY;
			}
		}
	}

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
