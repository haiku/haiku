/*****************************************************************************/
// TiffField
// Written by Michael Wilber, OBOS Translation Kit Team
//
// TiffField.h
//
// This object is for storing TIFF fields
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

#ifndef TIFF_FIELD_H
#define TIFF_FIELD_H

#include <SupportDefs.h>

// TIFF field id numbers for fields that the
// TIFFTranslator needs to know about / use
// (not a complete list of all tags)
#define TAG_NEW_SUBFILE_TYPE				254
	#define NEW_SUBFILE_TYPE_REDUCED	1
	#define NEW_SUBFILE_TYPE_PAGE		2
	#define NEW_SUBFILE_TYPE_MASK		4
#define TAG_SUBFILE_TYPE					255
	#define SUBFILE_TYPE_FULL			1
	#define SUBFILE_TYPE_REDUCED		2
	#define SUBFILE_TYPE_PAGE			3
#define TAG_IMAGE_WIDTH						256
#define TAG_IMAGE_HEIGHT					257
#define TAG_BITS_PER_SAMPLE					258
#define TAG_COMPRESSION						259
	#define COMPRESSION_NONE			1
	#define COMPRESSION_HUFFMAN			2
	#define COMPRESSION_T4				3
	#define COMPRESSION_T6				4
	#define COMPRESSION_LZW				5
	#define COMPRESSION_JPEG			6
	#define COMPRESSION_PACKBITS		32773
#define TAG_PHOTO_INTERPRETATION			262
	#define PHOTO_WHITEZERO				0
	#define PHOTO_BLACKZERO				1
	#define PHOTO_RGB					2
	#define PHOTO_PALETTE				3
	#define PHOTO_MASK					4
	#define PHOTO_SEPARATED				5
#define TAG_FILL_ORDER						266
#define TAG_STRIP_OFFSETS					273
#define TAG_ORIENTATION						274
#define TAG_SAMPLES_PER_PIXEL				277
#define TAG_ROWS_PER_STRIP					278
	#define DEFAULT_ROWS_PER_STRIP		4294967295UL
#define TAG_STRIP_BYTE_COUNTS				279
#define TAG_PLANAR_CONFIGURATION			284
#define TAG_T4_OPTIONS						292
	// (Also called Group3Options)
	#define T4_2D						1
	#define T4_UNCOMPRESSED				2
	#define T4_FILL_BYTE_BOUNDARY		4
#define TAG_T6_OPTIONS						293
	// (Also called Group4Options)
#define TAG_RESOLUTION_UNIT					296
#define TAG_COLOR_MAP						320
#define TAG_INK_SET							332
	#define INK_SET_CMYK				1
	#define INK_SET_NOT_CMYK			2
#define TAG_EXTRA_SAMPLES					338
#define TAG_SAMPLE_FORMAT					339

// Some types of data that can
// exist in a TIFF field
enum TIFF_ENTRY_TYPE {
	TIFF_BYTE = 1,
	TIFF_ASCII,
	TIFF_SHORT,
	TIFF_LONG,
	TIFF_RATIONAL,
	TIFF_SBYTE,
	TIFF_UNDEFINED,
	TIFF_SSHORT,
	TIFF_SRATIONAL,
	TIFF_FLOAT,
	TIFF_DOUBLE
};

struct IFDEntry {
	uint16 tag;
		// uniquely identifies the field
	uint16 fieldType;
		// number, string, float, etc.
	uint32 count;
		// length / number of values
		
	// The actual value or the file offset
	// where the actual value is located
	union {
		float 	floatval;
		uint32	longval;
		uint16	shortvals[2];
		uint8	bytevals[4];
	};
};

class TiffField {
public:
	TiffField(IFDEntry &entry);
	virtual ~TiffField() {};
	
	status_t InitCheck() 	{ return finitStatus; };
	uint16 GetTag()			{ return ftag; };
	uint16 GetType()		{ return ffieldType; };
	uint32 GetCount()		{ return fcount; };
	
protected:
	status_t finitStatus;
	
private:
	uint16 ftag;
	uint16 ffieldType;
	uint32 fcount;
};

#endif // #define TIFF_FIELD_H
