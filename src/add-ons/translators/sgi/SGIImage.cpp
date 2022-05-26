/*****************************************************************************/
// SGITranslator
// Written by Stephan Aßmus <stippi@yellowbites.com>
// derived from GIMP SGI plugin by Michael Sweet
//
// SGIImage.cpp
//
// SGI image file format library routines.
//
// Formed into a class SGIImage, adopted to Be API and modified to use
// BPositionIO, optimizations for buffered reading.
//
//
// Copyright (c) 2003 Haiku Project
// Portions Copyright 1997-1998 Michael Sweet (mike@easysw.com)
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

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <ByteOrder.h>
#include <DataIO.h>
#include <TranslationErrors.h>

#include "SGIImage.h"

const char kSGICopyright[] = "" B_UTF8_COPYRIGHT " 1997-1998 Michael Sweet "
	"<mike@easysw.com>";

// constructor
SGIImage::SGIImage()
	: fStream(NULL),
	  fMode(0),
	  fBytesPerChannel(0),
	  fCompression(0),
	  fWidth(0),
	  fHeight(0),
	  fChannelCount(0),
	  fFirstRowOffset(0),
	  fNextRowOffset(0),
	  fOffsetTable(NULL),
	  fLengthTable(NULL),
	  fARLERow(NULL),
	  fARLEOffset(0),
	  fARLELength(0)
{
}

// destructor
SGIImage::~SGIImage()
{
	Unset();
}

// InitCheck
status_t
SGIImage::InitCheck() const
{
	if (fStream)
		return B_OK;
	return B_NO_INIT;
}

// SetTo
// open an SGI image file for reading
//
// stream	the input stream
status_t
SGIImage::SetTo(BPositionIO* stream)
{
	if (!stream)
		return B_BAD_VALUE;

	fStream = stream;
	stream->Seek(0, SEEK_SET);

	int16 magic = _ReadShort();
	if (magic != SGI_MAGIC) {
		fStream = NULL;
		return B_NO_TRANSLATOR;
	}

	fMode = SGI_READ;

	fCompression = _ReadChar();
	fBytesPerChannel = _ReadChar();
	_ReadShort();	// Dimensions
	fWidth = _ReadShort();
	fHeight = _ReadShort();
	fChannelCount = _ReadShort();
//	_ReadLong();	// Minimum pixel
//	_ReadLong();	// Maximum pixel

	if (fCompression) {
		// this stream is compressed; read the scanline tables...

		fStream->Seek(512, SEEK_SET);

		fOffsetTable	= (int32**)calloc(fChannelCount, sizeof(int32*));
		fOffsetTable[0] = (int32*)calloc(fHeight * fChannelCount, sizeof(int32));
		for (uint32 i = 1; i < fChannelCount; i++)
			fOffsetTable[i] = fOffsetTable[0] + i * fHeight;

		for (uint32 i = 0; i < fChannelCount; i++)
			for (uint16 j = 0; j < fHeight; j++)
				fOffsetTable[i][j] = _ReadLong();

		fLengthTable	= (int32**)calloc(fChannelCount, sizeof(int32*));
		fLengthTable[0] = (int32*)calloc(fHeight * fChannelCount, sizeof(int32));

		for (int32 i = 1; i < fChannelCount; i ++)
			fLengthTable[i] = fLengthTable[0] + i * fHeight;

		for (uint32 i = 0; i < fChannelCount; i++)
			for (uint16 j = 0; j < fHeight; j++)
				fLengthTable[i][j] = _ReadLong();

	}
	return B_OK;
}

