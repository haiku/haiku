/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#ifndef APP_H
#define APP_H

#include <Application.h>
#include <FilePanel.h>

enum {
	M_SHOW_OPEN_PANEL,
	M_SHOW_SAVE_PANEL
};

class App : public BApplication
{
public:
			App(void);
			~App(void);
	void	MessageReceived(BMessage *msg);
	void	ArgvReceived(int32 argc, char** argv);
	void	RefsReceived(BMessage *msg);
	void	ReadyToRun(void);
	
private:
	BFilePanel	*fOpenPanel;
};

#endif
