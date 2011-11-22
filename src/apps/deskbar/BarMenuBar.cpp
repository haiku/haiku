/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

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
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/

#include <Debug.h>

#include "BarMenuBar.h"

#include <string.h>

#include <Bitmap.h>
#include <NodeInfo.h>

#include "icons.h"
#include "icons_logo.h"
#include "BarWindow.h"
#include "DeskbarMenu.h"
#include "DeskbarUtils.h"
#include "ResourceSet.h"
#include "TeamMenu.h"


TBarMenuBar::TBarMenuBar(TBarView* bar, BRect frame, const char* name)
	: BMenuBar(frame, name, B_FOLLOW_NONE, B_ITEMS_IN_ROW, false),
	fBarView(bar),
	fAppListMenuItem(NULL)
{
	SetItemMargins(0.0f, 0.0f, 0.0f, 0.0f);

	TDeskbarMenu* beMenu = new TDeskbarMenu(bar);
	TBarWindow::SetDeskbarMenu(beMenu);

	fDeskbarMenuItem = new TBarMenuTitle(frame.Width(), frame.Height(),
		AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_LeafLogoBitmap), beMenu);
	AddItem(fDeskbarMenuItem);
}


TBarMenuBar::~TBarMenuBar()
{
}


void
TBarMenuBar::SmartResize(float width, float height)
{
	if (width == -1.0f && height == -1.0f) {
		BRect frame = Frame();
		width = frame.Width();
		height = frame.Height();
	} else
		ResizeTo(width, height);

	width -= 1;

	int32 count = CountItems();
	if (fDeskbarMenuItem)
		fDeskbarMenuItem->SetWidthHeight(width / count, height);
	if (fAppListMenuItem)
		fAppListMenuItem->SetWidthHeight(width / count, height);

	InvalidateLayout();
}


void
TBarMenuBar::AddTeamMenu()
{
	if (CountItems() > 1)
		return;

	BRect frame(Frame());
	delete fAppListMenuItem;

	fAppListMenuItem = new TBarMenuTitle(0.0f, 0.0f,
		AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_TeamIcon), new TTeamMenu());
	AddItem(fAppListMenuItem);
	SmartResize(frame.Width() - 1.0f, frame.Height());
}


void
TBarMenuBar::RemoveTeamMenu()
{
	if (CountItems() < 2)
		return;

	if (fAppListMenuItem) {
		RemoveItem((BMenuItem*)fAppListMenuItem);
		delete fAppListMenuItem;
		fAppListMenuItem = NULL;
	}

	BRect frame = Frame();
	SmartResize(frame.Width(), frame.Height());
}


void
TBarMenuBar::Draw(BRect rect)
{
	// want to skip the fancy BMenuBar drawing code.
	BMenu::Draw(rect);
}


void
TBarMenuBar::DrawBackground(BRect rect)
{
	BMenu::DrawBackground(rect);
}


void
TBarMenuBar::MouseMoved(BPoint where, uint32 code, const BMessage* message)
{
	// the following code parallels that in ExpandoMenuBar for DnD tracking

	if (!message) {
		// force a cleanup
		fBarView->DragStop(true);
		BMenuBar::MouseMoved(where, code, message);
		return;
	}

	switch (code) {
		case B_ENTERED_VIEW:
		{
			BPoint loc;
			uint32 buttons;
			GetMouse(&loc, &buttons);
			// attempt to start DnD tracking
			if (message && buttons != 0) {
				fBarView->CacheDragData(const_cast<BMessage*>(message));
				MouseDown(loc);
			}
			break;
		}
	}
	BMenuBar::MouseMoved(where, code, message);
}


static void
init_tracking_hook(BMenuItem* item, bool (*hookFunction)(BMenu*, void*),
	void* state)
{
	if (!item)
		return;

	BMenu* windowMenu = item->Submenu();
	if (windowMenu) {
		// have a menu, set the tracking hook
		windowMenu->SetTrackingHook(hookFunction, state);
	}
}


void
TBarMenuBar::InitTrackingHook(bool (*hookFunction)(BMenu*, void*),
	void* state, bool both)
{
	BPoint loc;
	uint32 buttons;
	GetMouse(&loc, &buttons);
	// set the hook functions for the two menus
	// will always have the deskbar menu
	// may have the app menu as well (mini mode)
	if (fDeskbarMenuItem->Frame().Contains(loc) || both)
		init_tracking_hook(fDeskbarMenuItem, hookFunction, state);

	if (fAppListMenuItem && (fAppListMenuItem->Frame().Contains(loc) || both))
		init_tracking_hook(fAppListMenuItem, hookFunction, state);
}

