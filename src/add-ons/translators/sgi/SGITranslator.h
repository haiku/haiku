/*****************************************************************************/
// SGITranslator
// Written by Stephan Aßmus
// based on TIFFTranslator written mostly by
// Michael Wilber, Haiku Translation Kit Team
//
// SGITranslator.h
//
// This BTranslator based object is for opening and writing 
// SGI images.
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

#ifndef SGI_TRANSLATOR_H
#define SGI_TRANSLATOR_H

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

#define SGI_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(1,0,0)

#define SGI_IN_QUALITY 0.5
#define SGI_IN_CAPABILITY 0.6
#define SGI_OUT_QUALITY 1.0
	// high out quality because this code outputs fully standard SGIs
#define SGI_OUT_CAPABILITY 0.4
	// medium out capability because not many SGI features are supported (?)

#define BBT_IN_QUALITY 0.4
#define BBT_IN_CAPABILITY 0.6
#define BBT_OUT_QUALITY 0.4
#define BBT_OUT_CAPABILITY 0.6

// SGI Translator Settings
#define SGI_SETTING_COMPRESSION "sgi /compression"

enum {
	SGI_FORMAT	= 'SGI ',
};

class SGITranslator : public BaseTranslator {
public:
	SGITranslator();
	
	virtual status_t DerivedIdentify(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType);
		
	virtual status_t DerivedTranslate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination, int32 baseType);
		
	virtual BView *NewConfigView(TranslatorSettings *settings);

protected:
	virtual ~SGITranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user
		
private:

	status_t translate_from_bits(BPositionIO *inSource, uint32 outType,
		BPositionIO *outDestination);
		
	status_t translate_from_sgi(BPositionIO *inSource, uint32 outType,
		BPositionIO *outDestination);
};

#endif // #ifndef SGI_TRANSLATOR_H
