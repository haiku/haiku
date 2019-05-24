/*
 * Copyright 2002-2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Travis Smith
 *		Michael Wilber
 */


#include <BitmapStream.h>

#include <new>

#include <string.h>

#include <Bitmap.h>
#include <Debug.h>


// Initializes this object to either use the BBitmap passed to
// it as the object to read/write to or to create a BBitmap
// when data is written to this object.
BBitmapStream::BBitmapStream(BBitmap* bitmap)
{
	fBitmap = bitmap;
	fDetached = false;
	fPosition = 0;
	fSize = 0;
	fBigEndianHeader = new (std::nothrow) TranslatorBitmap;
	if (fBigEndianHeader == NULL) {
		fBitmap = NULL;
		return;
	}

	// Extract header information if bitmap is available
	if (fBitmap != NULL && fBitmap->InitCheck() == B_OK) {
		fHeader.magic = B_TRANSLATOR_BITMAP;
		fHeader.bounds = fBitmap->Bounds();
		fHeader.rowBytes = fBitmap->BytesPerRow();
		fHeader.colors = fBitmap->ColorSpace();
		fHeader.dataSize = static_cast<uint32>
			((fHeader.bounds.Height() + 1) * fHeader.rowBytes);
		fSize = sizeof(TranslatorBitmap) + fHeader.dataSize;

		if (B_HOST_IS_BENDIAN)
			*fBigEndianHeader = fHeader;
		else
			SwapHeader(&fHeader, fBigEndianHeader);
	} else
		fBitmap = NULL;
}


BBitmapStream::~BBitmapStream()
{
	if (!fDetached)
		delete fBitmap;

	delete fBigEndianHeader;
}


// Reads data from the stream at a specific position and size.
ssize_t
BBitmapStream::ReadAt(off_t pos, void* buffer, size_t size)
{
	if (fBitmap == NULL)
		return B_NO_INIT;
	if (size == 0)
		return B_OK;
	if (pos >= (off_t)fSize || pos < 0 || buffer == NULL)
		return B_BAD_VALUE;

	ssize_t toRead;
	void *source;

	if (pos < (off_t)sizeof(TranslatorBitmap)) {
		toRead = sizeof(TranslatorBitmap) - pos;
		source = (reinterpret_cast<uint8 *>(fBigEndianHeader)) + pos;
	} else {
		toRead = fSize - pos;
		source = (reinterpret_cast<uint8 *>(fBitmap->Bits())) + pos -
			sizeof(TranslatorBitmap);
	}
	if (toRead > (ssize_t)size)
		toRead = (ssize_t)size;

	memcpy(buffer, source, toRead);
	return toRead;
}


