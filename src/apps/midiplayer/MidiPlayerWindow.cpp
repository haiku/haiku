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

#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <MidiProducer.h>
#include <MidiRoster.h>
#include <StorageKit.h>
#include <SpaceLayoutItem.h>

#include "MidiPlayerApp.h"
#include "MidiPlayerWindow.h"
#include "ScopeView.h"
#include "SynthBridge.h"


#define _W(a) (a->Frame().Width())
#define _H(a) (a->Frame().Height())


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Main Window"


MidiPlayerWindow::MidiPlayerWindow()
	:
	BWindow(BRect(0, 0, 1, 1), B_TRANSLATE_SYSTEM_NAME("MidiPlayer"),
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE
		| B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	playing = false;
	scopeEnabled = true;
	reverb = B_REVERB_BALLROOM;
	volume = 75;
	windowX = -1;
	windowY = -1;
	inputId = -1;
	instrLoaded = false;

	be_synth->SetSamplingRate(44100);

	bridge = new SynthBridge;
	//bridge->Register();

	CreateViews();
	LoadSettings();
	InitControls();
}


MidiPlayerWindow::~MidiPlayerWindow()
{
	StopSynth();

	//bridge->Unregister();
	bridge->Release();
}


bool
MidiPlayerWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MidiPlayerWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_PLAY_STOP:
			OnPlayStop();
			break;

		case MSG_SHOW_SCOPE:
			OnShowScope();
			break;

		case MSG_INPUT_CHANGED:
			OnInputChanged(msg);
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


void
MidiPlayerWindow::FrameMoved(BPoint origin)
{
	super::FrameMoved(origin);
	windowX = Frame().left;
	windowY = Frame().top;
	SaveSettings();
}


void
MidiPlayerWindow::MenusBeginning()
{
	for (int32 t = inputPopUp->CountItems() - 1; t > 0; --t)
		delete inputPopUp->RemoveItem(t);

	// Note: if the selected endpoint no longer exists, then no endpoint is
	// marked. However, we won't disconnect it until you choose another one.

	inputOff->SetMarked(inputId == -1);

	int32 id = 0;
	while (BMidiEndpoint* endpoint = BMidiRoster::NextEndpoint(&id)) {
		if (endpoint->IsProducer()) {
			BMessage* msg = new BMessage(MSG_INPUT_CHANGED);
			msg->AddInt32("id", id);

			BMenuItem* item = new BMenuItem(endpoint->Name(), msg);
			inputPopUp->AddItem(item);
			item->SetMarked(inputId == id);
		}

		endpoint->Release();
	}
}


void
MidiPlayerWindow::CreateInputMenu()
{
	inputPopUp = new BPopUpMenu("inputPopUp");

	BMessage* msg = new BMessage;
	msg->what = MSG_INPUT_CHANGED;
	msg->AddInt32("id", -1);

	inputOff = new BMenuItem(B_TRANSLATE("Off"), msg);

	inputPopUp->AddItem(inputOff);

	inputMenu = new BMenuField(B_TRANSLATE("Live input:"), inputPopUp);
}


void
MidiPlayerWindow::CreateReverbMenu()
{
	BPopUpMenu* reverbPopUp = new BPopUpMenu("reverbPopUp");
	reverbNone = new BMenuItem(
		B_TRANSLATE("None"), new BMessage(MSG_REVERB_NONE));
	reverbCloset = new BMenuItem(
		B_TRANSLATE("Closet"), new BMessage(MSG_REVERB_CLOSET));
	reverbGarage = new BMenuItem(
		B_TRANSLATE("Garage"), new BMessage(MSG_REVERB_GARAGE));
	reverbIgor = new BMenuItem(
		B_TRANSLATE("Igor's lab"), new BMessage(MSG_REVERB_IGOR));
	reverbCavern = new BMenuItem(
		B_TRANSLATE("Cavern"), new BMessage(MSG_REVERB_CAVERN));
	reverbDungeon = new BMenuItem(
		B_TRANSLATE("Dungeon"), new BMessage(MSG_REVERB_DUNGEON));

	reverbPopUp->AddItem(reverbNone);
	reverbPopUp->AddItem(reverbCloset);
	reverbPopUp->AddItem(reverbGarage);
	reverbPopUp->AddItem(reverbIgor);
	reverbPopUp->AddItem(reverbCavern);
	reverbPopUp->AddItem(reverbDungeon);

	reverbMenu = new BMenuField(B_TRANSLATE("Reverb:"), reverbPopUp);
}


