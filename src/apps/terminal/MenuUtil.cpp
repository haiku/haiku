/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Kian Duffy <myob@users.sourceforge.net>
 */
#include <Menu.h>
#include <string.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Font.h>
#include <stdio.h>

#include "MenuUtil.h"
#include "TermConst.h"
#include "PrefHandler.h"
#include "Coding.h"

extern PrefHandler *gTermPref;

//#define LOCALE_FILE_DIR PREF_FOLDER"menu/"

BPopUpMenu *
MakeMenu(ulong msg, const char **items, const char *defaultItemName)
{
	BPopUpMenu *menu = new BPopUpMenu("");

	int32 i = 0;
	while (*items) {
		menu->AddItem(new BMenuItem(*items, new BMessage(msg)));
		if (!strcmp(*items, defaultItemName))
			menu->ItemAt(i)->SetMarked(true);

		items++;
		i++;
	}
	return menu;
}

int
longname2op(const char *longname)
{
	int op = M_UTF8;
	const etable *s = encoding_table;
	
	for (int i = 0; s->name; s++, i++) {
		if (!strcmp(s->name, longname)) {
			op = s->op;
			break;
		}
	}
	return op;
}

const char *
op2longname(int op)
{
	return encoding_table[op].name;
}

void
MakeEncodingMenu(BMenu *eMenu, int coding, bool flag)
{
	const etable *e = encoding_table;
	int i = 0;
	while (e->name) {
		BMessage *msg = new BMessage(MENU_ENCODING);
		msg->AddInt32("op", (int32)e->op);
		if (flag)
			eMenu->AddItem(new BMenuItem(e->name, msg, e->shortcut));
		else 
			eMenu->AddItem(new BMenuItem(e->name, msg));
		
		if (i == coding)
			eMenu->ItemAt(i)->SetMarked(true);
		
		e++;
		i++;
	}
}

void
LoadLocaleFile(PrefHandler *pref)
{
	char name[B_PATH_NAME_LENGTH];
	const char *locale;
	
	locale = gTermPref->getString(PREF_GUI_LANGUAGE);
	// TODO: this effectively disables any locale support - which is okay for now
	sprintf(name, "%s%s", /*LOCALE_FILE_DIR*/"", locale);

	//if (pref->OpenText(name) < B_OK)
	//	pref->OpenText(LOCALE_FILE_DEFAULT);
}
