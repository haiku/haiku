/*****************************************************************************/
// STXTTranslator
// Written by Michael Wilber, OBOS Translation Kit Team
//
// STXTTranslator.h
//
// This BTranslator based object is for opening and writing 
// StyledEdit (STXT) files.
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

#ifndef STXT_TRANSLATOR_H
#define STXT_TRANSLATOR_H

#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <File.h>
#include <ByteOrder.h>
#include <fs_attr.h>
#include "BaseTranslator.h"

#define STXT_TRANSLATOR_VERSION B_TRANSLATION_MAKE_VERSION(1,0,0)
#define STXT_IN_QUALITY 0.5
#define STXT_IN_CAPABILITY 0.5
#define STXT_OUT_QUALITY 0.5
#define STXT_OUT_CAPABILITY 0.5

#define TEXT_IN_QUALITY 0.4
#define TEXT_IN_CAPABILITY 0.6
#define TEXT_OUT_QUALITY 0.4
#define TEXT_OUT_CAPABILITY 0.6

class STXTTranslator : public BaseTranslator {
public:
	STXTTranslator();
	
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
		
	virtual BView *NewConfigView(TranslatorSettings *settings);

protected:
	virtual ~STXTTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user
		
private:
};

#endif // #ifndef STXT_TRANSLATOR_H
