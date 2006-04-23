/*
 * MainApp.h - Media Player for the Haiku Operating System
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
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

class MainApp : public BApplication
{
public:
					MainApp();
					~MainApp();

	BWindow *		NewWindow();

private:
	void			RefsReceived(BMessage *msg);
	void			ArgvReceived(int32 argc, char **argv);

private:
	BWindow *		fFirstWindow;
};

extern MainApp *gMainApp;

#endif
