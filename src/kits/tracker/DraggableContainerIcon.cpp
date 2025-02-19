/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal including
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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include "DraggableContainerIcon.h"

#include <algorithm>

#include <ControlLook.h>
#include <IconUtils.h>
#include <MenuItem.h>
#include <Region.h>

#include "ContainerWindow.h"
#include "IconCache.h"
#include "Model.h"


// amount to move the mouse over for before a drag starts
const float kDragSlop = 3.0f;


DraggableContainerIcon::DraggableContainerIcon(BSize iconSize)
	:
	BView("DraggableContainerIcon", B_WILL_DRAW),
	fIconSize(iconSize),
	fDragButton(0),
	fDragStarted(false)
{
	SetExplicitMinSize(BSize(iconSize.Width() + 5, iconSize.Height()));
	SetExplicitMaxSize(BSize(iconSize.Width() + 5, B_SIZE_UNSET));
}


void
DraggableContainerIcon::MouseDown(BPoint where)
{
	// we only like container windows
	BContainerWindow* window = dynamic_cast<BContainerWindow*>(Window());
	if (window == NULL)
		return;

	// we don't like the Trash icon (because it cannot be moved)
	if (window->TargetModel()->IsTrash() || window->TargetModel()->IsPrintersDir())
		return;

	if (window->CurrentMessage() == NULL)
		return;

	uint32 buttons = Window()->CurrentMessage()->GetInt32("buttons", 0);

	if (IconCache::sIconCache->IconHitTest(where, window->TargetModel(), kNormalIcon, fIconSize)) {
		// The click hit the icon, initiate a drag
		fDragButton = buttons & (B_PRIMARY_MOUSE_BUTTON | B_SECONDARY_MOUSE_BUTTON);
		fDragStarted = false;
		fClickPoint = where;
	} else {
		fDragButton = 0;
	}

	if (!fDragButton)
		Window()->Activate(true);
}


void
DraggableContainerIcon::MouseUp(BPoint)
{
	if (!fDragStarted)
		Window()->Activate(true);

	fDragButton = 0;
	fDragStarted = false;
}


void
DraggableContainerIcon::MouseMoved(BPoint where, uint32, const BMessage*)
{
	if (fDragButton == 0 || fDragStarted
		|| (abs((int32)(where.x - fClickPoint.x)) <= kDragSlop
			&& abs((int32)(where.y - fClickPoint.y)) <= kDragSlop))
		return;

	BContainerWindow* window = static_cast<BContainerWindow*>(Window());
		// we can only get here in a BContainerWindow
	Model* model = window->TargetModel();

	// Find the required height
	BFont font;
	GetFont(&font);

	font_height fontHeight;
	font.GetHeight(&fontHeight);
	float height = ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading + 2 + Bounds().Height() + 8);

	BRect rect(0, 0, std::max(Bounds().Width(),
		font.StringWidth(model->Name()) + 4), height);
	BBitmap* dragBitmap = new BBitmap(rect, B_RGBA32, true);

	dragBitmap->Lock();
	BView* view = new BView(dragBitmap->Bounds(), "", B_FOLLOW_NONE, 0);
	dragBitmap->AddChild(view);
	view->SetOrigin(0, 0);
	BRect clipRect(view->Bounds());
	BRegion newClip;
	newClip.Set(clipRect);
	view->ConstrainClippingRegion(&newClip);

	// Transparent draw magic
	view->SetHighColor(0, 0, 0, 0);
	view->FillRect(view->Bounds());
	view->SetDrawingMode(B_OP_ALPHA);

	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	textColor.alpha = 192;
		// set the level of opacity by value
	view->SetHighColor(textColor);
	view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);

	// Draw the icon
	float hIconOffset = (rect.Width() - Bounds().Width()) / 2;
	IconCache::sIconCache->Draw(model, view, BPoint(hIconOffset, 0), kNormalIcon, fIconSize, true);

	// See if we need to truncate the string
	BString nameString = model->Name();
	if (view->StringWidth(model->Name()) > rect.Width())
		view->TruncateString(&nameString, B_TRUNCATE_MIDDLE, rect.Width() - 5);

	// Draw the label
	float leftText = roundf((view->StringWidth(nameString.String()) - Bounds().Width()) / 2);
	float x = hIconOffset - leftText + 2;
	float y = Bounds().Height() + fontHeight.ascent + 2;
	view->MovePenTo(BPoint(x, y));
	view->DrawString(nameString.String());

	view->Sync();
	dragBitmap->Unlock();

	BMessage message(B_SIMPLE_DATA);
	message.AddRef("refs", model->EntryRef());
	message.AddPoint("click_pt", fClickPoint);

	BPoint tmpLoc;
	uint32 button;
	GetMouse(&tmpLoc, &button);
	if (button)
		message.AddInt32("buttons", (int32)button);

	if ((button & B_PRIMARY_MOUSE_BUTTON) != 0) {
		// add an action specifier to the message, so that it is not copied
		message.AddInt32("be:actions", (modifiers() & B_OPTION_KEY) != 0
			? B_COPY_TARGET : B_MOVE_TARGET);
	}

	fDragStarted = true;
	fDragButton = 0;

	DragMessage(&message, dragBitmap, B_OP_ALPHA,
		BPoint(fClickPoint.x + hIconOffset, fClickPoint.y), this);
}


void
DraggableContainerIcon::Draw(BRect updateRect)
{
	BContainerWindow* window = dynamic_cast<BContainerWindow*>(Window());
	ThrowOnAssert(window != NULL);

	BRect rect(Bounds());
	rgb_color base = ui_color(B_MENU_BACKGROUND_COLOR);
	be_control_look->DrawBorder(this, rect, updateRect, base, B_PLAIN_BORDER,
		0, BControlLook::B_BOTTOM_BORDER);
	be_control_look->DrawMenuBarBackground(this, rect, updateRect, base, 0,
		BControlLook::B_ALL_BORDERS & ~BControlLook::B_LEFT_BORDER);

	// Draw the icon, straddling the border
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	float iconOffsetX = (Bounds().Width() - fIconSize.Width()) / 2;
	float iconOffsetY = (Bounds().Height() - fIconSize.Height()) / 2;
	IconCache::sIconCache->Draw(window->TargetModel(), this,
		BPoint(iconOffsetX, iconOffsetY), kNormalIcon, fIconSize, true);
}

