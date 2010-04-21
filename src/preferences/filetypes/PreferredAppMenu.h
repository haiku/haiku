/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PREFERRED_APP_MENU_H
#define PREFERRED_APP_MENU_H


#include <SupportDefs.h>

class BMenu;
class BMessage;
class BMimeType;
class BString;

void update_preferred_app_menu(BMenu* menu, BMimeType* type, uint32 what,
	const char* preferredFrom = NULL);

status_t retrieve_preferred_app(BMessage* message, bool sameAs,
	const char* forType, BString& preferredApp);

#endif	// PREFERRED_APP_MENU_H
