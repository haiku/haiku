/*
 * Copyright 2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef _INTERFACE_UTILS__H
#define _INTERFACE_UTILS__H

#include <SupportDefs.h>

class DialUpAddon;
class BListView;
class BMenu;
class BString;
class BWindow;


extern BPoint center_on_screen(BRect rect, BWindow *window = NULL);
extern int32 FindNextMenuInsertionIndex(BMenu *menu, const char *name,
	int32 index = 0);
extern int32 FindNextListInsertionIndex(BListView *list, const char *name);
extern void AddAddonsToMenu(const BMessage *source, BMenu *menu, const char *type,
	uint32 what);


#endif
