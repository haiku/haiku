/*****************************************************************************/
//               File: BitmapStream.cpp
//              Class: BBitmapStream
//   Reimplimented by: Travis Smith, Michael Wilber, Translation Kit Team
//   Reimplimentation: 2002-04
//
// Description: BPositionIO based object to read/write bitmap format to/from
//              a BBitmap object.
//
//              The BTranslationUtils class uses this object and makes it
//              easy for users to load bitmaps.
//
// Copyright (c) 2002 OpenBeOS Project
//
// Original Version: Copyright 1998, Be Incorporated, All Rights Reserved.
//                   Copyright 1995-1997, Jon Watte
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

#include <BitmapStream.h>
#include <Bitmap.h>
#include <Debug.h>
#include <string.h>


// ---------------------------------------------------------------
// Constructor
//
// Initializes this object to either use the BBitmap passed to
// it as the object to read/write to or to create a BBitmap
// when data is written to this object.
//
// Preconditions:
//
// Parameters: bitmap, the bitmap used to read from/write to,
//                     if it is NULL, a bitmap is created when
//                     this object is written to
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
BBitmapStream::BBitmapStream(BBitmap *bitmap)
{
	fBitmap = bitmap;
	fDetached = false;
	fPosition = 0;
	fSize = 0;
	fpBigEndianHeader = NULL;
	
	// Extract header information if bitmap is available
	if (fBitmap) {
		fHeader.magic = B_TRANSLATOR_BITMAP;
		fHeader.bounds = fBitmap->Bounds();
		fHeader.rowBytes = fBitmap->BytesPerRow();
		fHeader.colors = fBitmap->ColorSpace();
		fHeader.dataSize = (uint32)
			(fHeader.bounds.Height() + 1) * fHeader.rowBytes;
		fSize = sizeof(TranslatorBitmap) + fHeader.dataSize;
		
		SetBigEndianHeader();
	}
}

// ---------------------------------------------------------------
// Destructor
//
// Destroys memory used by this object, but does not destroy the
// bitmap used by this object if it has been detached. This is so
// the user can use that bitmap after this object is destroyed.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
BBitmapStream::~BBitmapStream()
{
	if (fBitmap && !fDetached)
		delete fBitmap;
		
	if (fpBigEndianHeader)
		delete fpBigEndianHeader;
}

// ---------------------------------------------------------------
// ReadAt
//
// Reads data from the stream at a specific position and for a
// specific amount. The first sizeof(TranslatorBitmap) bytes
// are the bitmap header. The header is always written out
// and read in as Big Endian byte order.
//
// Preconditions:
//
// Parameters: pos, the position in the stream to read from
//             buffer, where the data will be read into
//             size, the amount of data to read
//
// Postconditions:
//
// Returns: B_ERROR if there is no bitmap stored by the stream
//                  or if pos is a bad value,
//          B_BAD_VALUE if buffer is NULL
//          or the amount read if the result >= 0
// ---------------------------------------------------------------
status_t
BBitmapStream::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (!buffer)
		return B_BAD_VALUE;
	if (!fBitmap)
		return B_ERROR;
	if (!size)
		return B_NO_ERROR;
	if (pos >= fSize)
		return B_ERROR;

	long toRead;
	void *source;

	if (fPosition < sizeof(TranslatorBitmap)) {
		toRead = sizeof(TranslatorBitmap) - pos;
		source = ((char *)fpBigEndianHeader) + pos;
	} else {
		toRead = fSize - pos;
		source = ((char *)fBitmap->Bits()) + fPosition -
			sizeof(TranslatorBitmap);
	}
	if (toRead > size)
		toRead = size;

	memcpy(buffer, source, toRead);
	return toRead;
}

