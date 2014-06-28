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
#ifndef _MIDI_PLAYER_WINDOW_H
#define _MIDI_PLAYER_WINDOW_H


#include <InterfaceKit.h>
#include <MidiSynthFile.h>


#define SETTINGS_FILE "/boot/home/config/settings/MidiPlayerSettings"


enum
{
	MSG_PLAY_STOP = 1000,
	MSG_SHOW_SCOPE,
	MSG_INPUT_CHANGED,
	MSG_REVERB_NONE,
	MSG_REVERB_CLOSET,
	MSG_REVERB_GARAGE,
	MSG_REVERB_IGOR,
	MSG_REVERB_CAVERN,
	MSG_REVERB_DUNGEON,
	MSG_VOLUME,
};


class ScopeView;
class SynthBridge;

class MidiPlayerWindow : public BWindow {
public:
	MidiPlayerWindow();
	virtual ~MidiPlayerWindow();

	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage* message);
	virtual void FrameMoved(BPoint origin);
	virtual void MenusBeginning();

private:
	typedef BWindow super;

	void CreateInputMenu();
	void CreateReverbMenu();
	void CreateViews();
	void InitControls();
	void LoadSettings();
	void SaveSettings();

	void LoadFile(entry_ref* ref);
	void StartSynth();
	void StopSynth();

	static void _StopHook(int32 arg);
	void StopHook();

	void OnPlayStop();
	void OnShowScope();
	void OnInputChanged(BMessage* message);
	void OnReverb(reverb_mode mode);
	void OnVolume();
	void OnDrop(BMessage* message);

	ScopeView* fScopeView;
	BCheckBox* fShowScopeCheckBox;
	BMenuField* fInputMenuField;
	BPopUpMenu* fInputPopUpMenu;
	BMenuItem* fInputOffMenuItem;
	BMenuField* fReverbMenuField;
	BMenuItem* fReverbNoneMenuItem;
	BMenuItem* fReverbClosetMenuItem;
	BMenuItem* fReverbGarageMenuItem;
	BMenuItem* fReverbIgorMenuItem;
	BMenuItem* fReverbCavern;
	BMenuItem* fReverbDungeon;
	BSlider* fVolumeSlider;
	BButton* fPlayButton;

	bool fIsPlaying;
	bool fScopeEnabled;
	int32 fInputId;
	reverb_mode fReverbMode;
	int32 fVolume;
	float fWindowX;
	float fWindowY;
	BMidiSynthFile fMidiSynthFile;
	SynthBridge* fSynthBridge;
	bool fInstrumentLoaded;
};


#endif // _MIDI_PLAYER_WINDOW_H
