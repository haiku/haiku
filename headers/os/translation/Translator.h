/*****************************************************************************/
//               File: Translator.h
//              Class: BTranslator
//   Reimplimented by: Michael Wilber, Translation Kit Team
//   Reimplimentation: 2002-06-15
//
// Description: This file contains the BTranslator class, the base class for
//              all translators that don't use the BeOS R4/R4.5 add-on method.
//
//
// Copyright (c) 2002 OpenBeOS Project
//
// Original Version: Copyright 1998, Be Incorporated, All Rights Reserved.
//                   Copyright 1995-1997, Jon Watte
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
#ifndef _TRANSLATOR_H
#define _TRANSLATOR_H

#include <TranslationDefs.h>
#include <Archivable.h>

class BTranslator : public BArchivable {
public:
	BTranslator();
		// Sets refcount to 1 (the object is initially Acquired()'d once)

	BTranslator *Acquire();
		// return actual object and increment the refcount
		
	BTranslator *Release();
		// Decrements the refcount and returns pointer to the object.
		// When the refcount is zero, NULL is returned and the object is
		// deleted
		
	int32 ReferenceCount();
		// returns the refcount, THIS IS NOT THREAD SAFE, ITS ONLY FOR "FUN"

	virtual const char *TranslatorName() const = 0;
		// returns the short name of the translator
		
	virtual const char *TranslatorInfo() const = 0;
		// returns a verbose name/description for the translator
	
	virtual int32 TranslatorVersion() const = 0;
		// returns the version of the translator

	virtual const translation_format *InputFormats(int32 *out_count)
		const = 0;
		// returns the input formats and the count of input formats
		// that this translator supports
		
	virtual const translation_format *OutputFormats(int32 *out_count)
		const = 0;
		// returns the output formats and the count of output formats
		// that this translator supports

	virtual status_t Identify(BPositionIO *inSource,
		const translation_format *inFormat, BMessage *ioExtension,
		translator_info *outInfo, uint32 outType) = 0;
		// determines whether or not this translator can convert the
		// data in inSource to the type outType

	virtual status_t Translate(BPositionIO *inSource,
		const translator_info *inInfo, BMessage *ioExtension,
		uint32 outType, BPositionIO *outDestination) = 0;
		// this function is the whole point of the Translation Kit,
		// it translates the data in inSource to outDestination
		// using the format outType

	virtual status_t MakeConfigurationView(BMessage *ioExtension,
		BView **outView, BRect *outExtent);
		// this function creates a view that allows the user to
		// configure the translator, this function is not
		// required for all translators

	virtual status_t GetConfigurationMessage(BMessage *ioExtension);
		// puts information about the current configuration of the
		// translator into ioExtension so that it can be used with
		// Identify or Translate or whatever, this function is
		// not required for all translators

protected:
	virtual ~BTranslator();
		// this is protected because the object is deleted by the
		// Release() function instead of being deleted directly by
		// the user

private:
	int32 fRefCount;
		// number of times this object has been referenced
		// by the Acquire() function
		
	sem_id fSem;
		// used to lock object for Acquire() and Release()
	
	// For binary compatibility with past and future
	// version of this class
	uint32 fUnused[8];
	virtual status_t _Reserved_Translator_0(int32, void *);
	virtual status_t _Reserved_Translator_1(int32, void *);
	virtual status_t _Reserved_Translator_2(int32, void *);
	virtual status_t _Reserved_Translator_3(int32, void *);
	virtual status_t _Reserved_Translator_4(int32, void *);
	virtual status_t _Reserved_Translator_5(int32, void *);
	virtual status_t _Reserved_Translator_6(int32, void *);
	virtual status_t _Reserved_Translator_7(int32, void *);
};

// The post-4.5 API suggests implementing this function in your translator
// add-on rather than the separate functions and variables of the previous
// API. You will be called for values of n starting at 0 and increasing;
// return 0 when you can't make another kind of translator (i.e. for n=1
// if you only implement one subclass of BTranslator). Ignore flags for now.
extern "C" _EXPORT BTranslator *make_nth_translator(int32 n, image_id you,
	uint32 flags, ...);



#endif /* _TRANSLATOR_H */
