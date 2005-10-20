/*****************************************************************************/
// BMPView
// BMPView.h
//
// This BView based object displays information about the BMPTranslator.
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

#ifndef BMPVIEW_H
#define BMPVIEW_H

#include <View.h>
#include <MenuField.h>
#include <MenuItem.h>
#include "TranslatorSettings.h"

class BMPView : public BView {
public:
	BMPView(const BRect &frame, const char *name, uint32 resize,
		uint32 flags, TranslatorSettings *settings);
		// sets up the view
		
	~BMPView();
		// releases settings

	virtual	void Draw(BRect area);
		// draws information about the BMPTranslator
private:
	TranslatorSettings *fSettings;
};

#endif // #ifndef BMPVIEW_H
