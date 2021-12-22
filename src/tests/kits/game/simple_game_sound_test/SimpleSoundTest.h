//------------------------------------------------------------------------------
// SimpleSoundTest.h
//
// Unit test for the game kit.
//
// Copyright (c) 2001 Haiku Project
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
//
//	File Name:		SimpleSoundTest.h
//	Author:			Christopher ML Zumwalt May (zummy@users.sf.net)
//	Description:	BSimpleGameSound test application
//------------------------------------------------------------------------------

#ifndef _SIMPLESOUNDTEST_H
#define _SIMPLESOUNDTEST_H

#include <Application.h>
#include <Window.h>

class SimpleSoundWin : public BWindow
{
public:
	SimpleSoundWin(BRect frame, const char * title);
	~SimpleSoundWin();

	void Quit();
	void MessageReceived(BMessage* msg);
	
	void SetSound(BSimpleGameSound* sound);
	
private:

	BSlider * 			fGain;
	BSlider * 			fPan;
	
	BButton *			fBrowse;
	BButton * 			fStart;
	BButton * 			fStop;
	BCheckBox *			fLoop;
	BTextControl *		fRamp;
	
	BSimpleGameSound * 	fSound;
	BFilePanel *		fPanel;
};

class SimpleSoundApp : public BApplication
{
public:
	SimpleSoundApp(const char * signature);
	
	void ReadyToRun();
	void RefsReceived(BMessage * msg);

private:
	SimpleSoundWin *	fWin;
};

#endif