// Writes data to the bitmap starting at a specific position and size.
ssize_t
BBitmapStream::WriteAt(off_t pos, const void* data, size_t size)
{
	if (size == 0)
		return B_OK;
	if (!data || pos < 0 || pos > (off_t)fSize)
		return B_BAD_VALUE;

	ssize_t written = 0;
	while (size > 0) {
		size_t toWrite;
		void *dest;
		// We depend on writing the header separately in detecting
		// changes to it
		if (pos < (off_t)sizeof(TranslatorBitmap)) {
			toWrite = sizeof(TranslatorBitmap) - pos;
			dest = (reinterpret_cast<uint8 *> (&fHeader)) + pos;
		} else {
			if (fBitmap == NULL || !fBitmap->IsValid())
				return B_ERROR;

			toWrite = fHeader.dataSize - pos + sizeof(TranslatorBitmap);
			dest = (reinterpret_cast<uint8 *> (fBitmap->Bits())) +
				pos - sizeof(TranslatorBitmap);
		}
		if (toWrite > size)
			toWrite = size;
		if (!toWrite && size)
			// i.e. we've been told to write too much
			return B_BAD_VALUE;

		memcpy(dest, data, toWrite);
		pos += toWrite;
		written += toWrite;
		data = (reinterpret_cast<const uint8 *> (data)) + toWrite;
		size -= toWrite;
		if (pos > (off_t)fSize)
			fSize = pos;
		// If we change the header, the rest needs to be reset
		if (pos == sizeof(TranslatorBitmap)) {
			// Setup both host and Big Endian byte order bitmap headers
			*fBigEndianHeader = fHeader;
			if (B_HOST_IS_LENDIAN)
				SwapHeader(fBigEndianHeader, &fHeader);

			if (fBitmap != NULL
				&& (fBitmap->Bounds() != fHeader.bounds
					|| fBitmap->ColorSpace() != fHeader.colors
					|| (uint32)fBitmap->BytesPerRow() != fHeader.rowBytes)) {
				if (!fDetached)
					// if someone detached, we don't delete
					delete fBitmap;
				fBitmap = NULL;
			}
			if (fBitmap == NULL) {
				if (fHeader.bounds.left > 0.0 || fHeader.bounds.top > 0.0)
					DEBUGGER("non-origin bounds!");
				fBitmap = new (std::nothrow )BBitmap(fHeader.bounds,
					fHeader.colors);
				if (fBitmap == NULL)
					return B_ERROR;
				if (!fBitmap->IsValid()) {
					status_t error = fBitmap->InitCheck();
					delete fBitmap;
					fBitmap = NULL;
					return error;
				}
				if ((uint32)fBitmap->BytesPerRow() != fHeader.rowBytes) {
					fprintf(stderr, "BitmapStream %" B_PRId32 " %" B_PRId32 "\n",
						fBitmap->BytesPerRow(), fHeader.rowBytes);
					return B_MISMATCHED_VALUES;
				}
			}
			if (fBitmap != NULL)
				fSize = sizeof(TranslatorBitmap) + fBitmap->BitsLength();
		}
	}
	return written;
}


// Changes the current stream position.
off_t
BBitmapStream::Seek(off_t position, uint32 seekMode)
{
	// When whence == SEEK_SET, it just falls through to
	// fPosition = position
	if (seekMode == SEEK_CUR)
		position += fPosition;
	else if (seekMode == SEEK_END)
		position += fSize;

	if (position < 0 || position > (off_t)fSize)
		return B_BAD_VALUE;

	fPosition = position;
	return fPosition;
}


// Returns the current stream position
off_t
BBitmapStream::Position() const
{
	return fPosition;
}



// Returns the current stream size
off_t
BBitmapStream::Size() const
{
	return fSize;
}


// Sets the size of the data.
// I'm not sure if this method has any real purpose.
status_t
BBitmapStream::SetSize(off_t size)
{
	if (size < 0)
		return B_BAD_VALUE;
	if (fBitmap && (size > (off_t)(fHeader.dataSize + sizeof(TranslatorBitmap))))
		return B_BAD_VALUE;
	// Problem:
	// What if someone calls SetSize() before writing the header,
	// so we don't know what bitmap to create?
	// Solution:
	// We assume people will write the header before any data,
	// so SetSize() is really not going to do anything.
	if (fBitmap != NULL)
		fSize = size;

	return B_NO_ERROR;
}


// Sets _bitmap to point to the internal bitmap object.
status_t
BBitmapStream::DetachBitmap(BBitmap** _bitmap)
{
	if (_bitmap == NULL)
		return B_BAD_VALUE;
	if (!fBitmap || fDetached)
		return B_ERROR;

	fDetached = true;
	*_bitmap = fBitmap;

	return B_OK;
}


// Swaps the byte order of source, no matter the byte order, and
// copies the result to destination.
void
BBitmapStream::SwapHeader(const TranslatorBitmap* source,
	TranslatorBitmap* destination)
{
	if (source == NULL || destination == NULL)
		return;

	*destination = *source;
	swap_data(B_UINT32_TYPE, destination, sizeof(TranslatorBitmap),
		B_SWAP_ALWAYS);
}


void BBitmapStream::_ReservedBitmapStream1() {}
void BBitmapStream::_ReservedBitmapStream2() {}
