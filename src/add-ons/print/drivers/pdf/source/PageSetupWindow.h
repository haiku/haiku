/*

PDF Writer printer driver.

Copyright (c) 2001 OpenBeOS. 

Authors: 
	Philippe Houdoin
	Simon Gauvin
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#ifndef PAGESETUPWINDOW_H
#define PAGESETUPWINDOW_H

#include <InterfaceKit.h>
#include <Message.h>
#include <Messenger.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>
#include "InterfaceUtils.h"
#include "Utils.h"
#include "Fonts.h"

class MarginView;

class PageSetupWindow : public HWindow 
{
public:
	// Constructors, destructors, operators...

							PageSetupWindow(BMessage *msg, const char *printerName = NULL);
							~PageSetupWindow();

	typedef HWindow 		inherited;

	// public constantes
	enum {
		OK_MSG				= 'ok__',
		CANCEL_MSG			= 'cncl',
		FONTS_MSG			= 'font',
		ADVANCED_MSG        = 'advc'
	};
			
	// Virtual function overrides
public:	
	virtual void 			MessageReceived(BMessage *msg);
	virtual bool 			QuitRequested();
	status_t 				Go();

	// From here, it's none of your business! ;-)
private:
	long 					fExitSem;
	status_t 				fResult;
	BMessage *				fSetupMsg;
	BMenuField *			fPageSizeMenu;
	BMenuField *			fOrientationMenu;
	BMenuField *			fPDFCompatibilityMenu;
	BSlider *				fPDFCompressionSlider;
	Fonts *                 fFonts;
	BMessage                fAdvancedSettings;
		
	void					UpdateSetupMessage();

	MarginView * 			fMarginView;
	
	// used for saving settings 
	BString					fPrinterDirName;

	//private class constants
	static const int kMargin = 10;
	static const int kOffset = 200;
};

#endif
