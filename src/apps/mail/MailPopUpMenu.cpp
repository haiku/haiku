/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or
registered trademarks of Be Incorporated in the United States and other
countries. Other brand product names are registered trademarks or trademarks
of their respective holders. All rights reserved.
*/


#include "MailPopUpMenu.h"

#include <Catalog.h>
#include <Locale.h>
#include <MenuItem.h>
#include <Message.h>

#include <malloc.h>
#include <string.h>

#include "MailSupport.h"
#include "Messages.h"


#define B_TRANSLATION_CONTEXT "Mail"


TMenu::TMenu(const char *name, const char *attribute, int32 message, bool popup,
		bool addRandom)
	: BPopUpMenu(name, false, false),
	fPopup(popup),
	fAddRandom(addRandom),
	fMessage(message)
{
	fAttribute = strdup(attribute);
	BuildMenu();
}


TMenu::~TMenu()
{
	free(fAttribute);
}


void
TMenu::AttachedToWindow()
{
	BuildMenu();
	BPopUpMenu::AttachedToWindow();
}


BPoint
TMenu::ScreenLocation(void)
{
	if (fPopup)
		return BPopUpMenu::ScreenLocation();

	return BMenu::ScreenLocation();
}


void
TMenu::BuildMenu()
{
	RemoveItems(0, CountItems(), true);
	add_query_menu_items(this, fAttribute, fMessage, NULL, fPopup);

	if (fAddRandom && CountItems() > 0) {
		AddItem(new BSeparatorItem(), 0);

		BMessage *msg = new BMessage(M_RANDOM_SIG);
		if (!fPopup) {
			AddItem(new BMenuItem(B_TRANSLATE("Random"), msg, '0'), 0);
		} else {
			AddItem(new BMenuItem(B_TRANSLATE("Random"), msg), 0);
		}
	}
}

