/*
 * Copyright 2014-2016, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef MIDIVIEW_H_
#define MIDIVIEW_H_


#include "MediaViews.h"

class BButton;
class BListView;
class BString;
class BStringView;

class MidiSettingsView : public SettingsView {
public:
					MidiSettingsView();

	virtual void	AttachedToWindow();
	virtual void	DetachedFromWindow();
	virtual void	MessageReceived(BMessage* message);

private:
	void 			_RetrieveSoundFontList();
	void 			_LoadSettings();
	void 			_SaveSettings();
	void 			_WatchFolders();
	void			_SelectActiveSoundFont();
	BString			_SelectedSoundFont() const;
	void			_UpdateSoundFontStatus();

	BListView*		fListView;
	BString*		fActiveSoundFont;
	BStringView*	fSoundFontStatus;
};

#endif /* MIDIVIEW_H_ */
