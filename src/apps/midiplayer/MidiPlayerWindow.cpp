/*
 * Copyright (c) 2004 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <StorageKit.h>

#include "MidiPlayerApp.h"
#include "MidiPlayerWindow.h"
#include "ScopeView.h"

#define _W(a) (a->Frame().Width())
#define _H(a) (a->Frame().Height())

//------------------------------------------------------------------------------

MidiPlayerWindow::MidiPlayerWindow()
	: BWindow(BRect(0, 0, 1, 1), "MIDI Player", B_TITLED_WINDOW, 
	          B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE)
{
	playing = false;
	scopeEnabled = true;
	reverb = B_REVERB_BALLROOM;
	volume = 75;
	windowX = -1;
	windowY = -1;

	CreateViews();
	LoadSettings();
	InitControls();
}

//------------------------------------------------------------------------------

MidiPlayerWindow::~MidiPlayerWindow()
{
	Stop(false);
}

//------------------------------------------------------------------------------

bool MidiPlayerWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what)
	{
		case MSG_PLAY_STOP:
			OnPlayStop();
			break;

		case MSG_SHOW_SCOPE:
			OnShowScope();
			break;
			
		case MSG_REVERB_NONE:
			OnReverb(B_REVERB_NONE);
			break;
			
		case MSG_REVERB_CLOSET:
			OnReverb(B_REVERB_CLOSET);
			break;
			
		case MSG_REVERB_GARAGE:
			OnReverb(B_REVERB_GARAGE);
			break;
		
		case MSG_REVERB_IGOR:
			OnReverb(B_REVERB_BALLROOM);
			break;
		
		case MSG_REVERB_CAVERN:
			OnReverb(B_REVERB_CAVERN);
			break;
		
		case MSG_REVERB_DUNGEON:
			OnReverb(B_REVERB_DUNGEON);
			break;
		
		case MSG_VOLUME:
			OnVolume();
			break;

		case B_SIMPLE_DATA:
			OnDrop(msg);
			break;

		default:
			super::MessageReceived(msg);
			break;
	}
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::FrameMoved(BPoint origin)
{
	super::FrameMoved(origin);
	windowX = Frame().left;
	windowY = Frame().top;
	SaveSettings();
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::CreateReverbMenu()
{
	BPopUpMenu* menu = new BPopUpMenu("reverbPopUp");

	reverbNone = new BMenuItem(
		"None", new BMessage(MSG_REVERB_NONE));

	reverbCloset = new BMenuItem(
		"Closet", new BMessage(MSG_REVERB_CLOSET));

	reverbGarage = new BMenuItem(
		"Garage", new BMessage(MSG_REVERB_GARAGE));

	reverbIgor = new BMenuItem(
		"Igor's Lab", new BMessage(MSG_REVERB_IGOR));

	reverbCavern = new BMenuItem(
		"Cavern", new BMessage(MSG_REVERB_CAVERN));

	reverbDungeon = new BMenuItem(
		"Dungeon", new BMessage(MSG_REVERB_DUNGEON));

	menu->AddItem(reverbNone);
	menu->AddItem(reverbCloset);
	menu->AddItem(reverbGarage);
	menu->AddItem(reverbIgor);
	menu->AddItem(reverbCavern);
	menu->AddItem(reverbDungeon);

	reverbMenu = new BMenuField(
		BRect(0, 0, 128, 17), "reverbMenu", "Reverb:", menu,
		B_FOLLOW_LEFT | B_FOLLOW_TOP);

	reverbMenu->SetDivider(50);
	reverbMenu->ResizeToPreferred();
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::CreateViews()
{
	scopeView = new ScopeView;

	showScope = new BCheckBox(
		BRect(0, 0, 1, 1), "showScope", "Scope",
		new BMessage(MSG_SHOW_SCOPE), B_FOLLOW_LEFT);
	
	showScope->SetValue(B_CONTROL_ON);
	showScope->ResizeToPreferred();

	CreateReverbMenu();
	
	volumeSlider = new BSlider(
		BRect(0, 0, 1, 1), "volumeSlider", NULL, NULL,
		0, 100, B_TRIANGLE_THUMB);

	rgb_color col = { 152, 152, 255 };
	volumeSlider->UseFillColor(true, &col);
	volumeSlider->SetModificationMessage(new BMessage(MSG_VOLUME));
	volumeSlider->ResizeToPreferred();
	volumeSlider->ResizeTo(_W(scopeView) - 37, _H(volumeSlider));

	playButton = new BButton(
		BRect(0, 1, 80, 1), "playButton", "Play", new BMessage(MSG_PLAY_STOP),
		B_FOLLOW_RIGHT);
	
	//playButton->MakeDefault(true);
	playButton->ResizeToPreferred();
	playButton->SetEnabled(false);

	BBox* background = new BBox(
		BRect(0, 0, 1, 1), B_EMPTY_STRING, B_FOLLOW_ALL_SIDES, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, 
		B_PLAIN_BORDER); 

	BBox* divider = new BBox(
		BRect(0, 0, 1, 1), B_EMPTY_STRING, B_FOLLOW_ALL_SIDES, 
		B_WILL_DRAW | B_FRAME_EVENTS, B_FANCY_BORDER); 

	divider->ResizeTo(_W(scopeView), 1);

	BStringView* volumeLabel = new BStringView(
					BRect(0, 0, 1, 1), NULL, "Volume:");
		
	volumeLabel->ResizeToPreferred();

	float width = 8 + _W(scopeView) + 8;

	float height =
		+ 8  + _H(scopeView) 
		+ 8  + _H(showScope) 
		+ 4  + _H(reverbMenu)
		+ 2  + _H(volumeSlider) 
		+ 10 + _H(divider) 
		+ 6  + _H(playButton)
		+ 10;

	ResizeTo(width, height);  

	AddChild(background);
	background->ResizeTo(width,	height);
	background->AddChild(scopeView);
	background->AddChild(showScope);
	background->AddChild(reverbMenu);
	background->AddChild(volumeLabel);
	background->AddChild(volumeSlider);
	background->AddChild(divider);
	background->AddChild(playButton);

	float y = 8;
	scopeView->MoveTo(8, y);

	y += _H(scopeView) + 8;
	showScope->MoveTo(8 + 50, y);

	y += _H(showScope) + 4;
	reverbMenu->MoveTo(8, y);

	y += _H(reverbMenu) + 2;
	volumeLabel->MoveTo(8, y);
	volumeSlider->MoveTo(8 + 44, y);

	y += _H(volumeSlider) + 10;
	divider->MoveTo(8, y);
	
	y += _H(divider) + 6;
	playButton->MoveTo((width - _W(playButton)) / 2, y);
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::CenterOnScreen()
{
	BRect screenRect = BScreen(this).Frame();
	BRect windowRect = Frame();

	MoveTo(
		(screenRect.Width()  - windowRect.Width())  / 2,
		(screenRect.Height() - windowRect.Height()) / 2);
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::InitControls()
{
	Lock();

	showScope->SetValue(scopeEnabled ? B_CONTROL_ON : B_CONTROL_OFF);
	scopeView->SetEnabled(scopeEnabled);

	reverbNone->SetMarked(reverb == B_REVERB_NONE);
	reverbCloset->SetMarked(reverb == B_REVERB_CLOSET);
	reverbGarage->SetMarked(reverb == B_REVERB_GARAGE);
	reverbIgor->SetMarked(reverb == B_REVERB_BALLROOM);
	reverbCavern->SetMarked(reverb == B_REVERB_CAVERN);
	reverbDungeon->SetMarked(reverb == B_REVERB_DUNGEON);
	be_synth->SetReverb(reverb);

	volumeSlider->SetValue(volume);

	if (windowX != -1 && windowY != -1)
	{
		MoveTo(windowX, windowY);
	}
	else
	{
		CenterOnScreen();
	}
	
	Unlock();
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::LoadSettings()
{
	BFile file(SETTINGS_FILE, B_READ_ONLY);

	if (file.InitCheck() != B_OK) { return; }
	if (file.Lock() != B_OK) { return; }

	file.ReadAttr("Scope", B_BOOL_TYPE, 0, &scopeEnabled, sizeof(bool));
	file.ReadAttr("Reverb", B_INT32_TYPE, 0, &reverb, sizeof(int32));
	file.ReadAttr("Volume", B_INT32_TYPE, 0, &volume, sizeof(int32));
	file.ReadAttr("WindowX", B_FLOAT_TYPE, 0, &windowX, sizeof(float));
	file.ReadAttr("WindowY", B_FLOAT_TYPE, 0, &windowY, sizeof(float));

	file.Unlock();
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::SaveSettings()
{
	BFile file(SETTINGS_FILE, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);

	if (file.InitCheck() != B_OK) { return; }
	if (file.Lock() != B_OK) { return; }

	file.WriteAttr("Scope", B_BOOL_TYPE, 0, &scopeEnabled, sizeof(bool));
	file.WriteAttr("Reverb", B_INT32_TYPE, 0, &reverb, sizeof(int32));
	file.WriteAttr("Volume", B_INT32_TYPE, 0, &volume, sizeof(int32));
	file.WriteAttr("WindowX", B_FLOAT_TYPE, 0, &windowX, sizeof(float));
	file.WriteAttr("WindowY", B_FLOAT_TYPE, 0, &windowY, sizeof(float));

	file.Sync();
	file.Unlock();
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::LoadFile(entry_ref* ref)
{
	if (playing)
	{
		Stop(false);
	}

	synth.UnloadFile();
	
	if (synth.LoadFile(ref) == B_OK)
	{
		// Ideally, we would call SetVolume() in InitControls(), 
		// but for some reason that doesn't work; BMidiSynthFile
		// will use the default volume instead. So we do it here.
		synth.SetVolume(volume / 100.0f);

		playButton->SetEnabled(true);
		scopeView->SetHaveFile(true);
	}
	else
	{
		(new BAlert(
			NULL, "Could not load song", "Okay", NULL, NULL,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();

		playButton->SetEnabled(false);
		scopeView->SetHaveFile(false);
	}
	
	Play();
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::Play()
{
	synth.Start();
	playButton->SetLabel("Stop");
	playing = true;
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::Stop(bool changeButton)
{
	if (changeButton)
	{
		playButton->SetEnabled(false);
	}
		
	synth.Fade();
	
	if (changeButton)
	{
		playButton->SetEnabled(true);
		playButton->SetLabel("Play");
	}

	playing = false;
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::OnPlayStop()
{
	if (playing)
	{
		Stop(true);
	}
	else
	{
		Play();
	}
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::OnShowScope()
{
	scopeEnabled = !scopeEnabled;
	scopeView->SetEnabled(scopeEnabled);
	SaveSettings();
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::OnReverb(reverb_mode mode)
{
	reverb = mode;
	be_synth->SetReverb(reverb);
	SaveSettings();
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::OnVolume()
{
	volume = volumeSlider->Value();
	synth.SetVolume(volume / 100.0f);
	SaveSettings();
}

//------------------------------------------------------------------------------

void MidiPlayerWindow::OnDrop(BMessage* msg)
{
	entry_ref ref;
	if (msg->FindRef("refs", &ref) == B_OK)
	{
		LoadFile(&ref);
	}
}

//------------------------------------------------------------------------------
