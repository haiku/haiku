/*****************************************************************************/
// TiffIfd
// Written by Michael Wilber, OBOS Translation Kit Team
//
// TiffIfd.cpp
//
// This object is for storing a TIFF Image File Directory
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <string.h>
#include "TiffIfd.h"
#include "TiffUintField.h"
#include "TiffUnknownField.h"

void
TiffIfd::LoadFields(uint32 offset, BPositionIO &io, swap_action swp)
{
	if (io.ReadAt(offset, &ffieldCount, 2) != 2) {
		finitStatus = B_IO_ERROR;
		return;
	} else {
		if (swap_data(B_UINT16_TYPE, &ffieldCount, 2, swp) != B_OK) {
			finitStatus = B_ERROR;
			return;
		} else {
			fpfields = new TiffField *[ffieldCount];
			if (!fpfields) {
				finitStatus = B_NO_MEMORY;
				return;
			} else {
				memset(fpfields, 0, ffieldCount * sizeof(TiffField *));
					// set all pointers to TiffFields to NULL

				// Load TIFF IFD Entries
				offset += 2;
				uint16 i;
				finitStatus = B_OK;
				for (i = 0; i < ffieldCount; i++, offset += 12) {
					IFDEntry entry;

					if (io.ReadAt(offset, &entry, 12) != 12) {
						finitStatus = B_IO_ERROR;
						return;
					}
					if (swap_data(B_UINT16_TYPE, &entry.tag, 2, swp) != B_OK) {
						finitStatus = B_ERROR;
						return;
					}
					if (swap_data(B_UINT16_TYPE, &entry.fieldType, 2, swp) != B_OK) {
						finitStatus = B_ERROR;
						return;
					}
					if (swap_data(B_UINT32_TYPE, &entry.count, 4, swp) != B_OK) {
						finitStatus = B_ERROR;
						return;
					}
					
					// Create a TiffField based object to store the
					// TIFF entry
					switch (entry.fieldType) {
						case TIFF_BYTE:
						case TIFF_SHORT:
						case TIFF_LONG:
							fpfields[i] = new TiffUintField(entry, io, swp);
							break;
							
						default:
							fpfields[i] = new TiffUnknownField(entry);
							break;
					}
					if (!fpfields[i]) {
						finitStatus = B_NO_MEMORY;
						return;
					}
					if (fpfields[i]->InitCheck() != B_OK) {
						finitStatus = fpfields[i]->InitCheck();
						return;
					}
				} // for (i = 0; i < ffieldCount; i++, offset += 12)
				
				// Read address of next IFD entry
				if (io.ReadAt(offset, &fnextIFDOffset, 4) != 4) {
					finitStatus = B_IO_ERROR;
					return;
				}
				if (swap_data(B_UINT32_TYPE, &fnextIFDOffset, 4, swp) != B_OK) {
					finitStatus = B_ERROR;
					return;
				}
			}
		}
	}
}

TiffIfd::TiffIfd(uint32 offset, BPositionIO &io, swap_action swp)
{
	fpfields = NULL;
	finitStatus = B_ERROR;
	fnextIFDOffset = 0;
	ffieldCount = 0;

	LoadFields(offset, io, swp);
}

TiffIfd::~TiffIfd()
{
	for (uint16 i = 0; i < ffieldCount; i++) {
		delete fpfields[i];
		fpfields[i] = NULL;
	}
	delete[] fpfields;
	fpfields = NULL;
	
	ffieldCount = 0;
	fnextIFDOffset = 0;
}

bool
TiffIfd::HasField(uint16 tag)
{
	TiffField *pfield = GetField(tag);
	if (pfield)
		return true;
	else
		return false;
}

uint32
TiffIfd::GetCount(uint16 tag)
{
	TiffField *pfield = GetField(tag);
	if (pfield)
		return pfield->GetCount();
	else
		throw TiffIfdFieldNotFoundException();
}

uint32
TiffIfd::GetUint(uint16 tag, uint32 index)
{
	TiffField *pfield = GetField(tag);
	if (pfield) {
	
		TiffUintField *puintField = NULL;		
		puintField = dynamic_cast<TiffUintField *>(pfield);
		if (puintField) {
		
			uint32 num;
			status_t ret = puintField->GetUint(num, index);
			if (ret == B_OK)
				return num;
			else if (ret == B_BAD_INDEX)
				throw TiffIfdBadIndexException();
			else
				throw TiffIfdUnexpectedTypeException();
				
		} else
			throw TiffIfdUnexpectedTypeException();
	}
	
	throw TiffIfdFieldNotFoundException();
}

uint32
TiffIfd::GetAdjustedColorMap(uint8 **pout)
{
	TiffField *pfield = GetField(TAG_COLOR_MAP);
	if (pfield) {
	
		TiffUintField *puintField = NULL;		
		puintField = dynamic_cast<TiffUintField *>(pfield);
		if (puintField) {
		
			uint32 count = puintField->GetCount();
			(*pout) = new uint8[count];
			uint8 *puints = (*pout);
			if (!puints)
				throw TiffIfdNoMemoryException();
				
			for (uint32 i = 0; i < count; i++) {
				uint32 num;
				status_t ret = puintField->GetUint(num, i + 1);
				if (ret == B_BAD_INDEX)
					throw TiffIfdBadIndexException();
				else if (ret == B_BAD_TYPE)
					throw TiffIfdUnexpectedTypeException();
					
				puints[i] = num / 256;
			}
			
			return count;
				
		} else
			throw TiffIfdUnexpectedTypeException();
	}
	
	throw TiffIfdFieldNotFoundException();
}

uint32
TiffIfd::GetUint32Array(uint16 tag, uint32 **pout)
{
	TiffField *pfield = GetField(tag);
	if (pfield) {
	
		TiffUintField *puintField = NULL;		
		puintField = dynamic_cast<TiffUintField *>(pfield);
		if (puintField) {
		
			uint32 count = puintField->GetCount();
			(*pout) = new uint32[count];
			uint32 *puints = (*pout);
			if (!puints)
				throw TiffIfdNoMemoryException();
				
			for (uint32 i = 0; i < count; i++) {
				uint32 num;
				status_t ret = puintField->GetUint(num, i + 1);
				if (ret == B_BAD_INDEX)
					throw TiffIfdBadIndexException();
				else if (ret == B_BAD_TYPE)
					throw TiffIfdUnexpectedTypeException();
					
				puints[i] = num;
			}
			
			return count;
				
		} else
			throw TiffIfdUnexpectedTypeException();
	}
	
	throw TiffIfdFieldNotFoundException();
}

TiffField *
TiffIfd::GetField(uint16 tag)
{
	for (uint16 i = 0; i < ffieldCount; i++) {
		TiffField *pfield = fpfields[i];
		if (pfield) {
			uint16 ltag = pfield->GetTag();
			if (tag == ltag)
				return pfield;
			else if (ltag > tag)
				break;
		}
	}
	
	return NULL;
}

