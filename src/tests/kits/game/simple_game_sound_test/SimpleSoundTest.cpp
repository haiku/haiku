//------------------------------------------------------------------------------
// SimpleSoundTest.cpp
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

#include <stdlib.h>

#include <Button.h>
#include <Slider.h>
#include <CheckBox.h>
#include <FilePanel.h>
#include <TextControl.h>

#include "SimpleGameSound.h"
#include "SimpleSoundTest.h"

const int32 SLIDER_PAN = 'SPan';
const int32 SLIDER_GAIN = 'Gain';
const int32 BUTTON_START = 'Strt';
const int32 BUTTON_STOP = 'Stop';
const int32 BUTTON_BROWSE = 'Open';
const int32 CHECK_LOOP = 'Loop';

int main(int, char**)
{
	SimpleSoundApp app("application/x-vnd.OBOS.GameKitApp");
	app.Run();
	return 0;
}


// SimpleSoundApp ---------------------------------------------------------
SimpleSoundApp::SimpleSoundApp(const char *signature)
	: BApplication(signature)
{
}


void SimpleSoundApp::ReadyToRun()
{
	fWin = new SimpleSoundWin(BRect(100, 200, 330, 450), "GameKit Unit Test");
	fWin->Show();
}


void SimpleSoundApp::RefsReceived(BMessage* msg)
{	
	uint32 type;
	int32 count;
	msg->GetInfo("refs", &type, &count);
	if (type == B_REF_TYPE)
	{
		// Load the files
		for(int32 i = 0; i < count; i++)
		{
			entry_ref file;
			if (msg->FindRef("refs", i, &file) == B_OK)
			{
				BSimpleGameSound * sound = new BSimpleGameSound(&file);
				
				if (sound->InitCheck() == B_OK) 
					fWin->SetSound(sound);
				else
					delete sound;
			}
		}
	}
}


// SimpleSoundWin ---------------------------------------------------------------
SimpleSoundWin::SimpleSoundWin(BRect frame, const char * title)
	:	BWindow(frame, title, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
		fSound(NULL),
		fPanel(NULL)
{
	BRect r(10, 10, 100, 40);
	fBrowse = new BButton(r, "buttonBrowse", "Browse", new BMessage(BUTTON_BROWSE));
	fBrowse->SetEnabled(true);
	AddChild(fBrowse);
	
	// Add the button which start and stop playback of the choosen sound
	r.OffsetBy(0, 50);
	fStart = new BButton(r, "buttonStart", "Start", new BMessage(BUTTON_START));
	fStart->SetEnabled(false);
	AddChild(fStart);

	r.OffsetBy(110, 0);
	fStop = new BButton(r, "buttonStop", "Stop", new BMessage(BUTTON_STOP));
	fStop->SetEnabled(false);
	AddChild(fStop);

	// Control the sound's volumn (gain)	
	r.Set(10, 100, 200, 40);
	fGain = new BSlider(r, "sliderGain", "Gain", new BMessage(SLIDER_GAIN), 0, 100);
	fGain->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fGain->SetHashMarkCount(10);
	fGain->SetLimitLabels("Mute", "Full");
	fGain->SetPosition(1.0);
	fGain->SetEnabled(false);
	AddChild(fGain);
	
	// Control the sound's pan
	r.OffsetBy(0, 60);
	fPan = new BSlider(r, "sliderPan", "Pan", new BMessage(SLIDER_PAN), -100, 100);
	fPan->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fPan->SetHashMarkCount(10);
	fPan->SetLimitLabels("Left", "Right");
	fPan->SetEnabled(false);
	AddChild(fPan);	
	
	r.Set(10, 220, 110, 40);
	fRamp = new BTextControl(r, "txtRamp", "Ramp", "20", NULL);
	fRamp->SetDivider(30);
	fRamp->SetEnabled(true);
	AddChild(fRamp);
	
	r.Set(120, 220, 200, 40);
	fLoop = new BCheckBox(r, "chkLoop", "Loop", new BMessage(CHECK_LOOP));
	fLoop->SetEnabled(false);
	AddChild(fLoop);	
}


SimpleSoundWin::~SimpleSoundWin()
{
	delete fPanel;
	delete fSound;
}


void SimpleSoundWin::Quit()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	BWindow::Quit();
}


void SimpleSoundWin::MessageReceived(BMessage *msg)
{
	int32 pos;
	bigtime_t ramp = bigtime_t(atoi(fRamp->Text()) * 100000.0);
	
	switch (msg->what)
	{
	case BUTTON_START:	
		fStop->SetEnabled(true);
		fGain->SetEnabled(true);
		fPan->SetEnabled(true);
		fLoop->SetEnabled(true);
		
		fSound->StartPlaying();
		break;
		
	case BUTTON_STOP:
		fStop->SetEnabled(false);
		fGain->SetEnabled(false);
		fPan->SetEnabled(false);
		fLoop->SetEnabled(false);
		
		fSound->StopPlaying();
		break;
		
	case SLIDER_GAIN:
		fSound->SetGain(fGain->Position(), ramp);
		break;
	
	case SLIDER_PAN:
		pos = fPan->Value();
		fSound->SetPan(pos / 100.0, ramp);
		break;
		
	case BUTTON_BROWSE:
		if (!fPanel) fPanel = new BFilePanel(B_OPEN_PANEL, &be_app_messenger);
		
		fPanel->Show();
		break;
	
	case CHECK_LOOP:
		fSound->SetIsLooping(fLoop->Value() == B_CONTROL_ON);
		break;
					
	default:
		BWindow::MessageReceived(msg);
		break;
	}
}


void SimpleSoundWin::SetSound(BSimpleGameSound * sound)
{
	delete fSound;
	
	// enable the Start button if we were given a valid sound
	Lock();
	
	fStart->SetEnabled((sound != NULL));	
	fLoop->SetValue(B_CONTROL_OFF);
	fGain->SetPosition(1.0);
	fPan->SetPosition(0.0);
	
	Unlock();
	
	fSound = sound;
}
