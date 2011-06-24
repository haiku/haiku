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


/*!	Initializes this object to either use the BBitmap passed to
	it as the object to read/write to or to create a BBitmap
	when data is written to this object.

	\param bitmap the bitmap used to read from/write to, if it is NULL, a
		bitmap is created when this object is written to.
*/
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
			memcpy(fBigEndianHeader, &fHeader, sizeof(TranslatorBitmap));
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


/*!	Reads data from the stream at a specific position and for a
	specific amount. The first sizeof(TranslatorBitmap) bytes
	are the bitmap header. The header is always written out
	and read in as Big Endian byte order.

	\param pos, the position in the stream to read from buffer, where the data
		will be read into size, the amount of data to read
	\return B_ERROR if there is no bitmap stored by the stream, B_BAD_VALUE if
		buffer is NULL or pos is invalid or the amount read if the result >= 0
*/
ssize_t
BBitmapStream::ReadAt(off_t pos, void* buffer, size_t size)
{
	if (fBitmap == NULL)
		return B_NO_INIT;
	if (size == 0)
		return B_OK;
	if (pos >= fSize || pos < 0 || buffer == NULL)
		return B_BAD_VALUE;

	ssize_t toRead;
	void *source;

	if (pos < sizeof(TranslatorBitmap)) {
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


/*!	Writes data to the bitmap from data, starting at position pos
	of size, size. The first sizeof(TranslatorBitmap) bytes
	of data must be the TranslatorBitmap header in the Big
	Endian byte order, otherwise, the data will fail to be
	successfully written.

	\param pos, the position in the stream to write to data, the data to write
		to the stream size, the size of the data to write to the stream
	\return B_BAD_VALUE if size is bad or data is NULL or pos is invalid,
		B_MISMATCHED_VALUES if the bitmap header is bad,
		B_ERROR if error allocating memory or setting up big endian header,
		or the amount written if the result is >= 0
*/
ssize_t
BBitmapStream::WriteAt(off_t pos, const void* data, size_t size)
{
	if (size == 0)
		return B_OK;
	if (!data || pos < 0 || pos > fSize)
		return B_BAD_VALUE;

	ssize_t written = 0;
	while (size > 0) {
		size_t toWrite;
		void *dest;
		// We depend on writing the header separately in detecting
		// changes to it
		if (pos < sizeof(TranslatorBitmap)) {
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
		if (pos > fSize)
			fSize = pos;
		// If we change the header, the rest needs to be reset
		if (pos == sizeof(TranslatorBitmap)) {
			// Setup both host and Big Endian byte order bitmap headers
			memcpy(fBigEndianHeader, &fHeader, sizeof(TranslatorBitmap));
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
					fprintf(stderr, "BitmapStream %ld %ld\n",
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


/*!	Changes the current stream position.

	\param position the position offset
	\param seekMode decides how the position offset is used:
		SEEK_CUR, position is added to current stream position
		SEEK_END, position is added to the end stream position
		SEEK_SET, the stream position is set to position
	\return B_BAD_VALUE if the position is bad or the new position value if the
		result >= 0
*/
off_t
BBitmapStream::Seek(off_t position, uint32 seekMode)
{
	// When whence == SEEK_SET, it just falls through to
	// fPosition = position
	if (seekMode == SEEK_CUR)
		position += fPosition;
	else if (seekMode == SEEK_END)
		position += fSize;

	if (position < 0 || position > fSize)
		return B_BAD_VALUE;

	fPosition = position;
	return fPosition;
}


/*! Returns the current stream position
*/
off_t
BBitmapStream::Position() const
{
	return fPosition;
}



/*! Returns the curren stream size
*/
off_t
BBitmapStream::Size() const
{
	return fSize;
}


/*!	Sets the size of the data, but I'm not sure if this function has any real
	purpose.

	\param size the size to set the stream size to.
	\return B_BAD_VALUE, if size is a bad value, B_NO_ERROR, if size is a
		valid value
*/
status_t
BBitmapStream::SetSize(off_t size)
{
	if (size < 0)
		return B_BAD_VALUE;
	if (fBitmap && (size > fHeader.dataSize + sizeof(TranslatorBitmap)))
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


/*!	Returns the internal bitmap through outBitmap so the user
	can do whatever they want to it. It means that when the
	BBitmapStream is deleted, the bitmap is not deleted. After
	the bitmap has been detached, it is still used by the stream,
	but it is never deleted by the stream.

	\param _bitmap where the bitmap is detached to
	\return B_BAD_VALUE, if outBitmap is NULL, B_ERROR, if the bitmap is NULL or
		has already been detached, B_OK, if the bitmap was successfully detached
*/
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


/*!	Swaps the byte order of source, no matter what the
	byte order, and copies the result to destination

	\param source data to be swapped
	\param destination where the swapped data will be copied to
*/
void
BBitmapStream::SwapHeader(const TranslatorBitmap* source,
	TranslatorBitmap* destination)
{
	if (source == NULL || destination == NULL)
		return;

	memcpy(destination, source, sizeof(TranslatorBitmap));
	swap_data(B_UINT32_TYPE, destination, sizeof(TranslatorBitmap),
		B_SWAP_ALWAYS);
}


void BBitmapStream::_ReservedBitmapStream1() {}
void BBitmapStream::_ReservedBitmapStream2() {}
