/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PREFLET_WIN_H
#define _PREFLET_WIN_H


#include <GroupView.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <Window.h>

#include "SettingsHost.h"

class BButton;

class PrefletView;

class PrefletWin : public BWindow, public SettingsHost {
public:
							PrefletWin();

	virtual	bool			QuitRequested();
	virtual	void			MessageReceived(BMessage* msg);

	virtual	void			SettingChanged(bool showExample);
			void			ReloadSettings();

private:
			status_t		_Revert();
			bool			_RevertPossible();
			status_t		_Defaults();
			bool			_DefaultsPossible();
			void			_SendExampleNotification();
			
			PrefletView*	fMainView;
			BGroupView*		fButtonsView;
			BButton*		fDefaults;
			BButton*		fRevert;
			BGroupLayout*	fButtonsLayout;
};

#endif // _PREFLET_WIN_H
