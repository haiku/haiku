/*

PDF Writer printer driver.

Copyright (c) 2002 OpenBeOS. 

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

#ifndef DOCINFOWINDOW_H
#define DOCINFOWINDOW_H

#include <InterfaceKit.h>
#include <Message.h>
#include <Messenger.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>
#include "Utils.h"

class DocInfoWindow : public HWindow 
{
public:
	// Constructors, destructors, operators...

							DocInfoWindow(BMessage *doc_info);

	typedef HWindow 		inherited;

	// public constantes
	enum {
		OK_MSG				= 'ok__',
		CANCEL_MSG			= 'cncl',
		ADD_KEY_MSG         = 'add_',
		REMOVE_KEY_MSG      = 'rmov',
		DEFAULT_KEY_MSG     = 'dflt',
	};
			
	// Virtual function overrides
public:	
	virtual void 			MessageReceived(BMessage *msg);
	virtual bool 			QuitRequested();
	virtual void            Quit();

private:
	BMessage               *fDocInfo;
	BView                  *fTable;
	BScrollView            *fTableScrollView;
	BMenu                  *fKeyList;

	void                    BuildTable(BMessage *fromDocInfo);
	void                    ReadFieldsFromTable(BMessage *toDocInfo);
	void                    EmptyKeyList();
	bool                    IsValidKey(const char *key);
	void                    AddKey(BMessage* msg, bool textControl);
	void                    RemoveKey(BMessage* msg);
};

#endif