// ---------------------------------------------------------------
// WriteAt
//
// Writes data to the bitmap from data, starting at position pos
// of size, size. The first sizeof(TranslatorBitmap) bytes
// of data must be the TranslatorBitmap header in the Big
// Endian byte order, otherwise, the data will fail to be
// successfully written.
//
// Preconditions:
//
// Parameters: pos, the position in the stream to write to
//             data, the data to write to the stream
//             size, the size of the data to write to the stream
//
// Postconditions:
//
// Returns: B_BAD_VALUE if size is bad or data is NULL,
//          B_MISMATCHED_VALUES if the bitmap header is bad,
//          or the amount written if the result is >= 0
// ---------------------------------------------------------------
status_t
BBitmapStream::WriteAt(off_t pos, const void *data, size_t size)
{
	if (!data)
		return B_BAD_VALUE;
	if (!size)
		return B_NO_ERROR;

	ssize_t written = 0;
	while (size > 0) {
		long toWrite;
		void *dest;
		// We depend on writing the header separately in detecting
		// changes to it
		if (pos < sizeof(TranslatorBitmap)) {
			toWrite = sizeof(TranslatorBitmap) - pos;
			dest = ((char *)&fHeader) + pos;
		} else {
			toWrite = fHeader.dataSize - pos + sizeof(TranslatorBitmap);
			dest = ((char *)fBitmap->Bits()) + pos - sizeof(TranslatorBitmap);
		}
		if (toWrite > size)
			toWrite = size;
		if (!toWrite && size)
			// i.e. we've been told to write too much
			return B_BAD_VALUE;

		memcpy(dest, data, toWrite);
		pos += toWrite;
		written += toWrite;
		data = ((char *)data) + toWrite;
		size -= toWrite;
		if (pos > fSize)
			fSize = pos;
		// If we change the header, the rest needs to be reset
		if (pos == sizeof(TranslatorBitmap)) {
			// Setup both host and Big Endian byte order bitmap headers
			ConvertBEndianToHost(&fHeader);
			SetBigEndianHeader();
			
			if (fBitmap && ((fBitmap->Bounds() != fHeader.bounds) ||
					(fBitmap->ColorSpace() != fHeader.colors) ||
					(fBitmap->BytesPerRow() != fHeader.rowBytes))) {
				if (!fDetached)
					// if someone detached, we don't delete
					delete fBitmap;
				fBitmap = NULL;
			}
			if (!fBitmap) {
				if ((fHeader.bounds.left > 0.0) || (fHeader.bounds.top > 0.0))
					DEBUGGER("non-origin bounds!");
				fBitmap = new BBitmap(fHeader.bounds, fHeader.colors);
				if (fBitmap->BytesPerRow() != fHeader.rowBytes)
					return B_MISMATCHED_VALUES;
			}
			if (fBitmap)
				fSize = sizeof(TranslatorBitmap) + fBitmap->BitsLength();
		}
	}
	return written;
}

// ---------------------------------------------------------------
// Seeks
//
// Changes the current stream position.
//
// Preconditions:
//
// Parameters: position, the position offset
//             whence, decides how the position offset is used
//                     SEEK_CUR, position is added to current 
//                               stream position
//                     SEEK_END, position is added to the end
//                               stream position
//                     SEEK_SET, the stream position is set to
//                               position
//
// Postconditions:
//
// Returns: B_BAD_VALUE if the position is bad
//          or the new position value if the result >= 0
// ---------------------------------------------------------------
off_t
BBitmapStream::Seek(off_t position, uint32 whence)
{
	// When whence == SEEK_SET, it just falls through to
	// fPosition = position
	if (whence == SEEK_CUR)
		position += fPosition;
	if (whence == SEEK_END)
		position += fSize;

	if (position < 0)
		return B_BAD_VALUE;
	if (position > fSize)
		return B_BAD_VALUE;
		
	fPosition = position;
	return fPosition;
}

// ---------------------------------------------------------------
// Position
//
// Returns the current stream position
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: returns the curren stream position
// ---------------------------------------------------------------
off_t
BBitmapStream::Position() const
{
	return fPosition;
}


// ---------------------------------------------------------------
// Size
//
// Returns the curren stream size
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns: returns the current stream size
// ---------------------------------------------------------------
off_t
BBitmapStream::Size() const
{
	return fSize;
}

