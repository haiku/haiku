/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef BOOT_PROMPT_APP_H
#define BOOT_PROMPT_APP_H

#include <Application.h>


enum {
	MSG_BOOT_DESKTOP	= 'dktp',
	MSG_RUN_INSTALLER	= 'inst'
};

extern const char* kAppSignature;


class BootPromptApp : public BApplication {
public:
								BootPromptApp();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				ReadyToRun();
};

#endif // BOOT_PROMPT_APP_H
