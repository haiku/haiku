/*****************************************************************************/
//               File: R4xTranslator.h
//              Class: BR4xTranslator
//             Author: Michael Wilber, Translation Kit Team
// Originally Created: 2002-06-11
//
// Description: This header file contains the BTranslator based object for
//              BeOS R4.0 and R4.5 type translators, aka, the translators
//              that don't use the make_nth_translator() mechanism.
//
//              This class is used by the OpenBeOS BTranslatorRoster
//              so that R4x translators, post R4.5 translators and private
//              BTranslator objects could be accessed in the same way. 
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

#ifndef _R4X_TRANSLATOR_H
#define _R4X_TRANSLATOR_H

#include <Translator.h>
#include <image.h>
#include <TranslationDefs.h>

class BR4xTranslator : public BTranslator {
public:
	BR4xTranslator(const translator_data *kpData);
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
	virtual ~BR4xTranslator();
		// This object is deleted by calling Release(),
		// it can not be deleted directly. See BTranslator in the Be Book
		
private:
	translator_data *fpData;
		// This contains all of the member variables used by this class.
		// It also contains pointers to functions that the member functions
		// use to do all of the actual work for this class.
};

#endif // _R4X_TRANSLATOR_H
