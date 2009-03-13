/*
 * Copyright (c) 2001-2009, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Kian Duffy <myob@users.sourceforge.net>
 */
#ifndef MENU_UTIL_H
#define MENU_UTIL_H

#include <SupportDefs.h>


class BPopUpMenu;
class BMenu;
class PrefHandler;
  
BPopUpMenu* MakeMenu(ulong msg, const char** items,
	const char* defaultItemName);
void MakeEncodingMenu(BMenu *eMenu, bool withShortcuts);
void LoadLocaleFile(PrefHandler* handler);

#endif	// MENU_UTIL_H
