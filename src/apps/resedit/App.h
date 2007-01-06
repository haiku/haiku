/*
 * Copyright (c) 2005-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@earthlink.net>
 */
#ifndef APP_H
#define APP_H

#include <Application.h>
#include <FilePanel.h>

enum {
	M_REGISTER_WINDOW = 'regw',
	M_UNREGISTER_WINDOW,
	M_SHOW_OPEN_PANEL,
	M_SHOW_SAVE_PANEL
};

class App : public BApplication
{
public:
			App(void);
			~App(void);
	void	MessageReceived(BMessage *msg);
	void	RefsReceived(BMessage *msg);
	void	ReadyToRun(void);
	bool	QuitRequested(void);
	
private:
	uint32		fWindowCount;
	BFilePanel	*fOpenPanel, *fSavePanel;
};

#endif
