/*****************************************************************************/
//               File: FuncTranslator.h
//              Class: BFuncTranslator
//             Author: Michael Wilber, Translation Kit Team
// Originally Created: 2002-06-11
//
// Description:
//   This header file contains the BTranslator based object for
//   function based translators, i.e. the translators which export
//   each translator function individually, instead of exporting
//   the make_nth_translator() function, which returns the
//   translator as a class derived from BTranslator.
//
//   All of the BeOS R5 translators are function based translators.
//   All of the Gobe Productive translators are function based
//   translators. Nearly all of the translators I've found, with
//   the exception of the Haiku translators, are function based.
//
//   This class is used by the OpenBeOS BTranslatorRoster
//   so that function based translators, make_nth_translator()
//   translators and private BTranslator objects could be
//   accessed in the same way. 
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

#ifndef _FUNC_TRANSLATOR_H
#define _FUNC_TRANSLATOR_H

#include <Translator.h>
#include <image.h>
#include <TranslationDefs.h>

// Structure used to hold the collection of function pointers and 
// public variables exported by function based translator add-ons.
struct translator_data {
	const char *translatorName;
	const char *translatorInfo;
	int32 translatorVersion;
	const translation_format *inputFormats;
	const translation_format *outputFormats;

	status_t (*Identify)(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType);
		
	status_t (*Translate)(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination);
		
	status_t (*MakeConfig)(BMessage *ioExtension,
		BView **outView, BRect *outExtent);
		
	status_t (*GetConfigMessage)(BMessage *ioExtension);
};

class BFuncTranslator : public BTranslator {
public:
	BFuncTranslator(const translator_data *kpData);
		// assigns the translator to the object

	virtual const char *TranslatorName() const;
		// returns the short translator name
		
	virtual const char *TranslatorInfo() const;
		// returns the verbose translator name/description
		
	virtual int32 TranslatorVersion() const;
		// returns the translator's version
	
	virtual const translation_format *InputFormats(int32 *out_count) const;
		// returns the list of supported input formats
		
	virtual const translation_format *OutputFormats(int32 *out_count) const;
		// returns the list of supported output formats

	virtual status_t Identify(BPositionIO *inSource, 
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType);
		// identifies wether or not the translator can handle
		// translating the data in inSource

	virtual status_t Translate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension, uint32 outType,
		BPositionIO * outDestination);
		// translates the data from inSource to outDestination
		// in the format outType

	virtual status_t MakeConfigurationView(BMessage *ioExtension,
		BView **outView, BRect *outExtent);
		// creates a view object that allows the user to change
		// the translator's options
		// (not required to be in all translators)

	virtual status_t GetConfigurationMessage(BMessage *ioExtension);
		// returns the current settings of the translator
		// (not required to be in all translators)

protected:
	virtual ~BFuncTranslator();
		// This object is deleted by calling Release(),
		// it can not be deleted directly. See BTranslator in the Be Book
		
private:
	translator_data *fpData;
		// This contains all of the member variables used by this class.
		// It also contains pointers to functions that the member functions
		// use to do all of the actual work for this class.
};

#endif // _FUNC_TRANSLATOR_H
