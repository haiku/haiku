/*****************************************************************************/
// ConfigWindow
//
// Author
//   Michael Pfeiffer
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
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

#ifndef _CONFIG_WINDOW_H
#define _CONFIG_WINDOW_H

#include "BeUtils.h"
#include "ObjectList.h"

#include <InterfaceKit.h>

enum config_setup_kind {
	kPageSetup,
	kJobSetup,
};

class ConfigWindow : public BWindow {
	enum {
		MSG_PAGE_SETUP       = 'cwps',
		MSG_JOB_SETUP        = 'cwjs',
		MSG_PRINTER_SELECTED = 'cwpr',
		MSG_OK               = 'cwok',
		MSG_CANCEL           = 'cwcl',
	};
	
public:
	ConfigWindow(config_setup_kind kind, Printer* defaultPrinter, BMessage* settings, AutoReply* sender);
	~ConfigWindow();
	void Go();
	
	void MessageReceived(BMessage* m);
	void AboutRequested();
	void FrameMoved(BPoint p);

	static BRect GetWindowFrame();
	static void SetWindowFrame(BRect frame);

private:
	BPictureButton* AddPictureButton(BView* panel, BRect frame, const char* name, const char* on, const char* off, uint32 what);
	void AddStringView(BView* panel, BRect frame, const char* text);
	void PrinterForMimeType();
	void SetupPrintersMenu(BMenu* menu);
	void UpdateAppSettings(const char* mime, const char* printer);
	void UpdateSettings(bool read);
	void UpdateUI();
	void Setup(config_setup_kind);

	config_setup_kind fKind;
	Printer*    fDefaultPrinter;
	BMessage*   fSettings;
	AutoReply*  fSender;
	BString     fSenderMimeType;

	BString     fPrinterName;
	Printer*    fCurrentPrinter;
	BMessage    fPageSettings;
	BMessage    fJobSettings;

	sem_id      fFinished;

	BMenuField* fPrinters;
	BPictureButton*    fPageSetup;
	BPictureButton*    fJobSetup;
	BButton*    fOk;
};

#endif
