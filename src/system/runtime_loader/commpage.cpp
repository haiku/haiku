/*
 * Copyright 2025, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#include "commpage.h"

#include <string.h>

#include <syscalls.h>

#include "runtime_loader_private.h"


static const char* const kCommpageName = "commpage";


static image_id sCommpageImageID = -1;
static int32 sCommpageSymbolCount = -1;
static void* sCommpageSymbolsBuffer = NULL;
static elf_sym* sCommpageSymbols = NULL;
static char* sCommpageStringTable = NULL;


status_t
commpage_reinit_after_fork()
{
	sCommpageImageID = -1;
	return B_OK;
}


static status_t
get_commpage_image_id(team_id team)
{
	image_info info;
	int32 cookie = 0;
	while (_kern_get_next_image_info(team, &cookie, &info, sizeof(info)) == B_OK) {
		if (strcmp(info.name, kCommpageName) == 0)
			return info.id;
	}
	return B_ENTRY_NOT_FOUND;
}


status_t
get_nearest_commpage_symbol_at_address_locked(void* address, image_id* _imageID, char** _imagePath,
	char** _imageName, char** _symbolName, int32* _type, void** _location, bool* _exactMatch)
{
	status_t status;
	bool wantsImage = _imageID != NULL;
	bool wantsSymbol = _symbolName != NULL || _type != NULL || _location != NULL;

	if (wantsImage && sCommpageImageID == -1) {
		if (sCommpageImageID == -1) {
			status = get_commpage_image_id(B_CURRENT_TEAM);
			if (status < B_OK)
				return status;
			sCommpageImageID = status;
		}
	}

	if (wantsSymbol && sCommpageSymbolCount == -1) {
		image_id systemCommpageID = get_commpage_image_id(B_SYSTEM_TEAM);
		if (systemCommpageID < B_OK)
			return systemCommpageID;

		int32 symbolCount = 0;
		size_t stringTableSize = 0;

		status = _kern_read_kernel_image_symbols(systemCommpageID, NULL,
			&symbolCount, NULL, &stringTableSize, NULL);
		if (status < B_OK)
			return status;

		size_t memorySize = symbolCount * sizeof(elf_sym) + stringTableSize + 1;
		void* buffer = malloc(memorySize);
		if (buffer == NULL)
			return B_NO_MEMORY;

		elf_sym* symbols = (elf_sym*)buffer;
		char* stringTable = (char*)buffer + symbolCount * sizeof(elf_sym);
		status = _kern_read_kernel_image_symbols(systemCommpageID, symbols, &symbolCount,
			stringTable, &stringTableSize, NULL);
		if (status < B_OK) {
			free(buffer);
			return status;
		}

		sCommpageSymbolCount = symbolCount;
		sCommpageSymbolsBuffer = buffer;
		sCommpageSymbols = symbols;
		sCommpageStringTable = stringTable;
	}

	if (_imageID != NULL)
		*_imageID = sCommpageImageID;
	if (_imagePath != NULL)
		*_imagePath = (char*)kCommpageName;
	if (_imageName != NULL)
		*_imageName = (char*)kCommpageName;

	bool exactMatch = false;
	elf_sym* foundSymbol = NULL;
	addr_t foundLocation = (addr_t)NULL;

	for (int32 i = 0; i < sCommpageSymbolCount && !exactMatch; i++) {
		elf_sym* symbol = &sCommpageSymbols[i];
		addr_t location = (addr_t)__gCommPageAddress + symbol->st_value;

		if (location <= (addr_t)address && location >= foundLocation) {
			foundSymbol = symbol;
			foundLocation = location;

			// jump out if we have an exact match
			if (location + symbol->st_size > (addr_t)address) {
				exactMatch = true;
				break;
			}
		}
	}

	if (_exactMatch != NULL)
		*_exactMatch = exactMatch;

	if (foundSymbol != NULL) {
		*_symbolName = sCommpageStringTable + foundSymbol->st_name;

		if (_type != NULL) {
			if (foundSymbol->Type() == STT_FUNC)
				*_type = B_SYMBOL_TYPE_TEXT;
			else if (foundSymbol->Type() == STT_OBJECT)
				*_type = B_SYMBOL_TYPE_DATA;
			else
				*_type = B_SYMBOL_TYPE_ANY;
		}

		if (_location != NULL)
			*_location = (void*)foundLocation;
	} else {
		*_symbolName = NULL;
		if (_location != NULL)
			*_location = NULL;
	}

	return B_OK;
}
