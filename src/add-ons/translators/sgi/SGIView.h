/*****************************************************************************/
// SGIView
// Adopted by Stephan Aßmus, <stippi@yellowbites.com>
// from TIFFView written by
// Picking the compression method added by Stephan Aßmus, <stippi@yellowbites.com>
//
// SGIView.h
//
// This BView based object displays information about the SGITranslator.
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
#ifndef SGIVIEW_H
#define SGIVIEW_H


#include <View.h>
#include "TranslatorSettings.h"


class BMenuField;


class SGIView : public BView {
public:
								SGIView(const char* name, uint32 flags, TranslatorSettings* settings);
									// sets up the view

								~SGIView();
									// releases the SGITranslator settings

	virtual	void				AllAttached();
	virtual	void				MessageReceived(BMessage* message);

	enum {
		MSG_COMPRESSION_CHANGED	= 'cmch',
	};

private:
			BMenuField*			fCompressionMF;

			TranslatorSettings*	fSettings;
				// the actual settings for the translator, shared with the translator
};


#endif // #ifndef SGIVIEW_H
