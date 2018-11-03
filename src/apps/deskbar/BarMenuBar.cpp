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


#include "BarMenuBar.h"

#include <algorithm>

#include <Bitmap.h>
#include <ControlLook.h>
#include <Debug.h>
#include <IconUtils.h>
#include <NodeInfo.h>

#include "icons.h"

#include "BarMenuTitle.h"
#include "BarView.h"
#include "BarWindow.h"
#include "DeskbarMenu.h"
#include "DeskbarUtils.h"
#include "ResourceSet.h"
#include "TeamMenu.h"


const float kSepItemWidth = 5.0f;


//	#pragma mark - TSeparatorItem


TSeparatorItem::TSeparatorItem()
	:
	BSeparatorItem()
{
}


void
TSeparatorItem::Draw()
{
	BMenu* menu = Menu();
	if (menu == NULL)
		return;

	BRect frame(Frame());
	frame.right = frame.left + kSepItemWidth;
	rgb_color base = ui_color(B_MENU_BACKGROUND_COLOR);

	menu->PushState();

	menu->SetHighColor(tint_color(base, 1.22));
	frame.top--;
		// need to expand the frame for some reason

	// stroke a darker line on the left edge
	menu->StrokeLine(frame.LeftTop(), frame.LeftBottom());
	frame.left++;

	// fill in background
	be_control_look->DrawButtonBackground(menu, frame, frame, base);

	menu->PopState();
}


//	#pragma mark - TBarMenuBar


TBarMenuBar::TBarMenuBar(BRect frame, const char* name, TBarView* barView)
	:
	BMenuBar(frame, name, B_FOLLOW_NONE, B_ITEMS_IN_ROW, false),
	fBarView(barView),
	fAppListMenuItem(NULL),
	fSeparatorItem(NULL)
{
	SetItemMargins(0.0f, 0.0f, 0.0f, 0.0f);

	TDeskbarMenu* beMenu = new TDeskbarMenu(barView);
	TBarWindow::SetDeskbarMenu(beMenu);

	BBitmap* icon = NULL;
	size_t dataSize;
	const void* data = AppResSet()->FindResource(B_VECTOR_ICON_TYPE,
		R_LeafLogoBitmap, &dataSize);
	if (data != NULL) {
		// Scale bitmap according to font size
		float width = std::max(63.f, ceilf(63 * be_plain_font->Size() / 12.f));
		float height = std::max(22.f, ceilf(22 * be_plain_font->Size() / 12.f));
		icon = new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGBA32);
		if (icon->InitCheck() != B_OK
			|| BIconUtils::GetVectorIcon((const uint8*)data, dataSize, icon)
					!= B_OK) {
			delete icon;
			icon = NULL;
		}
	}

	fDeskbarMenuItem = new TBarMenuTitle(0.0f, 0.0f, icon, beMenu);
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

	if (fSeparatorItem != NULL)
		fDeskbarMenuItem->SetContentSize(width - kSepItemWidth, height);
	else {
		int32 count = CountItems();
		if (fDeskbarMenuItem)
			fDeskbarMenuItem->SetContentSize(width / count, height);
		if (fAppListMenuItem)
			fAppListMenuItem->SetContentSize(width / count, height);
	}

	InvalidateLayout();
}


bool
TBarMenuBar::AddTeamMenu()
{
	if (CountItems() > 1)
		return false;

	BRect frame(Frame());

	delete fAppListMenuItem;
	fAppListMenuItem = new TBarMenuTitle(0.0f, 0.0f,
		AppResSet()->FindBitmap(B_MESSAGE_TYPE, R_TeamIcon), new TTeamMenu());

	bool added = AddItem(fAppListMenuItem);

	if (added)
		SmartResize(frame.Width() - 1.0f, frame.Height());
	else
		SmartResize(frame.Width(), frame.Height());

	return added;
}


bool
TBarMenuBar::RemoveTeamMenu()
{
	if (CountItems() < 2)
		return false;

	bool removed = false;

	if (fAppListMenuItem != NULL && RemoveItem(fAppListMenuItem)) {
		delete fAppListMenuItem;
		fAppListMenuItem = NULL;
		SmartResize(-1, -1);
		removed = true;
	}

	return removed;
}


bool
TBarMenuBar::AddSeparatorItem()
{
	if (CountItems() > 1)
		return false;

	BRect frame(Frame());

	delete fSeparatorItem;
	fSeparatorItem = new TSeparatorItem();

	bool added = AddItem(fSeparatorItem);

	if (added)
		SmartResize(frame.Width() - 1.0f, frame.Height());
	else
		SmartResize(frame.Width(), frame.Height());

	return added;
}


bool
TBarMenuBar::RemoveSeperatorItem()
{
	if (CountItems() < 2)
		return false;

	bool removed = false;

	if (fSeparatorItem != NULL && RemoveItem(fSeparatorItem)) {
		delete fSeparatorItem;
		fSeparatorItem = NULL;
		SmartResize(-1, -1);
		removed = true;
	}

	return removed;
}


void
TBarMenuBar::Draw(BRect updateRect)
{
	// skip the fancy BMenuBar drawing code
	BMenu::Draw(updateRect);
}


void
TBarMenuBar::DrawBackground(BRect updateRect)
{
	BMenu::DrawBackground(updateRect);
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
			if (message != NULL && buttons != 0) {
				// attempt to start DnD tracking
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
