/*
 * "$Id: SGIImage.h,v 1.1 2004/02/02 23:55:38 mwilber Exp $"
 *
 *	 SGI image file format library definitions.
 *
 *	 Copyright 1997-1998 Michael Sweet (mike@easysw.com)
 *
 *	 This program is free software; you can redistribute it and/or modify it
 *	 under the terms of the GNU General Public License as published by the Free
 *	 Software Foundation; either version 2 of the License, or (at your option)
 *	 any later version.
 *
 *	 This program is distributed in the hope that it will be useful, but
 *	 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *	 or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU General Public License
 *	 for more details.
 *
 *	 You should have received a copy of the GNU General Public License
 *	 along with this program; if not, write to the Free Software
 *	 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Revision History:
 *
 *	 $Log: SGIImage.h,v $
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

