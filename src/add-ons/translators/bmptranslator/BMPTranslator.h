/*****************************************************************************/
// BMPTranslator
// BMPTranslator.h
//
// This BTranslator based object is for opening and writing BMP files.
//
//
// Copyright (c) 2002 OpenBeOS Project
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

#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <DataIO.h>
#include <ByteOrder.h>

#define BMP_NO_COMPRESS 0
#define BMP_RLE8_COMPRESS 1
#define BMP_RLE4_COMPRESS 2

#define BMP_TRANSLATOR_VERSION 0x100
#define BMP_IN_QUALITY 1.0
	// high in quality becuase this code supports all BMP features
#define BMP_IN_CAPABILITY 0.8
	// high in capability because this code opens basically all BMPs
#define BMP_OUT_QUALITY 1.0
	// high out quality because this code outputs fully standard BMPs
#define BMP_OUT_CAPABILITY 0.5
	// medium out capability because RLE compression is not an option

#define BBT_IN_QUALITY 0.6
	// medium in quality because only most common features are supported
#define BBT_IN_CAPABILITY 0.8
	// high in capability because most variations are supported
#define BBT_OUT_QUALITY 0.6
	// medium out quality because only most common features are supported
#define BBT_OUT_CAPABILITY 0.8
	// high out capability because most variations are supported


struct BMPFileHeader {
	// for both MS and OS/2 BMP formats
	uint16 magic;			// = 'BM'
	uint32 fileSize;		// file size in bytes
	uint32 reserved;		// equals 0
	uint32 dataOffset;		// file offset to actual image
};

struct MSInfoHeader {
	uint32 size;			// size of this struct (40)
	uint32 width;			// bitmap width
	uint32 height;			// bitmap height
	uint16 planes;			// number of planes, always 1?
	uint16 bitsperpixel;	// bits per pixel, (1,4,8,16 or 24)
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
	uint16 bitsperpixel;	// bits per pixel, (1,4,8,16 or 24)
};

class BMPTranslator : public BTranslator {
public:
	BMPTranslator();
	
	virtual const char *TranslatorName() const;
		// returns the short name of the translator
		
	virtual const char *TranslatorInfo() const;
		// returns a verbose name/description for the translator
	
	virtual int32 TranslatorVersion() const;
		// returns the version of the translator

	virtual const translation_format *InputFormats(int32 *out_count)
		const;
		// returns the input formats and the count of input formats
		// that this translator supports
		
	virtual const translation_format *OutputFormats(int32 *out_count)
		const;
		// returns the output formats and the count of output formats
		// that this translator supports

	virtual status_t Identify(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType);
		// determines whether or not this translator can convert the
		// data in inSource to the type outType

	virtual status_t Translate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination);
		// this function is the whole point of the Translation Kit,
		// it translates the data in inSource to outDestination
		// using the format outType
		
	virtual status_t MakeConfigurationView(BMessage *ioExtension,
		BView **outView, BRect *outExtent);
		// creates and returns the view for displaying information
		// about this translator

protected:
	virtual ~BMPTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user
		
private:
	char fName[30];
	char fInfo[100];
};

#endif // #ifndef BMP_TRANSLATOR_H
