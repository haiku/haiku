/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 */
#ifndef _APPLICATION_H
#define _APPLICATION_H

#include <Messenger.h>


struct app_info;


class BApplication {
public:
							BApplication(const char* signature);
							BApplication(const char* signature,
										 status_t* error);
	virtual					~BApplication();

	status_t				GetAppInfo(app_info* info);
};

// Global Objects

extern _IMPEXP_BE BApplication*	be_app;
extern _IMPEXP_BE BMessenger be_app_messenger;

#endif	// _APPLICATION_H
