/*****************************************************************************/
// PNGTranslator
// Written by Michael Wilber, OBOS Translation Kit Team
//
// PNGTranslator.h
//
// This BTranslator based object is for opening and writing 
// PNG images.
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
#include "PNGTranslatorSettings.h"

// IO Extension Names:

#define PNG_TRANSLATOR_VERSION 0x100

#define PNG_IN_QUALITY 0.8
#define PNG_IN_CAPABILITY 0.8
#define PNG_OUT_QUALITY 0.8
#define PNG_OUT_CAPABILITY 0.5

#define BBT_IN_QUALITY 0.8
#define BBT_IN_CAPABILITY 0.6
#define BBT_OUT_QUALITY 0.5
#define BBT_OUT_CAPABILITY 0.4


class PNGTranslator : public BTranslator {
public:
	PNGTranslator();
	
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
		
	virtual status_t GetConfigurationMessage(BMessage *ioExtension);
		// write the current state of the translator into
		// the supplied BMessage object
		
	virtual status_t MakeConfigurationView(BMessage *ioExtension,
		BView **outView, BRect *outExtent);
		// creates and returns the view for displaying information
		// about this translator
		
	PNGTranslatorSettings *AcquireSettings();

protected:
	virtual ~PNGTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user
		
private:
	PNGTranslatorSettings *fpsettings;
	
	char fName[30];
	char fInfo[100];
};

#endif // #ifndef PNG_TRANSLATOR_H
