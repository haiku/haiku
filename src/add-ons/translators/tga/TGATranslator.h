/*****************************************************************************/
// TGATranslator
// Written by Michael Wilber, Haiku Translation Kit Team
//
// TGATranslator.h
//
// This BTranslator based object is for opening and writing TGA files.
//
//
// Copyright (c) 2002  Haiku, Inc.
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

#ifndef TGA_TRANSLATOR_H
#define TGA_TRANSLATOR_H

#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <DataIO.h>
#include <ByteOrder.h>
#include "BaseTranslator.h"

#define TGA_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(1,0,0)
#define TGA_IN_QUALITY 0.7
#define TGA_IN_CAPABILITY 0.8
#define TGA_OUT_QUALITY 0.7
#define TGA_OUT_CAPABILITY 0.6

#define BBT_IN_QUALITY 0.7
#define BBT_IN_CAPABILITY 0.6
#define BBT_OUT_QUALITY 0.6
#define BBT_OUT_CAPABILITY 0.8

// TGA Translator Settings
#define TGA_SETTING_RLE "tga /rle"
#define TGA_SETTING_IGNORE_ALPHA "tga /ignore_alpha"

// TGA files are stored in the Intel byte order :)
struct TGAFileHeader {
	uint8 idlength;
		// Number of bytes in the Image ID field
	uint8 colormaptype;
		// 0	Has NO color-map (palette)
		// 1	Has color-map (palette)
	uint8 imagetype;
		// 0	No Image Data Included
		// 1	Uncompressed, Color-mapped image
		// 2	Uncompressed, True-color image
		// 3	Uncompressed, Black-and-white (Greyscale?) image
		// 9	Run-length encoded, Color-mapped image
		// 10	Run-length encoded, True-color image
		// 11	Run-length encoded, Black-and-white (Greyscale?) image
};

#define TGA_NO_COLORMAP			0
#define TGA_COLORMAP			1

#define TGA_NO_IMAGE_DATA		0

#define TGA_NOCOMP_COLORMAP		1
#define TGA_NOCOMP_TRUECOLOR	2
#define TGA_NOCOMP_BW			3
#define TGA_RLE_COLORMAP		9
#define TGA_RLE_TRUECOLOR		10
#define TGA_RLE_BW				11

// Information about the color map (palette). These bytes are
// always present, but are zero if no color map is present
struct TGAColorMapSpec {
	uint16 firstentry;		// first useful entry in the color map
	uint16 length;			// number of color map entries
	uint8 entrysize;		// number of bits per entry
};

struct TGAImageSpec {
	uint16 xorigin;
	uint16 yorigin;
	uint16 width;
	uint16 height;
	uint8 depth;
		// pixel depth includes alpha bits!
	uint8 descriptor;
		// bits 3-0: number of attribute bits per pixel
		// bits 5&4: order pixels are drawn to the screen
			// Screen Dest of 		Image Origin
			// first pixel			bit 5	bit 4
			//////////////////////////////////////////
			// bottom left			0		0
			// bottom right			0		1
			// top left				1		0
			// top right			1		1
		// bits 7&6: must be zero
};

#define TGA_ORIGIN_VERT_BIT	0x20
#define TGA_ORIGIN_BOTTOM	0
#define TGA_ORIGIN_TOP		1

#define TGA_ORIGIN_HORZ_BIT	0x10
#define TGA_ORIGIN_LEFT		0
#define TGA_ORIGIN_RIGHT	1

#define TGA_DESC_BITS76		0xc0
#define TGA_DESC_ALPHABITS	0x0f

#define TGA_RLE_PACKET_TYPE_BIT 0x80

#define TGA_HEADERS_SIZE 18

#define TGA_STREAM_BUFFER_SIZE 1024

class TGATranslator : public BaseTranslator {
public:
	TGATranslator();
	
	virtual status_t DerivedIdentify(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType);
		
	virtual status_t DerivedTranslate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination, int32 baseType);
		
	virtual BView *NewConfigView(TranslatorSettings *settings);

protected:
	virtual ~TGATranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user
		
private:
	uint8 tga_alphabits(TGAFileHeader &filehead, TGAColorMapSpec &mapspec,
		TGAImageSpec &imagespec);
		
	status_t translate_from_bits(BPositionIO *inSource, uint32 outType,
		BPositionIO *outDestination);
		
	status_t translate_from_tganm_to_bits(BPositionIO *inSource,
		BPositionIO *outDestination, TGAFileHeader &filehead,
		TGAColorMapSpec &mapspec, TGAImageSpec &imagespec);
		
	status_t translate_from_tganmrle_to_bits(BPositionIO *inSource,
		BPositionIO *outDestination, TGAFileHeader &filehead,
		TGAColorMapSpec &mapspec, TGAImageSpec &imagespec);
		
	status_t translate_from_tgamrle_to_bits(BPositionIO *inSource,
		BPositionIO *outDestination, TGAFileHeader &filehead,
		TGAColorMapSpec &mapspec, TGAImageSpec &imagespec, uint8 *pmap);
		
	status_t translate_from_tga(BPositionIO *inSource, uint32 outType,
		BPositionIO *outDestination);

};

#endif // #ifndef TGA_TRANSLATOR_H

