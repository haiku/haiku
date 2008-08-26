/*
 * MainApp.h - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 * Copyright (C) 2008 Stephan Aßmus <superstippi@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#ifndef __MAIN_APP_H
#define __MAIN_APP_H

#include <Application.h>
#include "MainWin.h"

enum  {
	M_PLAYER_QUIT				= 'plqt',
	M_SETTINGS					= 'stng',

	M_MEDIA_SERVER_STARTED		= 'msst',
	M_MEDIA_SERVER_QUIT			= 'msqt'
};

class SettingsWindow;

class MainApp : public BApplication {
public:
								MainApp();
	virtual						~MainApp();

			BWindow*			NewWindow();
			int32				PlayerCount() const;

private:
	virtual	bool				QuitRequested();
	virtual	void				ReadyToRun();
	virtual	void				RefsReceived(BMessage* message);
	virtual	void				ArgvReceived(int32 argc, char** argv);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AboutRequested();

private:
			void				_ShowSettingsWindow();
			void				_BroadcastMessage(const BMessage& message);

			int32				fPlayerCount;
			BWindow*			fFirstWindow;
			SettingsWindow*		fSettingsWindow;

			bool				fMediaServerRunning;
			bool				fMediaAddOnServerRunning;
};

extern MainApp* gMainApp;
extern const char* kAppSig;

#endif