// SetTo
// open an SGI image file for writing
//
// stream			the output stream
// width			number of pixels in a row
// height			number of rows
// channels			number of channels per pixel
// bytesPerChannel	number of bytes per channel
// compression		compression mode
status_t
SGIImage::SetTo(BPositionIO* stream,
				uint16 width, uint16 height,
				uint16 channels, uint32 bytesPerChannel,
				uint32 compression)
{
	// sanity checks
	if (!stream ||
		width < 1 || height < 1 || channels < 1 ||
		bytesPerChannel < 1 || bytesPerChannel > 2 ||
		compression < SGI_COMP_NONE || compression > SGI_COMP_ARLE)
		return B_BAD_VALUE;

	fStream = stream;
	fMode = SGI_WRITE;

	_WriteShort(SGI_MAGIC);
	_WriteChar((fCompression = compression) != 0);
	_WriteChar(fBytesPerChannel = bytesPerChannel);
	_WriteShort(3);		// Dimensions
	_WriteShort(fWidth = width);
	_WriteShort(fHeight = height);
	_WriteShort(fChannelCount = channels);

	if (fBytesPerChannel == 1) {
		_WriteLong(0);		// Minimum pixel
		_WriteLong(255);	// Maximum pixel
	} else {
		_WriteLong(-32768);	// Minimum pixel
		_WriteLong(32767);	// Maximum pixel
	}
	_WriteLong(0);			// Reserved

	char name[80];	// Name of file in image header
	memset(name, 0, sizeof(name));
	sprintf(name, "Haiku SGITranslator");
	fStream->Write(name, sizeof(name));

	// fill the rest of the image header with zeros
	for (int32 i = 0; i < 102; i++)
		_WriteLong(0);

	switch (fCompression) {
		case SGI_COMP_NONE : // No compression
			// This file is uncompressed.  To avoid problems with
			// sparse files, we need to write blank pixels for the
			// entire image...

/*			if (fBytesPerChannel == 1) {
				for (int32 i = fWidth * fHeight * fChannelCount; i > 0; i --)
					_WriteChar(0);
			} else {
				for (int32 i = fWidth * fHeight * fChannelCount; i > 0; i --)
					_WriteShort(0);
			}*/
			break;

		case SGI_COMP_ARLE: // Aggressive RLE
			fARLERow	= (uint16*)calloc(fWidth, sizeof(uint16));
			fARLEOffset = 0;
			// FALL THROUGH
		case SGI_COMP_RLE : // Run-Length Encoding
			// This file is compressed; write the (blank) scanline tables...

//			for (int32 i = 2 * fHeight * fChannelCount; i > 0; i--)
//				_WriteLong(0);
fStream->Seek(2 * fHeight * fChannelCount * sizeof(int32), SEEK_CUR);

			fFirstRowOffset = fStream->Position();
			fNextRowOffset  = fStream->Position();

			// allocate and read offset table
			fOffsetTable	= (int32**)calloc(fChannelCount, sizeof(int32*));
			fOffsetTable[0] = (int32*)calloc(fHeight * fChannelCount, sizeof(int32));

			for (int32 i = 1; i < fChannelCount; i ++)
				fOffsetTable[i] = fOffsetTable[0] + i * fHeight;

			// allocate and read length table
			fLengthTable	= (int32**)calloc(fChannelCount, sizeof(int32*));
			fLengthTable[0] = (int32*)calloc(fHeight * fChannelCount, sizeof(int32));

			for (int32 i = 1; i < fChannelCount; i ++)
				fLengthTable[i] = fLengthTable[0] + i * fHeight;
			break;
	}
	return B_OK;
}

