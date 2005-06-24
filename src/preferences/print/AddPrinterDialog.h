/*****************************************************************************/
// Printers Preference Application.
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001-2003 OpenBeOS Project
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


#ifndef ADDPRINTERDIALOG_H
#define ADDPRINTERDIALOG_H

class AddPrinterDialog;

#include <Button.h>
#include <String.h>
#include <TextControl.h>
#include <PopUpMenu.h>
#include <Window.h>

class AddPrinterDialog : public BWindow
{
	typedef BWindow Inherited;
public:
	static status_t Start();

private:
				AddPrinterDialog();
	void        MessageReceived(BMessage* msg);

	void		BuildGUI(int stage);
	void        FillMenu(BMenu* menu, const char* path, uint32 what);
	void        AddPortSubMenu(BMenu* menu, const char* transport, const char* port);
	void        Update();

	BTextControl* fName;
	BPopUpMenu*   fPrinter;
	BPopUpMenu*   fTransport;
	BButton*      fOk;
	
	BString       fNameText;
	BString       fPrinterText;
	BString       fTransportText;
	BString       fTransportPathText;
};

#endif
