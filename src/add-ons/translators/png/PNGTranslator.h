/*****************************************************************************/
// PNGTranslator
// Written by Michael Wilber, Haiku Translation Kit Team
//
// PNGTranslator.h
//
// This BTranslator based object is for opening and writing 
// PNG images.
//
//
// Copyright (c) 2003 Haiku Project
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

#ifndef PNG_TRANSLATOR_H
#define PNG_TRANSLATOR_H

#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <DataIO.h>
#include <File.h>
#include <ByteOrder.h>
#include <fs_attr.h>
#include "BaseTranslator.h"

// PNG Translator Settings
#define PNG_SETTING_INTERLACE "png /interlace"

#define PNG_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(1,0,0)

#define PNG_IN_QUALITY 0.8
#define PNG_IN_CAPABILITY 0.8
#define PNG_OUT_QUALITY 0.8
#define PNG_OUT_CAPABILITY 0.5

#define BBT_IN_QUALITY 0.8
#define BBT_IN_CAPABILITY 0.6
#define BBT_OUT_QUALITY 0.5
#define BBT_OUT_CAPABILITY 0.4


class PNGTranslator : public BaseTranslator {
public:
	PNGTranslator();

	virtual status_t DerivedIdentify(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType);
		
	virtual status_t DerivedTranslate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination, int32 baseType);
		
	virtual BView *NewConfigView(TranslatorSettings *settings);

protected:
	virtual ~PNGTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user
		
private:
	status_t translate_from_png_to_bits(BPositionIO *inSource,
		BPositionIO *outDestination);
		
	status_t translate_from_png(BPositionIO *inSource, uint32 outType,
		BPositionIO *outDestination);
		
	status_t translate_from_bits_to_png(BPositionIO *inSource,
		BPositionIO *outDestination);
};

#endif // #ifndef PNG_TRANSLATOR_H