// Unset
//
// if in write mode, writes final information to the stream
status_t
SGIImage::Unset()
{
	status_t ret = InitCheck(); // return status
	if (ret >= B_OK) {

		if (fMode == SGI_WRITE && fCompression != SGI_COMP_NONE) {
			// write the scanline offset table to the file...
		
			fStream->Seek(512, SEEK_SET);

/*			off_t* offset = fOffsetTable[0];
			for (int32 i = fHeight * fChannelCount; i > 0; i--) {
				if ((ret = _WriteLong(offset[0])) < B_OK)
					break;
				offset++;
			}*/

int32 size = fHeight * fChannelCount * sizeof(int32);
swap_data(B_INT32_TYPE, fOffsetTable[0], size, B_SWAP_HOST_TO_BENDIAN);
ret = fStream->Write(fOffsetTable[0], size);

			if (ret >= B_OK) {

/*				int32* length = fLengthTable[0];
				for (int32 i = fHeight * fChannelCount; i > 0; i--) {
					if ((ret = _WriteLong(length[0])) < B_OK)
						break;
					length++;
				}*/

swap_data(B_INT32_TYPE, fLengthTable[0], size, B_SWAP_HOST_TO_BENDIAN);
ret = fStream->Write(fLengthTable[0], size);

			}
		}
		
		if (fOffsetTable != NULL) {
			free(fOffsetTable[0]);
			free(fOffsetTable);
			fOffsetTable = NULL;
		}
		
		if (fLengthTable != NULL) {
			free(fLengthTable[0]);
			free(fLengthTable);
			fLengthTable = NULL;
		}
		
		if (fARLERow) {
			free(fARLERow);
			fARLERow = NULL;
		}

		fStream = NULL;
	}
	return ret;
}

// ReadRow
//
// reads a row of image data from the stream
//
// row			pointer to buffer (row of pixels) to read
// y			index (line number) of this row
// z			which channel to read
status_t
SGIImage::ReadRow(void* row, int32 y, int32 z)
{
	// sanitiy checks
	if (row == NULL ||
		y < 0 || y >= fHeight ||
		z < 0 || z >= fChannelCount)
		return B_BAD_VALUE;


	status_t ret = B_ERROR;

	switch (fCompression) {
		case SGI_COMP_NONE: {
			// seek to the image row
			// optimize buffering by only seeking if necessary...

			off_t offset = 512 + (y + z * fHeight) * fWidth * fBytesPerChannel;
			fStream->Seek(offset, SEEK_SET);

			uint32 bytes = fWidth * fBytesPerChannel;
//printf("reading %ld bytes 8 Bit uncompressed row: %ld, channel: %ld\n", bytes, y, z);
			ret = fStream->Read(row, bytes);

			break;
		}
		case SGI_COMP_RLE: {
			int32 offset = fOffsetTable[z][y];
			int32 rleLength = fLengthTable[z][y];
			fStream->Seek(offset, SEEK_SET);
			uint8* rleBuffer = new uint8[rleLength];
			fStream->Read(rleBuffer, rleLength);

			if (fBytesPerChannel == 1) {
//printf("reading 8 Bit RLE compressed row: %ld, channel: %ld\n", y, z);
//				ret = _ReadRLE8((uint8*)row, fWidth);
				ret = _ReadRLE8((uint8*)row, rleBuffer, fWidth);
			} else {
//printf("reading 16 Bit RLE compressed row: %ld, channel: %ld\n", y, z);
//				ret = _ReadRLE16((uint16*)row, fWidth);
				if ((ret = swap_data(B_INT16_TYPE, rleBuffer, rleLength, B_SWAP_BENDIAN_TO_HOST)) >= B_OK)
					ret = _ReadRLE16((uint16*)row, (uint16*)rleBuffer, fWidth);
			}
			delete[] rleBuffer;
			break;
		}
	}
	return ret;
}

