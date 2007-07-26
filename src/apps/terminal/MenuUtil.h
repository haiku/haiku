/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Kian Duffy <myob@users.sourceforge.net>
 */
#ifndef MENUUTIL_H_INCLUDED
#define MENUUTIL_H_INCLUDED

#include <SupportDefs.h>


class BPopUpMenu;
class BMenu;
class PrefHandler;
  
BPopUpMenu *	MakeMenu(ulong msg, const char **items, 
  						const char *defaultItemName);

void			MakeEncodingMenu(BMenu *eMenu, bool flag);
void			LoadLocaleFile (PrefHandler *);



#endif
