/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef RESOURCE_EDIT_H
#define RESOURCE_EDIT_H


#include <Application.h>


class MainWindow;
class SettingsFile;
class SettingsWindow;

class BEntry;
class BFilePanel;
class BMessage;


class ResourceEdit : public BApplication {
public:
					ResourceEdit();
					~ResourceEdit();

	void			MessageReceived(BMessage* msg);

private:
	BRect			fCascade;
	uint32			fCascadeCount;
	BList			fWindowList;

	BFilePanel*		fOpenPanel;

	SettingsFile*	fSettings;
	SettingsWindow*	fSettingsWindow;

	void			ArgvReceived(int32 argc, char* argv[]);
	void			ReadyToRun();

	void			_CreateWindow(BEntry* assocEntry);
	BRect			_Cascade();

};


#endif
