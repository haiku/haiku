/*****************************************************************************/
//               File: BitmapStream.h
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

#if !defined(_BITMAP_STREAM_H)
#define _BITMAP_STREAM_H

#include <BeBuild.h>
#include <TranslationDefs.h>
#include <DataIO.h>
#include <TranslatorFormats.h>
#include <ByteOrder.h>

class BBitmap;

class BBitmapStream : public BPositionIO {
public:
	BBitmapStream(BBitmap *bitmap = NULL);
		// Initializes the stream to use map as the bitmap for stream
		// operations. If map is null, a BBitmap is created when properly
		// formatted data is written to the stream.
		
	~BBitmapStream();
		// Destroys the BBitmap held by this object if it has not been
		// detached and frees fpBigEndianHeader

	// Overrides from BPositionIO
	ssize_t ReadAt(off_t pos, void *buffer, size_t size);
		// reads data from the bitmap into buffer
	ssize_t WriteAt(off_t pos, const void *data, size_t size);
		// writes the data from data into this bitmap
		// (if the data is properly formatted)
	off_t Seek(off_t position, uint32 whence);
		// sets the stream position member
	off_t Position() const;
		// returns the current stream position
	off_t Size() const;
		// returns the size of the data
	status_t SetSize(off_t size);
		// sets the amount of memory to be used by this object

	status_t DetachBitmap(BBitmap **outMap);
		// "Detaches" the bitmap used by this stream and returns it
		// to the user through outMap. This allows the user to
		// use the bitmap even after the BBitmapStream that it
		// came from has been deleted.

protected:
	TranslatorBitmap fHeader;
		// stores the bitmap header information in the
		// host format, used to determine the format of
		// the bitmap data and to see if the data is valid
	BBitmap *fBitmap;
		// the actual bitmap used by this object
	size_t fPosition;
		// current data position
	size_t fSize;
		// size of the data stored
	bool fDetached;
		// true if the bitmap has been detached, false if not

	static status_t ConvertBEndianToHost(TranslatorBitmap *);
		// Converts a TranslatorBitmap struct from the Big Endian
		// format to the host format. This is needed because
		// the headers are always stored in the Big Endian format
		// when read in from or written out to files.
	status_t SetBigEndianHeader();
		// Sets fpBigEndianHeader to be the Big Endian version
		// of the data in fHeader. I need a Big Endian copy
		// of fHeader for the ReadAt() function.
	
private:
	TranslatorBitmap *fpBigEndianHeader;
		// same data as in fHeader, but in Big Endian format
		// (Intel machines are Little Endian)
	
	// For maintaining binary compatibility with past and future
	// versions of this object
	long fUnused[5];
	virtual	void _ReservedBitmapStream1();
	virtual void _ReservedBitmapStream2();
};

#endif /* _BITMAP_STREAM_H */

