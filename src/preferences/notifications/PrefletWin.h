/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PREFLET_WIN_H
#define _PREFLET_WIN_H

#include <Window.h>

#include "SettingsHost.h"

class BButton;

class PrefletView;

class PrefletWin : public BWindow, public SettingsHost {
public:
							PrefletWin();

	virtual	bool			QuitRequested();
	virtual	void			MessageReceived(BMessage* msg);

	virtual	void			SettingChanged();

private:
			PrefletView*	fMainView;
			BButton*		fSave;
			BButton*		fRevert;
};

#endif // _PREFLET_WIN_H
