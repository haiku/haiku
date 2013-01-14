/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef GENERIC_SETTINGS_VIEW_H
#define GENERIC_SETTINGS_VIEW_H


#include <View.h>


class SettingsFile;

class BTextControl;


class GenericSettingsView : public BView {
public:
						GenericSettingsView(BRect frame, const char* name,
							SettingsFile* settings,
							uint32 resizingMode = B_FOLLOW_ALL,
							uint32 flags = 0);
						~GenericSettingsView();

	void				ApplySettings();
	void				AdaptSettings();

private:
	SettingsFile*		fSettings;

	BTextControl*		fUndoLimitText;
	// TODO: Add more controls for generic settings (1/4).

};


#endif