void
MidiPlayerWindow::CreateViews()
{
	// Set up needed views
	scopeView = new ScopeView;

	showScope = new BCheckBox("showScope", B_TRANSLATE("Scope"),
		new BMessage(MSG_SHOW_SCOPE));
	showScope->SetValue(B_CONTROL_ON);

	CreateInputMenu();
	CreateReverbMenu();

	volumeSlider = new BSlider("volumeSlider", NULL, NULL, 0, 100,
		B_HORIZONTAL);
	rgb_color col = { 152, 152, 255 };
	volumeSlider->UseFillColor(true, &col);
	volumeSlider->SetModificationMessage(new BMessage(MSG_VOLUME));

	playButton = new BButton("playButton", B_TRANSLATE("Play"),
		new BMessage(MSG_PLAY_STOP));
	playButton->SetEnabled(false);

	BBox* divider = new BBox(B_EMPTY_STRING, B_WILL_DRAW | B_FRAME_EVENTS,
		B_FANCY_BORDER);
	divider->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, 1));

	BStringView* volumeLabel = new BStringView(NULL, B_TRANSLATE("Volume:"));
	volumeLabel->SetAlignment(B_ALIGN_LEFT);
	volumeLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	// Build the layout
	SetLayout(new BGroupLayout(B_HORIZONTAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 10)
		.Add(scopeView)
		.Add(BGridLayoutBuilder(10, 10)
			.Add(BSpaceLayoutItem::CreateGlue(), 0, 0)
			.Add(showScope, 1, 0)

			.Add(reverbMenu->CreateLabelLayoutItem(), 0, 1)
			.Add(reverbMenu->CreateMenuBarLayoutItem(), 1, 1)

			.Add(inputMenu->CreateLabelLayoutItem(), 0, 2)
			.Add(inputMenu->CreateMenuBarLayoutItem(), 1, 2)

			.Add(volumeLabel, 0, 3)
			.Add(volumeSlider, 1, 3)
		)
		.AddGlue()
		.Add(divider)
		.AddGlue()
		.Add(playButton)
		.AddGlue()
		.SetInsets(5, 5, 5, 5)
	);
}


void
MidiPlayerWindow::InitControls()
{
	Lock();

	showScope->SetValue(scopeEnabled ? B_CONTROL_ON : B_CONTROL_OFF);
	scopeView->SetEnabled(scopeEnabled);

	inputOff->SetMarked(true);

	reverbNone->SetMarked(reverb == B_REVERB_NONE);
	reverbCloset->SetMarked(reverb == B_REVERB_CLOSET);
	reverbGarage->SetMarked(reverb == B_REVERB_GARAGE);
	reverbIgor->SetMarked(reverb == B_REVERB_BALLROOM);
	reverbCavern->SetMarked(reverb == B_REVERB_CAVERN);
	reverbDungeon->SetMarked(reverb == B_REVERB_DUNGEON);
	be_synth->SetReverb(reverb);

	volumeSlider->SetValue(volume);

	if (windowX != -1 && windowY != -1)
		MoveTo(windowX, windowY);
	else
		CenterOnScreen();

	Unlock();
}


void
MidiPlayerWindow::LoadSettings()
{
	BFile file(SETTINGS_FILE, B_READ_ONLY);
	if (file.InitCheck() != B_OK || file.Lock() != B_OK)
		return;

	file.ReadAttr("Scope", B_BOOL_TYPE, 0, &scopeEnabled, sizeof(bool));
	file.ReadAttr("Reverb", B_INT32_TYPE, 0, &reverb, sizeof(int32));
	file.ReadAttr("Volume", B_INT32_TYPE, 0, &volume, sizeof(int32));
	file.ReadAttr("WindowX", B_FLOAT_TYPE, 0, &windowX, sizeof(float));
	file.ReadAttr("WindowY", B_FLOAT_TYPE, 0, &windowY, sizeof(float));

	file.Unlock();
}


