//------------------------------------------------------------------------------
//	Copyright (c) 2003, Ingo Weinhold
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ElfImage.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Implementation of ElfImage, a class encapsulating
//					a loaded ELF image, providing support for accessing the
//					image's symbols and their relocation entries.
//------------------------------------------------------------------------------

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <List.h>

#include "ElfImage.h"

// ElfImage

// constructor
ElfImage::ElfImage()
	: fImage(-1),
	  fFile(),
	  fTextAddress(NULL),
	  fDataAddress(NULL),
	  fGotAddress(NULL)
{
}

// destructor
ElfImage::~ElfImage()
{
	Unset();
}

// SetTo
status_t
ElfImage::SetTo(image_id image)
{
	Unset();
	status_t error = _SetTo(image);
	if (error)
		Unset();
	return error;
}

// Unset
void
ElfImage::Unset()
{
	fFile.Unset();
	fImage = -1;
	fTextAddress = NULL;
	fDataAddress = NULL;
	fGotAddress = NULL;
}

// Unload
void
ElfImage::Unload()
{
	fFile.Unload();
}

// FindSymbol
status_t
ElfImage::FindSymbol(const char* symbolName, void** address)
{
	return get_image_symbol(fImage, symbolName, B_SYMBOL_TYPE_ANY, address);
}

// GetSymbolRelocations
status_t
ElfImage::GetSymbolRelocations(const char* symbolName, BList* relocations)
{
	status_t error = B_OK;
	ElfRelocation relocation;
	for (ElfRelocationIterator it(&fFile); it.GetNext(&relocation); ) {
		uint32 type = relocation.GetType();
		// get the symbol
		ElfSymbol symbol;
		if ((type == R_386_GLOB_DAT || type == R_386_JMP_SLOT)
			&& relocation.GetSymbol(&symbol) == B_OK
			&& symbol.GetName()) {
			// only undefined symbols with global binding
			if ((symbol.GetBinding() == STB_GLOBAL
				 || symbol.GetBinding() == STB_WEAK)
				&& (symbol.GetTargetSectionIndex() == SHN_UNDEF
					|| symbol.GetTargetSectionIndex()
					   >= (uint32)fFile.CountSections())
				&& !strcmp(symbol.GetName(), symbolName)) {
				// get the address of the GOT entry for the symbol
				void** gotEntry
					= (void**)(fTextAddress + relocation.GetOffset());
				if (!relocations->AddItem(gotEntry)) {
					error = B_NO_MEMORY;
					break;
				}
			}
		}
	}
	return error;
}

// _SetTo
status_t
ElfImage::_SetTo(image_id image)
{
	// get an image info
	image_info imageInfo;
	status_t error = get_image_info(image, &imageInfo);
	if (error != B_OK)
		return error;
	fImage = imageInfo.id;
	// get the address of global offset table
	error = get_image_symbol(image, "_GLOBAL_OFFSET_TABLE_",
							 B_SYMBOL_TYPE_ANY, (void**)&fGotAddress);
	if (error != B_OK)
		return error;
	fTextAddress = (uint8*)imageInfo.text;
	fDataAddress = (uint8*)imageInfo.data;
	// init the file
	error = fFile.SetTo(imageInfo.name);
	if (error != B_OK)
		return error;
	return B_OK;
}

