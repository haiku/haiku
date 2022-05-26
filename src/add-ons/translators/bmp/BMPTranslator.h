/*****************************************************************************/
// BMPTranslator
// BMPTranslator.h
//
// This BTranslator based object is for opening and writing BMP files.
//
//
// Copyright (c) 2002 Haiku Project
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

#ifndef BMP_TRANSLATOR_H
#define BMP_TRANSLATOR_H

#include <ByteOrder.h>
#include <Catalog.h>
#include <DataIO.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <Locale.h>
#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>

#include "BaseTranslator.h"

#define BMP_NO_COMPRESS 0
#define BMP_RLE8_COMPRESS 1
#define BMP_RLE4_COMPRESS 2

#define BMP_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(1,0,0)
#define BMP_IN_QUALITY 0.4
#define BMP_IN_CAPABILITY 0.8
#define BMP_OUT_QUALITY 0.4
#define BMP_OUT_CAPABILITY 0.8

#define BBT_IN_QUALITY 0.4
#define BBT_IN_CAPABILITY 0.6
#define BBT_OUT_QUALITY 0.4
#define BBT_OUT_CAPABILITY 0.6


struct BMPFileHeader {
	// for both MS and OS/2 BMP formats
	uint16 magic;			// = 'BM'
	uint32 fileSize;		// file size in bytes
	uint32 reserved;		// equals 0
	uint32 dataOffset;		// file offset to actual image
};

struct MSInfoHeader {
	uint32 size;			// size of this struct (40)
	int32 width;			// bitmap width
	int32 height;			// bitmap height
	uint16 planes;			// number of planes, always 1?
	uint16 bitsperpixel;	// bits per pixel, (1,4,8,16, 24 or 32)
	uint32 compression;		// type of compression
	uint32 imagesize;		// size of image data if compressed
	uint32 xpixperm;		// horizontal pixels per meter
	uint32 ypixperm;		// vertical pixels per meter
	uint32 colorsused;		// number of actually used colors
	uint32 colorsimportant;	// number of important colors, zero = all
};

struct OS2InfoHeader {
	uint32 size;			// size of this struct (12)
	uint16 width;			// bitmap width
	uint16 height;			// bitmap height
	uint16 planes;			// number of planes, always 1?
	uint16 bitsperpixel;	// bits per pixel, (1,4,8,16, 24 or 32)
};

class BMPTranslator : public BaseTranslator {
public:
	BMPTranslator();

	virtual status_t DerivedIdentify(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType);
		
	virtual status_t DerivedTranslate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination, int32 baseType);
		
	virtual BView *NewConfigView(TranslatorSettings *settings);	

protected:
	virtual ~BMPTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user

private:
	
	status_t translate_from_bits(BPositionIO *inSource, uint32 outType,
		BPositionIO *outDestination);
		
	status_t translate_from_bmp(BPositionIO *inSource, uint32 outType,
		BPositionIO *outDestination);
};

#endif // #ifndef BMP_TRANSLATOR_H
