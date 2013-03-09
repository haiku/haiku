/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H


#include <Window.h>


class GenericSettingsView;
class SettingsFile;

class BButton;
class BListView;
class BScrollView;
class BView;


class SettingsWindow : public BWindow {
public:
							SettingsWindow(SettingsFile* settings);
							~SettingsWindow();

	bool					QuitRequested();
	void					MessageReceived(BMessage* msg);

	void					ApplySettings();
	void					AdaptSettings();

private:
	SettingsFile*			fSettings;

	BView*					fBackView;
	BListView*				fListView;
	BScrollView*			fScrollView;

	GenericSettingsView*	fGenericSettingsView;
	// TODO: Add more settings (1/4).

	BButton*				fRevertButton;
	BButton*				fApplyButton;

};


#endif