// WriteRow
//
// writes a row of image data to the stream
//
// row			pointer to buffer (row of pixels) to write
// y			index (line number) of this row
// z			which channel to write
status_t
SGIImage::WriteRow(void* row, int32 y, int32 z)
{
	// sanitiy checks
	if (row == NULL ||
		y < 0 || y >= fHeight ||
		z < 0 || z >= fChannelCount)
		return B_BAD_VALUE;

	int32 x;		// x coordinate
	int32 offset;	// stream offset

	status_t ret = B_ERROR;

	switch (fCompression) {
		case SGI_COMP_NONE: {
			// Seek to the image row

			offset = 512 + (y + z * fHeight) * fWidth * fBytesPerChannel;
			fStream->Seek(offset, SEEK_SET);

			uint32 bytes = fWidth * fBytesPerChannel;
//printf("writing %ld bytes %ld byte/channel uncompressed row: %ld, channel: %ld\n", bytes, fBytesPerChannel, y, z);
			ret = fStream->Write(row, bytes);
/*			if (fBytesPerChannel == 1) {
				for (x = fWidth; x > 0; x--) {
					_WriteChar(*row);
					row++;
				}
			} else {
				for (x = fWidth; x > 0; x--) {
					_WriteShort(*row);
					row++;
				}
			}*/
			break;
		}
		case SGI_COMP_ARLE:
			if (fOffsetTable[z][y] != 0)
				return B_ERROR;

			// First check the last row written...

			if (fARLEOffset > 0) {
				if (fBytesPerChannel == 1) {
					uint8* arleRow = (uint8*)fARLERow;
					uint8* src = (uint8*)row;
					for (x = 0; x < fWidth; x++)
						if (*src++ != *arleRow++)
							break;
				} else {
					uint16* arleRow = (uint16*)fARLERow;
					uint16* src = (uint16*)row;
					for (x = 0; x < fWidth; x++)
						if (*src++ != *arleRow++)
							break;
				}

				if (x == fWidth) {
					fOffsetTable[z][y] = fARLEOffset;
					fLengthTable[z][y] = fARLELength;
					return B_OK;
				}
			}

			// If that didn't match, search all the previous rows...

			fStream->Seek(fFirstRowOffset, SEEK_SET);

			if (fBytesPerChannel == 1) {
				do {
					fARLEOffset = fStream->Position();

					uint8* arleRow = (uint8*)fARLERow;
					if ((fARLELength = _ReadRLE8(arleRow, fWidth)) < B_OK) {
						x = 0;
						break;
					}

					uint8* src = (uint8*)row;
					for (x = 0; x < fWidth; x++)
						if (*src++ != *arleRow++)
							break;
				} while (x < fWidth);
			} else {
				do {
					fARLEOffset = fStream->Position();

					uint16* arleRow = (uint16*)fARLERow;
					if ((fARLELength = _ReadRLE16(arleRow, fWidth)) < B_OK) {
						x = 0;
						break;
					}

					uint16* src = (uint16*)row;
					for (x = 0; x < fWidth; x++)
						if (*src++ != *arleRow++)
							break;
				} while (x < fWidth);
			}

			if (x == fWidth) {
				fOffsetTable[z][y] = fARLEOffset;
				fLengthTable[z][y] = fARLELength;
				return B_OK;
			} else
				fStream->Seek(0, SEEK_END);	// seek to end of stream
			// FALL THROUGH!
		case SGI_COMP_RLE :
			if (fOffsetTable[z][y] != 0)
				return B_ERROR;

			offset = fOffsetTable[z][y] = fNextRowOffset;

			if (offset != fStream->Position())
				fStream->Seek(offset, SEEK_SET);

//printf("writing %d pixels %ld byte/channel RLE row: %ld, channel: %ld\n", fWidth, fBytesPerChannel, y, z);

			if (fBytesPerChannel == 1)
				x = _WriteRLE8((uint8*)row, fWidth);
			else
				x = _WriteRLE16((uint16*)row, fWidth);

			if (fCompression == SGI_COMP_ARLE) {
				fARLEOffset = offset;
				fARLELength = x;
				memcpy(fARLERow, row, fWidth * fBytesPerChannel);
			}

			fNextRowOffset = fStream->Position();
			fLengthTable[z][y] = x;

			return x;
		default:
			break;
	}

	return ret;
}

// _ReadLong
//
// reads 4 bytes from the stream and
// returns a 32-bit big-endian integer
int32
SGIImage::_ReadLong() const
{
	int32 n;
	if (fStream->Read(&n, 4) == 4) {
		return B_BENDIAN_TO_HOST_INT32(n);
	} else
		return 0;
}

// _ReadShort
//
// reads 2 bytes from the stream and
// returns a 16-bit big-endian integer
int16
SGIImage::_ReadShort() const
{
	int16 n;
	if (fStream->Read(&n, 2) == 2) {
		return B_BENDIAN_TO_HOST_INT16(n);
	} else
		return 0;
}