void
MidiPlayerWindow::SaveSettings()
{
	BFile file(SETTINGS_FILE, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (file.InitCheck() != B_OK || file.Lock() != B_OK)
		return;

	file.WriteAttr("Scope", B_BOOL_TYPE, 0, &scopeEnabled, sizeof(bool));
	file.WriteAttr("Reverb", B_INT32_TYPE, 0, &reverb, sizeof(int32));
	file.WriteAttr("Volume", B_INT32_TYPE, 0, &volume, sizeof(int32));
	file.WriteAttr("WindowX", B_FLOAT_TYPE, 0, &windowX, sizeof(float));
	file.WriteAttr("WindowY", B_FLOAT_TYPE, 0, &windowY, sizeof(float));

	file.Sync();
	file.Unlock();
}


void
MidiPlayerWindow::LoadFile(entry_ref* ref)
{
	if (playing) {
		scopeView->SetPlaying(false);
		scopeView->Invalidate();
		UpdateIfNeeded();

		StopSynth();
	}

	synth.UnloadFile();

	if (synth.LoadFile(ref) == B_OK) {
		// Ideally, we would call SetVolume() in InitControls(),
		// but for some reason that doesn't work: BMidiSynthFile
		// will use the default volume instead. So we do it here.
		synth.SetVolume(volume / 100.0f);

		playButton->SetEnabled(true);
		playButton->SetLabel(B_TRANSLATE("Stop"));
		scopeView->SetHaveFile(true);
		scopeView->SetPlaying(true);
		scopeView->Invalidate();

		StartSynth();
	} else {
		playButton->SetEnabled(false);
		playButton->SetLabel(B_TRANSLATE("Play"));
		scopeView->SetHaveFile(false);
		scopeView->SetPlaying(false);
		scopeView->Invalidate();

		BAlert* alert = new BAlert(NULL, B_TRANSLATE("Could not load song"),
			B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
	}
}


void
MidiPlayerWindow::StartSynth()
{
	synth.Start();
	synth.SetFileHook(_StopHook, (int32) this);
	playing = true;
}


void
MidiPlayerWindow::StopSynth()
{
	if (!synth.IsFinished())
		synth.Fade();

	playing = false;
}


void
MidiPlayerWindow::_StopHook(int32 arg)
{
	((MidiPlayerWindow*)arg)->StopHook();
}


void
MidiPlayerWindow::StopHook()
{
	Lock();
		// we may be called from the synth's thread

	playing = false;

	scopeView->SetPlaying(false);
	scopeView->Invalidate();
	playButton->SetEnabled(true);
	playButton->SetLabel(B_TRANSLATE("Play"));

	Unlock();
}


void
MidiPlayerWindow::OnPlayStop()
{
	if (playing) {
		playButton->SetEnabled(false);
		scopeView->SetPlaying(false);
		scopeView->Invalidate();
		UpdateIfNeeded();

		StopSynth();
	} else {
		playButton->SetLabel(B_TRANSLATE("Stop"));
		scopeView->SetPlaying(true);
		scopeView->Invalidate();

		StartSynth();
	}
}


void
MidiPlayerWindow::OnShowScope()
{
	scopeEnabled = !scopeEnabled;
	scopeView->SetEnabled(scopeEnabled);
	scopeView->Invalidate();
	SaveSettings();
}


void
MidiPlayerWindow::OnInputChanged(BMessage* msg)
{
	int32 newId;
	if (msg->FindInt32("id", &newId) == B_OK) {
		BMidiProducer* endpoint = BMidiRoster::FindProducer(inputId);
		if (endpoint != NULL) {
			endpoint->Disconnect(bridge);
			endpoint->Release();
		}

		inputId = newId;

		endpoint = BMidiRoster::FindProducer(inputId);
		if (endpoint != NULL) {
			if (!instrLoaded) {
				scopeView->SetLoading(true);
				scopeView->Invalidate();
				UpdateIfNeeded();

				bridge->Init(B_BIG_SYNTH);
				instrLoaded = true;

				scopeView->SetLoading(false);
				scopeView->Invalidate();
			}

			endpoint->Connect(bridge);
			endpoint->Release();

			scopeView->SetLiveInput(true);
			scopeView->Invalidate();
		} else {
			scopeView->SetLiveInput(false);
			scopeView->Invalidate();
		}
	}
}


void
MidiPlayerWindow::OnReverb(reverb_mode mode)
{
	reverb = mode;
	be_synth->SetReverb(reverb);
	SaveSettings();
}


void
MidiPlayerWindow::OnVolume()
{
	volume = volumeSlider->Value();
	synth.SetVolume(volume / 100.0f);
	SaveSettings();
}


void
MidiPlayerWindow::OnDrop(BMessage* msg)
{
	entry_ref ref;
	if (msg->FindRef("refs", &ref) == B_OK)
		LoadFile(&ref);
}
