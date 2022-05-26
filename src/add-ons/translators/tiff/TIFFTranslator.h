/*****************************************************************************/
// TIFFTranslator
// Written by Michael Wilber, Haiku Translation Kit Team
//
// TIFFTranslator.h
//
// This BTranslator based object is for opening and writing 
// TIFF images.
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
#include "BaseTranslator.h"

// IO Extension Names:
#define DOCUMENT_COUNT "/documentCount"
#define DOCUMENT_INDEX "/documentIndex"

// TIFF Translator Settings
#define TIFF_SETTING_COMPRESSION "tiff /compression"

#define TIFF_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(1,0,0)

#define TIFF_IN_QUALITY 0.7
#define TIFF_IN_CAPABILITY 0.6
#define TIFF_OUT_QUALITY 0.7
#define TIFF_OUT_CAPABILITY 0.6

#define BBT_IN_QUALITY 0.7
#define BBT_IN_CAPABILITY 0.6
#define BBT_OUT_QUALITY 0.7
#define BBT_OUT_CAPABILITY 0.6

class TIFFTranslator : public BaseTranslator {
public:
	TIFFTranslator();
	
	virtual status_t DerivedIdentify(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType);
		
	virtual status_t DerivedTranslate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination, int32 baseType);
		
	virtual BView *NewConfigView(TranslatorSettings *settings);

protected:
	virtual ~TIFFTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user
		
private:	
	status_t translate_from_bits(BPositionIO *inSource, uint32 outType,
		BPositionIO *outDestination);
		
	status_t translate_from_tiff(BPositionIO *inSource, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination);
};

#endif // #ifndef TIFF_TRANSLATOR_H
