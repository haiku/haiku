/*
 * Copyright (c) 2004 Matthijs Hollemans
 * Copyright (c) 2008-2014 Haiku, Inc. All rights reserved.
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


#include "MidiPlayerWindow.h"

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MidiProducer.h>
#include <MidiRoster.h>
#include <SeparatorView.h>
#include <StorageKit.h>
#include <SpaceLayoutItem.h>

#include "MidiPlayerApp.h"
#include "ScopeView.h"
#include "SynthBridge.h"


#define _W(a) (a->Frame().Width())
#define _H(a) (a->Frame().Height())


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Main Window"


//	#pragma mark - MidiPlayerWindow


MidiPlayerWindow::MidiPlayerWindow()
	:
	BWindow(BRect(0, 0, 1, 1), B_TRANSLATE_SYSTEM_NAME("MidiPlayer"),
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE
			| B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fIsPlaying = false;
	fScopeEnabled = true;
	fReverbMode = B_REVERB_BALLROOM;
	fVolume = 75;
	fWindowX = -1;
	fWindowY = -1;
	fInputId = -1;
	fInstrumentLoaded = false;

	be_synth->SetSamplingRate(44100);

	fSynthBridge = new SynthBridge;
	//fSynthBridge->Register();

	CreateViews();
	LoadSettings();
	InitControls();
}


MidiPlayerWindow::~MidiPlayerWindow()
{
	StopSynth();

	//fSynthBridge->Unregister();
	fSynthBridge->Release();
}


bool
MidiPlayerWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MidiPlayerWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PLAY_STOP:
			OnPlayStop();
			break;

		case MSG_SHOW_SCOPE:
			OnShowScope();
			break;

		case MSG_INPUT_CHANGED:
			OnInputChanged(message);
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
			OnDrop(message);
			break;

		default:
			super::MessageReceived(message);
			break;
	}
}


void
MidiPlayerWindow::FrameMoved(BPoint origin)
{
	super::FrameMoved(origin);
	fWindowX = Frame().left;
	fWindowY = Frame().top;
	SaveSettings();
}


void
MidiPlayerWindow::MenusBeginning()
{
	for (int32 t = fInputPopUpMenu->CountItems() - 1; t > 0; --t)
		delete fInputPopUpMenu->RemoveItem(t);

	// Note: if the selected endpoint no longer exists, then no endpoint is
	// marked. However, we won't disconnect it until you choose another one.

	fInputOffMenuItem->SetMarked(fInputId == -1);

	int32 id = 0;
	while (BMidiEndpoint* endpoint = BMidiRoster::NextEndpoint(&id)) {
		if (endpoint->IsProducer()) {
			BMessage* message = new BMessage(MSG_INPUT_CHANGED);
			message->AddInt32("id", id);

			BMenuItem* item = new BMenuItem(endpoint->Name(), message);
			fInputPopUpMenu->AddItem(item);
			item->SetMarked(fInputId == id);
		}

		endpoint->Release();
	}
}


void
MidiPlayerWindow::CreateInputMenu()
{
	fInputPopUpMenu = new BPopUpMenu("inputPopUp");

	BMessage* message = new BMessage(MSG_INPUT_CHANGED);
	message->AddInt32("id", -1);

	fInputOffMenuItem = new BMenuItem(B_TRANSLATE("Off"), message);
	fInputPopUpMenu->AddItem(fInputOffMenuItem);

	fInputMenuField = new BMenuField(B_TRANSLATE("Live input:"),
		fInputPopUpMenu);
}


void
MidiPlayerWindow::CreateReverbMenu()
{
	BPopUpMenu* reverbPopUpMenu = new BPopUpMenu("reverbPopUp");
	fReverbNoneMenuItem = new BMenuItem(B_TRANSLATE("None"),
		new BMessage(MSG_REVERB_NONE));
	fReverbClosetMenuItem = new BMenuItem(B_TRANSLATE("Closet"),
		new BMessage(MSG_REVERB_CLOSET));
	fReverbGarageMenuItem = new BMenuItem(B_TRANSLATE("Garage"),
		new BMessage(MSG_REVERB_GARAGE));
	fReverbIgorMenuItem = new BMenuItem(B_TRANSLATE("Igor's lab"),
		new BMessage(MSG_REVERB_IGOR));
	fReverbCavern = new BMenuItem(B_TRANSLATE("Cavern"),
		new BMessage(MSG_REVERB_CAVERN));
	fReverbDungeon = new BMenuItem(B_TRANSLATE("Dungeon"),
		new BMessage(MSG_REVERB_DUNGEON));

	reverbPopUpMenu->AddItem(fReverbNoneMenuItem);
	reverbPopUpMenu->AddItem(fReverbClosetMenuItem);
	reverbPopUpMenu->AddItem(fReverbGarageMenuItem);
	reverbPopUpMenu->AddItem(fReverbIgorMenuItem);
	reverbPopUpMenu->AddItem(fReverbCavern);
	reverbPopUpMenu->AddItem(fReverbDungeon);

	fReverbMenuField = new BMenuField(B_TRANSLATE("Reverb:"), reverbPopUpMenu);
}


void
MidiPlayerWindow::CreateViews()
{
	// Set up needed views
	fScopeView = new ScopeView();

	fShowScopeCheckBox = new BCheckBox("showScope", B_TRANSLATE("Scope"),
		new BMessage(MSG_SHOW_SCOPE));
	fShowScopeCheckBox->SetValue(B_CONTROL_ON);

	CreateInputMenu();
	CreateReverbMenu();

	fVolumeSlider = new BSlider("volumeSlider", NULL, NULL, 0, 100,
		B_HORIZONTAL);
	rgb_color color = (rgb_color){ 152, 152, 255 };
	fVolumeSlider->UseFillColor(true, &color);
	fVolumeSlider->SetModificationMessage(new BMessage(MSG_VOLUME));
	fVolumeSlider->SetExplicitMinSize(
		BSize(fScopeView->Frame().Width(), B_SIZE_UNSET));

	fPlayButton = new BButton("playButton", B_TRANSLATE("Play"),
		new BMessage(MSG_PLAY_STOP));
	fPlayButton->SetEnabled(false);

	BStringView* volumeLabel = new BStringView(NULL, B_TRANSLATE("Volume:"));
	volumeLabel->SetAlignment(B_ALIGN_LEFT);

	// Build the layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fScopeView)
		.AddGroup(B_VERTICAL, 0)
			.AddGrid(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
				.Add(fShowScopeCheckBox, 1, 0)

				.Add(fReverbMenuField->CreateLabelLayoutItem(), 0, 1)
				.AddGroup(B_HORIZONTAL, 0.0f, 1, 1)
					.Add(fReverbMenuField->CreateMenuBarLayoutItem())
					.AddGlue()
					.End()

				.Add(fInputMenuField->CreateLabelLayoutItem(), 0, 2)
				.AddGroup(B_HORIZONTAL, 0.0f, 1, 2)
					.Add(fInputMenuField->CreateMenuBarLayoutItem())
					.AddGlue()
					.End()

				.Add(volumeLabel, 0, 3)
				.Add(fVolumeSlider, 0, 4, 2, 1)
				.End()
			.AddGlue()
			.SetInsets(B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING)

			.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_VERTICAL, 0)
			.Add(fPlayButton)
			.SetInsets(0, B_USE_DEFAULT_SPACING, 0, B_USE_WINDOW_SPACING)
			.End()
		.End();
}


void
MidiPlayerWindow::InitControls()
{
	Lock();

	fShowScopeCheckBox->SetValue(fScopeEnabled ? B_CONTROL_ON : B_CONTROL_OFF);
	fScopeView->SetEnabled(fScopeEnabled);

	fInputOffMenuItem->SetMarked(true);

	fReverbNoneMenuItem->SetMarked(fReverbMode == B_REVERB_NONE);
	fReverbClosetMenuItem->SetMarked(fReverbMode == B_REVERB_CLOSET);
	fReverbGarageMenuItem->SetMarked(fReverbMode == B_REVERB_GARAGE);
	fReverbIgorMenuItem->SetMarked(fReverbMode == B_REVERB_BALLROOM);
	fReverbCavern->SetMarked(fReverbMode == B_REVERB_CAVERN);
	fReverbDungeon->SetMarked(fReverbMode == B_REVERB_DUNGEON);
	be_synth->SetReverb(fReverbMode);

	fVolumeSlider->SetValue(fVolume);

	if (fWindowX != -1 && fWindowY != -1)
		MoveTo(fWindowX, fWindowY);
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

	file.ReadAttr("Scope", B_BOOL_TYPE, 0, &fScopeEnabled, sizeof(bool));
	file.ReadAttr("Reverb", B_INT32_TYPE, 0, &fReverbMode, sizeof(int32));
	file.ReadAttr("Volume", B_INT32_TYPE, 0, &fWindowX, sizeof(int32));
	file.ReadAttr("WindowX", B_FLOAT_TYPE, 0, &fWindowX, sizeof(float));
	file.ReadAttr("WindowY", B_FLOAT_TYPE, 0, &fWindowY, sizeof(float));

	file.Unlock();
}


void
MidiPlayerWindow::SaveSettings()
{
	BFile file(SETTINGS_FILE, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (file.InitCheck() != B_OK || file.Lock() != B_OK)
		return;

	file.WriteAttr("Scope", B_BOOL_TYPE, 0, &fScopeEnabled, sizeof(bool));
	file.WriteAttr("Reverb", B_INT32_TYPE, 0, &fReverbMode, sizeof(int32));
	file.WriteAttr("Volume", B_INT32_TYPE, 0, &fVolume, sizeof(int32));
	file.WriteAttr("WindowX", B_FLOAT_TYPE, 0, &fWindowX, sizeof(float));
	file.WriteAttr("WindowY", B_FLOAT_TYPE, 0, &fWindowY, sizeof(float));

	file.Sync();
	file.Unlock();
}


void
MidiPlayerWindow::LoadFile(entry_ref* ref)
{
	if (fIsPlaying) {
		fScopeView->SetPlaying(false);
		fScopeView->Invalidate();
		UpdateIfNeeded();

		StopSynth();
	}

	fMidiSynthFile.UnloadFile();

	if (fMidiSynthFile.LoadFile(ref) == B_OK) {
		// Ideally, we would call SetVolume() in InitControls(),
		// but for some reason that doesn't work: BMidiSynthFile
		// will use the default volume instead. So we do it here.
		fMidiSynthFile.SetVolume(fVolume / 100.0f);

		fPlayButton->SetEnabled(true);
		fPlayButton->SetLabel(B_TRANSLATE("Stop"));
		fScopeView->SetHaveFile(true);
		fScopeView->SetPlaying(true);
		fScopeView->Invalidate();

		StartSynth();
	} else {
		fPlayButton->SetEnabled(false);
		fPlayButton->SetLabel(B_TRANSLATE("Play"));
		fScopeView->SetHaveFile(false);
		fScopeView->SetPlaying(false);
		fScopeView->Invalidate();

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
	fMidiSynthFile.Start();
	fMidiSynthFile.SetFileHook(_StopHook, (int32)(addr_t)this);
	fIsPlaying = true;
}


void
MidiPlayerWindow::StopSynth()
{
	if (!fMidiSynthFile.IsFinished())
		fMidiSynthFile.Fade();

	fIsPlaying = false;
}


void
MidiPlayerWindow::_StopHook(int32 arg)
{
	((MidiPlayerWindow*)(addr_t)arg)->StopHook();
}


void
MidiPlayerWindow::StopHook()
{
	if (Lock()) {
		// we may be called from the synth's thread

		fIsPlaying = false;

		fScopeView->SetPlaying(false);
		fScopeView->Invalidate();
		fPlayButton->SetEnabled(true);
		fPlayButton->SetLabel(B_TRANSLATE("Play"));

		Unlock();
	}
}


void
MidiPlayerWindow::OnPlayStop()
{
	if (fIsPlaying) {
		fPlayButton->SetEnabled(false);
		fScopeView->SetPlaying(false);
		fScopeView->Invalidate();
		UpdateIfNeeded();

		StopSynth();
	} else {
		fPlayButton->SetLabel(B_TRANSLATE("Stop"));
		fScopeView->SetPlaying(true);
		fScopeView->Invalidate();

		StartSynth();
	}
}


void
MidiPlayerWindow::OnShowScope()
{
	fScopeEnabled = !fScopeEnabled;
	fScopeView->SetEnabled(fScopeEnabled);
	fScopeView->Invalidate();
	SaveSettings();
}


void
MidiPlayerWindow::OnInputChanged(BMessage* message)
{
	int32 newId;
	if (message->FindInt32("id", &newId) == B_OK) {
		BMidiProducer* endpoint = BMidiRoster::FindProducer(fInputId);
		if (endpoint != NULL) {
			endpoint->Disconnect(fSynthBridge);
			endpoint->Release();
		}

		fInputId = newId;

		endpoint = BMidiRoster::FindProducer(fInputId);
		if (endpoint != NULL) {
			if (!fInstrumentLoaded) {
				fScopeView->SetLoading(true);
				fScopeView->Invalidate();
				UpdateIfNeeded();

				fSynthBridge->Init(B_BIG_SYNTH);
				fInstrumentLoaded = true;

				fScopeView->SetLoading(false);
				fScopeView->Invalidate();
			}

			endpoint->Connect(fSynthBridge);
			endpoint->Release();

			fScopeView->SetLiveInput(true);
			fScopeView->Invalidate();
		} else {
			fScopeView->SetLiveInput(false);
			fScopeView->Invalidate();
		}
	}
}


void
MidiPlayerWindow::OnReverb(reverb_mode mode)
{
	fReverbMode = mode;
	be_synth->SetReverb(fReverbMode);
	SaveSettings();
}


void
MidiPlayerWindow::OnVolume()
{
	fVolume = fVolumeSlider->Value();
	fMidiSynthFile.SetVolume(fVolume / 100.0f);
	SaveSettings();
}


void
MidiPlayerWindow::OnDrop(BMessage* message)
{
	entry_ref ref;
	if (message->FindRef("refs", &ref) == B_OK)
		LoadFile(&ref);
}
