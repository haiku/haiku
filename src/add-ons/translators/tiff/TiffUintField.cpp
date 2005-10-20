/*****************************************************************************/
// TiffUintField
// Written by Michael Wilber, OBOS Translation Kit Team
//
// TiffUintField.cpp
//
// This object is for storing Unsigned Integer TIFF fields
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
#include "TiffUintField.h"

void
TiffUintField::LoadByte(IFDEntry &entry, BPositionIO &io, swap_action swp)
{
	// Make certain there is enough memory
	// before trying to do anything else
	fpByte = new uint8[entry.count];
	if (!fpByte) {
		finitStatus = B_NO_MEMORY;
		return;
	}
	
	if (entry.count <= 4) {
		// If all of the byte values can fit into the
		// IFD entry value bytes
		memcpy(fpByte, entry.bytevals, entry.count);
		finitStatus = B_OK;		
	} else {
	
		// entry.count > 4, use longval to find offset for byte data
		if (swap_data(B_UINT32_TYPE, &entry.longval, 4, swp) != B_OK)
			finitStatus = B_ERROR;
		else {
			ssize_t read;
			read = io.ReadAt(entry.longval, fpByte, entry.count);
			if (read != static_cast<ssize_t>(entry.count))
				finitStatus = B_IO_ERROR;
			else
				finitStatus = B_OK;
		}
		
	}
}

void
TiffUintField::LoadShort(IFDEntry &entry, BPositionIO &io, swap_action swp)
{
	// Make certain there is enough memory
	// before trying to do anything else
	fpShort = new uint16[entry.count];
	if (!fpShort) {
		finitStatus = B_NO_MEMORY;
		return;
	}
	
	if (entry.count <= 2) {
		// If all of the byte values can fit into the
		// IFD entry value bytes
		memcpy(fpShort, entry.shortvals, entry.count * 2);
		finitStatus = B_OK;		
	} else {
	
		// entry.count > 2, use longval to find offset for short data
		if (swap_data(B_UINT32_TYPE, &entry.longval, 4, swp) != B_OK)
			finitStatus = B_ERROR;
		else {
			ssize_t read;
			read = io.ReadAt(entry.longval, fpShort, entry.count * 2);
			if (read != static_cast<ssize_t>(entry.count) * 2)
				finitStatus = B_IO_ERROR;
			else
				finitStatus = B_OK;
		}
		
	}
	
	// If short values were successfully read in, swap them to
	// the correct byte order
	if (finitStatus == B_OK &&
		swap_data(B_UINT16_TYPE, fpShort, entry.count * 2, swp) != B_OK)
		finitStatus = B_ERROR;
}

void
TiffUintField::LoadLong(IFDEntry &entry, BPositionIO &io, swap_action swp)
{
	// Make certain there is enough memory
	// before trying to do anything else
	fpLong = new uint32[entry.count];
	if (!fpLong) {
		finitStatus = B_NO_MEMORY;
		return;
	}
	
	if (entry.count == 1) {
		fpLong[0] = entry.longval;
		finitStatus = B_OK;		
	} else {
	
		// entry.count > 1, use longval to find offset for long data
		if (swap_data(B_UINT32_TYPE, &entry.longval, 4, swp) != B_OK)
			finitStatus = B_ERROR;
		else {
			ssize_t read;
			read = io.ReadAt(entry.longval, fpLong, entry.count * 4);
			if (read != static_cast<ssize_t>(entry.count) * 4)
				finitStatus = B_IO_ERROR;
			else
				finitStatus = B_OK;
		}
		
	}
	
	// If long values were successfully read in, swap them to
	// the correct byte order
	if (finitStatus == B_OK &&
		swap_data(B_UINT32_TYPE, fpLong, entry.count * 4, swp) != B_OK)
		finitStatus = B_ERROR;
}

TiffUintField::TiffUintField(IFDEntry &entry, BPositionIO &io, swap_action swp)
	: TiffField(entry)
{
	if (entry.count > 0) {
		switch (entry.fieldType) {
			case TIFF_BYTE:
				fpByte = NULL;
				LoadByte(entry, io, swp);
				break;
				
			case TIFF_SHORT:
				fpShort = NULL;
				LoadShort(entry, io, swp);
				break;
				
			case TIFF_LONG:
				fpLong = NULL;
				LoadLong(entry, io, swp);
				break;
			
			default:
				finitStatus = B_BAD_TYPE;
				break;
		}
	} else
		finitStatus = B_BAD_VALUE;
}

TiffUintField::~TiffUintField()
{
	switch (GetType()) {
		case TIFF_BYTE:
			delete[] fpByte;
			fpByte = NULL;
			break;
		case TIFF_SHORT:
			delete[] fpShort;
			fpShort = NULL;
			break;
		case TIFF_LONG:
			delete[] fpLong;
			fpLong = NULL;
			break;
			
		default:
			// if invalid type, should have never
			// allocated any value, so there should
			// be no value to delete
			break;
	}
}

// ---------------------------------------------------------------
// GetUint
//
// Retrieves the number at the given index for this field.
// Some fields store a single number, others store an array of
// numbers.
//
// The index parameter has a default value of zero.  When
// index is zero, there is special behavior: the first
// number is retrieved if this field has a count of 1.  If
// the index is zero and the field's count is not 1, then
// B_BAD_INDEX will be returned.
//
// Preconditions:
//
// Parameters:	out,	where the desired uint is copied to
//
//				index,	one-based index for the desired value
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
TiffUintField::GetUint(uint32 &out, uint32 index)
{
	if (finitStatus != B_OK)
		return finitStatus;
		
	uint32 count = GetCount();
	if (index > count || (index == 0 && count != 1))
		return B_BAD_INDEX;
		
	status_t result = B_OK;
	if (!index)
		index = 1;
	switch (GetType()) {
		case TIFF_BYTE:
			out = fpByte[index - 1];
			break;
			
		case TIFF_SHORT:
			out = fpShort[index - 1];
			break;
			
		case TIFF_LONG:
			out = fpLong[index - 1];
			break;
			
		default:
			result = B_BAD_TYPE;
			break;
	}
	
	return result;
}
