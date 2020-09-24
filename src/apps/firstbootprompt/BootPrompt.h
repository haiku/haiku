/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2020, Panagiotis Vasilopoulos <hello@alwayslivid.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BOOT_PROMPT_APP_H
#define BOOT_PROMPT_APP_H

#include <Application.h>

#include "BootPromptWindow.h"

enum {
	MSG_BOOT_DESKTOP	= 'dktp',
	MSG_RUN_INSTALLER	= 'inst',
	MSG_REBOOT_REQUESTED	= 'rebt'
};

extern const char* kAppSignature;
extern const char* kDeskbarSignature;


class BootPromptApp : public BApplication {
public:
								BootPromptApp();

	virtual	void				MessageReceived(BMessage* message);
			bool				QuitRequested();
	virtual	void				ReadyToRun();
};


#endif // BOOT_PROMPT_APP_H
