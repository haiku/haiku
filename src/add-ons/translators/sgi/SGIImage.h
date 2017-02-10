/*****************************************************************************/
// SGITranslator
// Written by Stephan Aßmus <stippi@yellowbites.com>
// derived from GIMP SGI plugin by Michael Sweet
//
// SGIImage.h
//
// SGI image file format library routines.
//
// Formed into a class SGIImage, adopted to Be API and modified to use
// BPositionIO, optimizations for buffered reading.
//
//
// Copyright (c) 2003 OpenBeOS Project
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

/*
 * "$Id: SGIImage.h 14449 2005-10-20 12:15:56Z stippi $"
 *
 * Revision History:
 *
 *	 $Log: SGIImage.h,v $
 *	 Revision 1.2  2004/02/03 00:52:18  mwilber
 *	 Removed GPL text as permission was obtained from Michael Sweet to allow this derivative work to be distributed under the MIT License.
 *
 *	 Revision 1.1  2004/02/02 23:55:38  mwilber
 *	 Initial check in for Stephan Assmus' SGITranslator
 *	
 *	 Revision 1.5	1998/05/17 16:01:33	mike
 *	 Added <unistd.h> header file.
 *
 *	 Revision 1.4	1998/04/23	17:40:49	mike
 *	 Updated to support 16-bit <unsigned> image data.
 *
 *	 Revision 1.3	1998/02/05	17:10:58	mike
 *	 Added sgiOpenFile() function for opening an existing file pointer.
 *
 *	 Revision 1.2	1997/06/18	00:55:28	mike
 *	 Updated to hold length table when writing.
 *	 Updated to hold current length when doing ARLE.
 *
 *	 Revision 1.1	1997/06/15	03:37:19	mike
 *	 Initial revision
 */

#ifndef SGI_IMAGE_H
#define SGI_IMAGE_H

#include <DataIO.h>
#include <InterfaceDefs.h>

#define SGI_MAGIC		474	// magic number in image file

#define SGI_READ		0	// read from an SGI image file
#define SGI_WRITE		1	// write to an SGI image file

#define SGI_COMP_NONE	0	// no compression
#define SGI_COMP_RLE	1	// run-length encoding
#define SGI_COMP_ARLE	2	// agressive run-length encoding

extern const char kSGICopyright[];

class SGIImage {
 public:
								SGIImage();
	virtual						~SGIImage();

			// not really necessary, SetTo() will return an error anyways
			status_t			InitCheck() const;

			// first version -> read from an existing sgi image in stream
			status_t			SetTo(BPositionIO* stream);
			// second version -> set up a stream for writing an sgi image;
			// when SetTo() returns, the image header will have been written
			// already
			status_t			SetTo(BPositionIO* stream,
									  uint16 width, uint16 height,
									  uint16 channels, uint32 bytesPerChannel,
									  uint32 compression);
			// has to be called if writing, writes final information to the stream
			status_t			Unset();

			// access to each row of image data
			status_t			ReadRow(void* row, int32 lineNum, int32 channel);
			// write one row of image data
			// right now, could be used to modify an image in place, but only
			// if dealing with uncompressed data, compressed data is currently
			// not supported
			status_t			WriteRow(void* row, int32 lineNum, int32 channel);

			// access to the attributes of the sgi image
			uint16				Width() const
									{ return fWidth; }
			uint16				Height() const
									{ return fHeight; }
			uint32				BytesPerChannel() const
									{ return fBytesPerChannel; }
			uint32				CountChannels() const
									{ return fChannelCount; }

 private:
			int32				_ReadLong() const;
			int16				_ReadShort() const;
			int8				_ReadChar() const;
			status_t			_WriteLong(int32 n) const;
			status_t			_WriteShort(uint16 n) const;
			status_t			_WriteChar(int8 n) const;

			ssize_t				_ReadRLE8(uint8* row, int32 numPixels) const;
			ssize_t				_ReadRLE8(uint8* row, uint8* rleBuffer, int32 numPixels) const;
			ssize_t				_ReadRLE16(uint16* row, int32 numPixels) const;
			ssize_t				_ReadRLE16(uint16* row, uint16* rleBuffer, int32 numPixels) const;
			ssize_t				_WriteRLE8(uint8* row, int32 numPixels) const;
			ssize_t				_WriteRLE16(uint16* row, int32 numPixels) const;


	BPositionIO*				fStream;

	uint32						fMode;				// reading or writing
	uint32						fBytesPerChannel;
	uint32						fCompression;

	uint16						fWidth;				// in number of pixels
	uint16						fHeight;			// in number of pixels
	uint16						fChannelCount;

	off_t						fFirstRowOffset;	// offset into stream
	off_t						fNextRowOffset;		// offset into stream

	int32**						fOffsetTable;		// offset table for compression
	int32**						fLengthTable;		// length table for compression

	uint16*						fARLERow;			// advanced RLE compression buffer
	int32						fARLEOffset;		// advanced RLE buffer offset
	int32						fARLELength;		// advanced RLE buffer length
};

#endif // SGI_IMAGE_H

