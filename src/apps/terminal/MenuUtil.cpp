/*
 * Copyright (c) 2001-2009, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Kian Duffy <myob@users.sourceforge.net>
 */

#include "MenuUtil.h"

#include <stdio.h>
#include <string.h>

#include <Font.h>
#include <Menu.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include "Coding.h"
#include "PrefHandler.h"
#include "TermConst.h"


BPopUpMenu *
MakeMenu(ulong msg, const char **items, const char *defaultItemName)
{
	BPopUpMenu *menu = new BPopUpMenu("");

	int32 i = 0;
	while (*items) {
		if (!strcmp(*items, ""))
			menu->AddSeparatorItem();
		else
			menu->AddItem(new BMenuItem(*items, new BMessage(msg)));
		if (!strcmp(*items, defaultItemName))
			menu->ItemAt(i)->SetMarked(true);

		items++;
		i++;
	}
	return menu;
}


void
MakeEncodingMenu(BMenu *eMenu, bool withShortcuts)
{
	int encoding;
	int i = 0;
	while (get_nth_encoding(i, &encoding) == B_OK) {
		BMessage *msg = new BMessage(MENU_ENCODING);
		msg->AddInt32("op", (int32)encoding);
		if (withShortcuts) {
			eMenu->AddItem(new BMenuItem(EncodingAsString(encoding), msg,
				id2shortcut(encoding)));
		} else 
			eMenu->AddItem(new BMenuItem(EncodingAsString(encoding), msg));

		i++;
	}
}


void
LoadLocaleFile(PrefHandler *pref)
{
	char name[B_PATH_NAME_LENGTH];
	const char *locale;
	
	locale = PrefHandler::Default()->getString(PREF_GUI_LANGUAGE);
	// TODO: this effectively disables any locale support - which is okay for now
	sprintf(name, "%s%s", /*LOCALE_FILE_DIR*/"", locale);

	//if (pref->OpenText(name) < B_OK)
	//	pref->OpenText(LOCALE_FILE_DEFAULT);
}
