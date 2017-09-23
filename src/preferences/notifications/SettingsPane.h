/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SETTINGS_PANE_H
#define _SETTINGS_PANE_H

#include <View.h>

class BNode;

class SettingsHost;


class SettingsPane : public BView {
public:
							SettingsPane(const char* name, SettingsHost* host);

	void					SettingsChanged(bool showExample);

	virtual status_t		Load(BMessage&) = 0;
	virtual	status_t		Save(BMessage&) = 0;
	virtual	status_t		Revert() = 0;
	virtual bool			RevertPossible() = 0;
	virtual status_t		Defaults() = 0;
	virtual bool			DefaultsPossible() = 0;
	virtual bool			UseDefaultRevertButtons() = 0;

protected:
			SettingsHost*	fHost;
};

#endif // _SETTINGS_PANE_H
