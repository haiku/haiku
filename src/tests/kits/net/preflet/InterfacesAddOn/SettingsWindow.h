/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Alexander von Gluck, kallisti5@unixzen.com
 */


#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include "NetworkSettings.h"

#include <Button.h>
#include <Catalog.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <TabView.h>
#include <Window.h>


enum {
	APPLY_MSG = 'aply',
	CANCEL_MSG = 'cncl'
};


class SettingsWindow : public BWindow {
public:
								SettingsWindow(NetworkSettings* settings);
	virtual						~SettingsWindow();
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* mesage);

private:
			status_t			_PopulateTabs();

			NetworkSettings*	fNetworkSettings;
			BButton*			fApplyButton;
			BButton*			fCancelButton;
			BTabView*			fTabView;
};


#endif  /* SETTINGS_WINDOW_H */

