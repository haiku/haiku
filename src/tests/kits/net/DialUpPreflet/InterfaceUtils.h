//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _INTERFACE_UTILS__H
#define _INTERFACE_UTILS__H

#include <SupportDefs.h>

class DialUpAddon;
class BListView;
class BMenu;
class BString;


extern int32 FindNextMenuInsertionIndex(BMenu *menu, const BString& name,
	int32 index = 0);
extern int32 FindNextListInsertionIndex(BListView *list, const BString& name);
extern void AddAddonsToMenu(const BMessage *source, BMenu *menu, const char *type,
	uint32 what);


#endif
