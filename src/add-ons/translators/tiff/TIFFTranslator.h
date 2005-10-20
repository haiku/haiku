/*****************************************************************************/
// TIFFTranslator
// Written by Michael Wilber, OBOS Translation Kit Team
//
// TIFFTranslator.h
//
// This BTranslator based object is for opening and writing 
// TIFF images.
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

#ifndef TIFF_TRANSLATOR_H
#define TIFF_TRANSLATOR_H

#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <DataIO.h>
#include <File.h>
#include <ByteOrder.h>
#include <fs_attr.h>
#include "DecodeTree.h"

// IO Extension Names:
#define DOCUMENT_COUNT "/documentCount"
#define DOCUMENT_INDEX "/documentIndex"

#define TIFF_TRANSLATOR_VERSION 100

#define TIFF_IN_QUALITY 0.1
#define TIFF_IN_CAPABILITY 0.1
#define TIFF_OUT_QUALITY 0.6
#define TIFF_OUT_CAPABILITY 0.2

#define BBT_IN_QUALITY 0.4
#define BBT_IN_CAPABILITY 0.6
#define BBT_OUT_QUALITY 0.4
#define BBT_OUT_CAPABILITY 0.6

enum TIFF_IMAGE_TYPE {
	TIFF_BILEVEL = 1,
	TIFF_PALETTE,
	TIFF_RGB,
	TIFF_CMYK
};

// class for storing only the TIFF fields
// that are of interest to the TIFFTranslator
//
// The class is very minimal so that it is
// convenient to use, but cleans up after itself
class TiffDetails {
public:
	TiffDetails();
	~TiffDetails();
	
	uint32 width;
	uint32 height;
	uint32 compression;
	uint32 t4options;
	uint32 rowsPerStrip;
	uint32 stripsPerImage;
	uint32 *pstripOffsets;
	uint32 *pstripByteCounts;
	uint8 *pcolorMap;
	uint16 interpretation;
	uint16 bitsPerPixel;
	uint16 imageType;
	uint16 fillOrder;
};

class TIFFTranslator : public BTranslator {
public:
	TIFFTranslator();
	
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
	virtual ~TIFFTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user
		
private:
	status_t LoadHuffmanTrees();
	
	ssize_t decode_huffman(StreamBuffer *pstreambuf, TiffDetails &details, 
		uint8 *pbits);
	ssize_t decode_t4(BitReader &stream, TiffDetails &details, 
		uint8 *pbits, bool bfirstLine);
	
	status_t translate_from_tiff(BPositionIO *inSource, BMessage *ioExtension, 
		ssize_t amtread, uint8 *read, swap_action swp, uint32 outType,
		BPositionIO *outDestination);
	
	DecodeTree *fpblackTree;
	DecodeTree *fpwhiteTree;
	
	char fName[30];
	char fInfo[100];
};

#endif // #ifndef TIFF_TRANSLATOR_H
