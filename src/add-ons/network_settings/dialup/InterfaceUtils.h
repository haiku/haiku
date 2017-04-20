/*
 * Copyright 2003-2004 Waldemar Kornewald. All rights reserved.
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _INTERFACE_UTILS__H
#define _INTERFACE_UTILS__H

#include <SupportDefs.h>
#include <Point.h>

class DialUpAddon;
class BListView;
class BMenu;
class BString;
class BMenu;
class BWindow;
class BMessage;


extern BPoint center_on_screen(BRect rect, BWindow *window = NULL);
extern int32 FindNextMenuInsertionIndex(BMenu *menu, const char *name,
	int32 index = 0);
extern int32 FindNextListInsertionIndex(BListView *list, const char *name);
extern void AddAddonsToMenu(const BMessage *source, BMenu *menu, const char *type,
	uint32 what);


#endif