// ---------------------------------------------------------------
// SetSize
//
// Sets the size of the data, but I'm not sure if this function
// has any real purpose.
//
// Preconditions:
//
// Parameters: size, the size to set the stream size to.
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if size is a bad value
//          B_NO_ERROR, if size is a valid value
// ---------------------------------------------------------------
status_t
BBitmapStream::SetSize(off_t size)
{
	if (size < 0)
		return B_BAD_VALUE;
	if (fBitmap && (size > fHeader.dataSize + sizeof(TranslatorBitmap)))
		return B_BAD_VALUE;
	//	Problem:
	//	What if someone calls SetSize() before writing the header,
	//  so we don't know what bitmap to create?
	//	Solution:
	//	We assume people will write the header before any data, 
	//	so SetSize() is really not going to do anything.
	if (fBitmap)
		// if we checked that the size was OK
		fSize = size;

	return B_NO_ERROR;
}

// ---------------------------------------------------------------
// DetachBitmap
//
// Returns the internal bitmap through outBitmap so the user
// can do whatever they want to it. It means that when the
// BBitmapStream is deleted, the bitmap is not deleted. After
// the bitmap has been detached, it is still used by the stream,
// but it is never deleted by the stream.
//
// Preconditions:
//
// Parameters: outBitmap, where the bitmap is detached to
//
// Postconditions:
//
// Returns: B_BAD_VALUE, if outBitmap is NULL or the internal
//                       bitmap is NULL
//          B_ERROR,     if the bitmap has already been detached
//          B_NO_ERROR,  if the bitmap was successfully detached
// ---------------------------------------------------------------
status_t
BBitmapStream::DetachBitmap(BBitmap **outBitmap)
{
	if (!outBitmap || !fBitmap)
		return B_BAD_VALUE;
	if (fDetached)
		return B_ERROR;
		
	fDetached = true;
	*outBitmap = fBitmap;
	
	return B_NO_ERROR;
}

// ---------------------------------------------------------------
// ConvertBEndianToHost
//
// This static function converts a TranslatorBitmap from Big
// Endian byte order to the host byte order.
//
// Preconditions: Data pointed to by pheader must be in
//                Big Endian byte order
//
// Parameters: pheader, the TranslatorBitmap structure to convert
//
// Postconditions: 
//
// Returns: B_OK, if the byte swap succeeded
//          B_BAD_VALUE, if pheader is NULL, 
//                       or the byte swap failed
// ---------------------------------------------------------------
status_t
BBitmapStream::ConvertBEndianToHost(TranslatorBitmap *pheader)
{
	if (!pheader)
		return B_BAD_VALUE;
		
	return swap_data(B_UINT32_TYPE, pheader, sizeof(TranslatorBitmap),
		B_SWAP_BENDIAN_TO_HOST);
}

// ---------------------------------------------------------------
// SetBigEndianHeader
//
// Assigns fpBigEndianHeader to the Big Endian byte order version
// of fHeader, the bitmap header information. fpBigEndianHeader is
// used by ReadAt() to send out the bitmap header in the Big
// Endian byte order.
//
// Preconditions: fHeader must be in the host byte order for
//                this function to work properly
//
// Parameters:
//
// Postconditions:
//
// Returns: B_OK, if the byte swap succeeded
//          B_BAD_VALUE, if the byte swap failed
// ---------------------------------------------------------------
status_t
BBitmapStream::SetBigEndianHeader()
{
	if (!fpBigEndianHeader)
		fpBigEndianHeader = new TranslatorBitmap;
		
	fpBigEndianHeader->magic = fHeader.magic;
	fpBigEndianHeader->bounds = fHeader.bounds;
	fpBigEndianHeader->rowBytes = fHeader.rowBytes;
	fpBigEndianHeader->colors = fHeader.colors;
	fpBigEndianHeader->dataSize = fHeader.dataSize;
	
	return swap_data(B_UINT32_TYPE, fpBigEndianHeader,
		sizeof(TranslatorBitmap), B_SWAP_HOST_TO_BENDIAN);
}

// ---------------------------------------------------------------
// _ReservedBitmapStream1()
//
// It doesn't do anything :). Its here only for past/future
// binary compatibility.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
void
BBitmapStream::_ReservedBitmapStream1()
{
}

// ---------------------------------------------------------------
// _ReservedBitmapStream2()
//
// It doesn't do anything :). Its only here for past/future
// binary compatibility.
//
// Preconditions:
//
// Parameters:
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
void
BBitmapStream::_ReservedBitmapStream2()
{
}

