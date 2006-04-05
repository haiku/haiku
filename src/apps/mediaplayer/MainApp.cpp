/*
 * MainApp.cpp - Media Player for the Haiku Operating System
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
#include <Path.h>
#include <Entry.h>
#include <Alert.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "MainApp.h"
#include "config.h"
#include "DeviceRoster.h"

MainApp *gMainApp;

MainApp::MainApp()
 :	BApplication(APP_SIG)
{
	InitPrefs();

	gDeviceRoster = new DeviceRoster;
	
	fMainWindow = NewWindow();
}


MainApp::~MainApp()
{
	delete gDeviceRoster;
}


status_t
MainApp::InitPrefs()
{
	return B_OK;
}


BWindow *
MainApp::NewWindow()
{
	static int i = 0;
	BRect rect(200, 200, 750, 300);
	rect.OffsetBy(i * 25, i * 25);
	i = (i + 1) % 15;
	BWindow *win = new MainWin(rect);
	win->Show();
	return win;
}


int 
main()
{
	gMainApp = new MainApp;
	gMainApp->Run();
	delete gMainApp;
	return 0;
}
