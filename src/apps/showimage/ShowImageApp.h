/*****************************************************************************/
// ShowImageApp
// Written by Fernando Francisco de Oliveira, Michael Wilber, Michael Pfeiffer
//
// ShowImageApp.h
//
//
// Copyright (c) 2003 OpenBeOS Project
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

#ifndef _ShowImageApp_h
#define _ShowImageApp_h

#include <Application.h>

class ShowImageApp : public BApplication {
public:
	ShowImageApp();

public:
	virtual void AboutRequested();
	virtual void ArgvReceived(int32 argc, char **argv);
	virtual void MessageReceived(BMessage *pmsg);
	virtual void ReadyToRun();
	virtual void Pulse();
	virtual void RefsReceived(BMessage *pmsg);

private:
	void StartPulse();
	void Open(const entry_ref *pref);
	void BroadcastToWindows(uint32 what);

	BFilePanel *fpOpenPanel;
	bool fbPulseStarted;
};

#endif /* _ShowImageApp_h */
