/*****************************************************************************/
// BaseTranslator
// Written by Michael Wilber, Haiku Translation Kit Team
//
// BaseTranslator.h
//
// The BaseTranslator class implements functionality common to most
// Translators so that this functionality need not be implemented over and
// over in each Translator.
//
//
// Copyright (c) 2004  Haiku, Inc.
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

#ifndef BASE_TRANSLATOR_H
#define BASE_TRANSLATOR_H

#include <ByteOrder.h>
#include <DataIO.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <Translator.h>
#include <TranslatorFormats.h>
#include <TranslationDefs.h>
#include <View.h>
#include "TranslatorSettings.h"

class BaseTranslator : public BTranslator {
public:
	BaseTranslator(const char *name, const char *info,
		const int32 version, const translation_format *inFormats,
		int32 inCount, const translation_format *outFormats, int32 outCount,
		const char *settingsFile, const TranSetting *defaults, int32 defCount,
		uint32 tranGroup, uint32 tranType);

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

	TranslatorSettings *AcquireSettings();

	// Functions to be implmemented by derived class
	virtual status_t DerivedIdentify(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType);

	virtual status_t DerivedTranslate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination, int32 baseType);

	virtual status_t DerivedCanHandleImageSize(float width, float height) const;

	virtual BView *NewConfigView(TranslatorSettings *settings);


protected:
	status_t BitsCheck(BPositionIO *inSource, BMessage *ioExtension,
		uint32 &outType);

	status_t BitsIdentify(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType);

	status_t identify_bits_header(BPositionIO *inSource,
		translator_info *outInfo, TranslatorBitmap *pheader = NULL);

	status_t BitsTranslate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension, uint32 outType,
		BPositionIO *outDestination);

	status_t translate_from_bits_to_bits(BPositionIO *inSource,
		uint32 outType, BPositionIO *outDestination);

	virtual ~BaseTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user

	TranslatorSettings *fSettings;

private:
	int32 fVersion;
	char *fName;
	char *fInfo;
	const translation_format *fInputFormats;
	int32 fInputCount;
	const translation_format *fOutputFormats;
	int32 fOutputCount;
	uint32 fTranGroup;
	uint32 fTranType;
};

void translate_direct_copy(BPositionIO *inSource, BPositionIO *outDestination);

#endif // #ifndef BASE_TRANSLATOR_H

