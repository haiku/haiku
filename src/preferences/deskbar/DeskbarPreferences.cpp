/*
 * Copyright 2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */


#include "PreferencesWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <Roster.h>


int
main(int argc, char **argv)
{
	B_TRANSLATE_MARK_SYSTEM_NAME_VOID("Deskbar");
	BApplication app("application/x-vnd.Haiku-DeskbarPreferences");
	be_roster->Launch("application/x-vnd.Be-TSKB", new BMessage(kConfigShow));
	return 0;
}