// _ReadChar
//
// reads 1 byte from the stream and
// returns it
int8
SGIImage::_ReadChar() const
{
	int8 b;
	ssize_t read = fStream->Read(&b, 1);
	if (read == 1)
		return b;
	else if (read < B_OK)
		return (int8)read;
	return (int8)B_ERROR;
}

// _WriteLong
//
// writes a 32-bit big-endian integer to the stream
status_t
SGIImage::_WriteLong(int32 n) const
{
	int32 bigN = B_HOST_TO_BENDIAN_INT32(n);
	ssize_t written = fStream->Write(&bigN, sizeof(int32));
	if (written == sizeof(int32))
		return B_OK;
	if (written < B_OK)
		return written;
	return B_ERROR;
}

// _WriteShort
//
// writes a 16-bit big-endian integer to the stream
status_t
SGIImage::_WriteShort(uint16 n) const
{
	uint16 bigN = B_HOST_TO_BENDIAN_INT16(n);
	ssize_t written = fStream->Write(&bigN, sizeof(uint16));
	if (written == sizeof(uint16))
		return B_OK;
	if (written < B_OK)
		return written;
	return B_ERROR;
}

// _WriteChar
//
// writes one byte to the stream
status_t
SGIImage::_WriteChar(int8 n) const
{
	ssize_t written = fStream->Write(&n, sizeof(int8));
	if (written == sizeof(int8))
		return B_OK;
	if (written < B_OK)
		return written;
	return B_ERROR;
}

// _ReadRLE8
//
// reads 8-bit RLE data into provided buffer
//
// row			pointer to buffer for one row
// numPixels	number of pixels that fit into row buffer
ssize_t
SGIImage::_ReadRLE8(uint8* row, int32 numPixels) const
{
	int32 ch;			// current charater
	uint32 count;		// RLE count
	uint32 length = 0;	// number of bytes read

	uint32 bufferSize = 1024;
	uint8* buffer = new uint8[bufferSize];
	uint32 bufferPos = bufferSize;

	status_t ret = B_OK;

	while (numPixels > 0) {

		// fetch another buffer if we need to
		if (bufferPos >= bufferSize) {
			ret = fStream->Read(buffer, bufferSize);
			if (ret < B_OK)
				break;
			else
				bufferPos = 0;
		}

		ch = buffer[bufferPos ++];
		length ++;

		count = ch & 127;
		if (count == 0)
			break;

		if (ch & 128) {
			for (uint32 i = 0; i < count; i++) {

				// fetch another buffer if we need to
				if (bufferPos >= bufferSize) {
					ret = fStream->Read(buffer, bufferSize);
					if (ret < B_OK) {
						delete[] buffer;
						return ret;
					} else
						bufferPos = 0;
				}

				*row = buffer[bufferPos ++];
				row ++;
				numPixels --;
				length ++;
			}
		} else {

			// fetch another buffer if we need to
			if (bufferPos >= bufferSize) {
				ret = fStream->Read(buffer, bufferSize);
				if (ret < B_OK) {
					delete[] buffer;
					return ret;
				} else
					bufferPos = 0;
			}

			ch = buffer[bufferPos ++];
			length ++;
			for (uint32 i = 0; i < count; i++) {
				*row = ch;
				row ++;
				numPixels --;
			}
		}
	}
	delete[] buffer;

	return (numPixels > 0 ? ret : length);
}

