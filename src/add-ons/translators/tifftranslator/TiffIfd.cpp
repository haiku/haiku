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

#include <stdio.h>
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
			printf("TiffIfd::ffieldCount: %d\n", ffieldCount);
			
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

// ---------------------------------------------------------------
// GetUintField
//
// Retrieves the TiffUintField with the given tag. 
//
// Preconditions:
//
// Parameters:	tag,		the field id tag
//
//				poutField,	the pointer to the desired field
//							is stored here
//
// Postconditions:
//
// Returns:	B_OK if the field was found and it is a TiffUintField
//			based object,
//
//			B_BAD_TYPE if the field was found BUT it was not a
//			TiffUintField based object
//
//			B_BAD_INDEX if a field with the given tag
//			was not found
// ---------------------------------------------------------------
status_t
TiffIfd::GetUintField(uint16 tag, TiffUintField *&poutField)
{
	for (uint16 i = 0; i < ffieldCount; i++) {
		TiffField *pfield;
		
		pfield = fpfields[i];
		if (pfield && tag == pfield->GetTag()) {
			TiffUintField *puintField = NULL;
			
			puintField = dynamic_cast<TiffUintField *>(pfield);
			if (puintField) {
				poutField = puintField;
				return B_OK;
			} else
				return B_BAD_TYPE;
		}
	}
	
	return B_BAD_INDEX;
}

