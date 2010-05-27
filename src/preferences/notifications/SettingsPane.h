/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SETTINGS_PANE_H
#define _SETTINGS_PANE_H

#include <View.h>

class BNode;

class SettingsHost;

const int32 kSettingChanged = '_STC';

class SettingsPane : public BView {
public:
							SettingsPane(const char* name, SettingsHost* host);

	virtual	void			MessageReceived(BMessage* msg);

	virtual status_t		Load() = 0;
	virtual	status_t		Save() = 0;
	virtual	status_t		Revert() = 0;

protected:
			SettingsHost*	fHost;
};

#endif // _SETTINGS_PANE_H