// _ReadRLE8
//
// reads 8-bit RLE data into provided buffer
//
// row			pointer to buffer for one row
// numPixels	number of pixels that fit into row buffer
ssize_t
SGIImage::_ReadRLE8(uint8* row, uint8* rleBuffer, int32 numPixels) const
{
	int32 ch;			// current charater
	uint32 count;		// RLE count
	uint32 length = 0;	// number of bytes read

	if (numPixels <= 0)
		return B_ERROR;

	while (numPixels > 0) {

		ch = *rleBuffer ++;
		length ++;

		count = ch & 127;
		if (count == 0)
			break;

		if (ch & 128) {
			for (uint32 i = 0; i < count; i++) {

				*row = *rleBuffer ++;
				row ++;
				numPixels --;
				length ++;
			}
		} else {

			ch = *rleBuffer ++;
			length ++;
			for (uint32 i = 0; i < count; i++) {
				*row = ch;
				row ++;
				numPixels --;
			}
		}
	}

	return length;
}
/*ssize_t
SGIImage::_ReadRLE8(uint8* row, int32 numPixels) const
{
	int32 ch;			// current charater
	uint32 count;		// RLE count
	uint32 length = 0;	// number of bytes read

	while (numPixels > 0) {
		ch = _ReadChar();
		length ++;

		count = ch & 127;
		if (count == 0)
			break;

		if (ch & 128) {
			for (uint32 i = 0; i < count; i++) {
				*row = _ReadChar();
				row ++;
				numPixels --;
				length ++;
			}
		} else {
			ch = _ReadChar();
			length ++;
			for (uint32 i = 0; i < count; i++) {
				*row = ch;
				row ++;
				numPixels --;
			}
		}
	}
	return (numPixels > 0 ? B_ERROR : length);
}*/

// read_and_swap
status_t
read_and_swap(BPositionIO* stream, int16* buffer, uint32 size)
{
	status_t ret = stream->Read(buffer, size);
	if (ret >= B_OK)
		return swap_data(B_INT16_TYPE, buffer, ret, B_SWAP_BENDIAN_TO_HOST);
	return ret;
}

// _ReadRLE16
//
// reads 16-bit RLE data into provided buffer
//
// row			pointer to buffer for one row
// numPixels	number of pixels that fit into row buffer
ssize_t
SGIImage::_ReadRLE16(uint16* row, int32 numPixels) const
{
	int32 ch;			// current character
	uint32 count;		// RLE count
	uint32 length = 0;	// number of bytes read...

	uint32 bufferSize = 1024;
	int16* buffer = new int16[bufferSize];
	uint32 bufferPos = bufferSize;
	status_t ret = B_OK;

	while (numPixels > 0) {

		// fetch another buffer if we need to
		if (bufferPos >= bufferSize) {
			ret = read_and_swap(fStream, buffer, bufferSize * 2);
			if (ret < B_OK)
				break;
			bufferPos = 0;
		}

		ch = buffer[bufferPos ++];
		length ++;

		count = ch & 127;
		if (count == 0)
			break;

		if (ch & 128) {
			for (uint32 i = 0; i < count; i++) {

				// fetch another buffer if we need to
				if (bufferPos >= bufferSize) {
					ret = read_and_swap(fStream, buffer, bufferSize * 2);
					if (ret < B_OK) {
						delete[] buffer;
						return ret;
					} else
						bufferPos = 0;
				}

				*row = B_HOST_TO_BENDIAN_INT16(buffer[bufferPos ++]);
				row++;
				numPixels--;
				length++;
			}
		} else {

			// fetch another buffer if we need to
			if (bufferPos >= bufferSize) {
				ret = read_and_swap(fStream, buffer, bufferSize * 2);
				if (ret < B_OK) {
					delete[] buffer;
					return ret;
				} else
					bufferPos = 0;
			}

			ch = B_HOST_TO_BENDIAN_INT16(buffer[bufferPos ++]);
			length ++;
			for (uint32 i = 0; i < count; i++) {
				*row = ch;
				row++;
				numPixels--;
			}
		}
	}
	delete[] buffer;
	return (numPixels > 0 ? ret : length * 2);
}

