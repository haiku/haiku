/*
 * Copyright 2014, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef MIDIVIEW_H_
#define MIDIVIEW_H_


#include "MediaViews.h"

class BButton;
class BListView;
class MidiSettingsView : public SettingsView {
public:
	MidiSettingsView();
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);

private:
	void _RetrieveSoftSynthList();
	void _LoadSettings();
	void _SaveSettings();

	BListView* fListView;
};

#endif /* MIDIVIEW_H_ */