// _ReadRLE16
//
// reads 16-bit RLE data into provided buffer
//
// row			pointer to buffer for one row
// numPixels	number of pixels that fit into row buffer
ssize_t
SGIImage::_ReadRLE16(uint16* row, uint16* rleBuffer, int32 numPixels) const
{
	int32 ch;			// current character
	uint32 count;		// RLE count
	uint32 length = 0;	// number of bytes read...

	if (numPixels <= 0)
		return B_ERROR;

	while (numPixels > 0) {

		ch = *rleBuffer ++;
		length ++;

		count = ch & 127;
		if (count == 0)
			break;

		if (ch & 128) {
			for (uint32 i = 0; i < count; i++) {

				*row = B_HOST_TO_BENDIAN_INT16(*rleBuffer ++);
				row++;
				numPixels--;
				length++;
			}
		} else {

			ch = B_HOST_TO_BENDIAN_INT16(*rleBuffer ++);
			length ++;
			for (uint32 i = 0; i < count; i++) {
				*row = ch;
				row++;
				numPixels--;
			}
		}
	}
	return length * 2;
}

// _WriteRLE8
//
// writes 8-bit RLE data into the stream
//
// row			pointer to buffer for one row
// numPixels	number of pixels that fit into row buffer
ssize_t
SGIImage::_WriteRLE8(uint8* row, int32 numPixels) const
{
	int32 length = 0;	// length of output line
	int32 count;		// number of repeated/non-repeated pixels
	int32 i;			// looping var
	uint8* start;		// start of sequence
	uint16 repeat;		// repeated pixel


	for (int32 x = numPixels; x > 0;) {
		start = row;
		row   += 2;
		x	 -= 2;

		while (x > 0 && (row[-2] != row[-1] || row[-1] != row[0])) {
			row++;
			x--;
		}

		row -= 2;
		x   += 2;

		count = row - start;
		while (count > 0) {
			i	 = count > 126 ? 126 : count;
			count -= i;

			if (_WriteChar(128 | i) == EOF)
				return EOF;
			length ++;

			while (i > 0) {
				if (_WriteChar(*start) == EOF)
					return EOF;
				start ++;
				i --;
				length ++;
			}
		}

		if (x <= 0)
			break;

		start  = row;
		repeat = row[0];

		row ++;
		x --;

		while (x > 0 && *row == repeat) {
			row ++;
			x --;
		}

		count = row - start;
		while (count > 0) {
			i	 = count > 126 ? 126 : count;
			count -= i;

			if (_WriteChar(i) == EOF)
				return EOF;
			length ++;
	
			if (_WriteChar(repeat) == EOF)
				return (-1);
			length ++;
		}
	}

	length ++;

	if (_WriteChar(0) == EOF)
		return EOF;
	else
		return length;
}


// _WriteRLE16
//
// writes 16-bit RLE data into the stream
//
// row			pointer to buffer for one row
// numPixels	number of pixels that fit into row buffer
ssize_t
SGIImage::_WriteRLE16(uint16* row, int32 numPixels) const
{
	int32 length = 0;	// length of output line
	int32 count;		// number of repeated/non-repeated pixels
	int32 i;			// looping var
	int32 x;			// looping var
	uint16* start;		// start of sequence
	uint16 repeat;		// repeated pixel


	for (x = numPixels; x > 0;) {
		start = row;
		row   += 2;
		x	 -= 2;

		while (x > 0 && (row[-2] != row[-1] || row[-1] != row[0])) {
			row ++;
			x --;
		}

		row -= 2;
		x   += 2;

		count = row - start;
		while (count > 0) {
			i	 = count > 126 ? 126 : count;
			count -= i;

			if (_WriteShort(128 | i) == EOF)
				return EOF;
			length ++;
	
			while (i > 0) {
				if (_WriteShort(*start) == EOF)
					return EOF;
				start ++;
				i --;
				length ++;
			}
		}

		if (x <= 0)
			break;

		start  = row;
		repeat = row[0];

		row ++;
		x --;

		while (x > 0 && *row == repeat) {
			row ++;
			x --;
		}

		count = row - start;
		while (count > 0) {
			i	 = count > 126 ? 126 : count;
			count -= i;

			if (_WriteShort(i) == EOF)
				return EOF;
			length ++;

			if (_WriteShort(repeat) == EOF)
				return EOF;
			length ++;
		}
	}

	length ++;

	if (_WriteShort(0) == EOF)
		return EOF;
	else
		return (2 * length);
}


