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


#include "BarMenuTitle.h"

#include <Bitmap.h>
#include <ControlLook.h>
#include <Debug.h>
#include <Region.h>

#include "BarApp.h"
#include "BarView.h"
#include "BarWindow.h"
#include "DeskbarMenu.h"


TBarMenuTitle::TBarMenuTitle(float width, float height, const BBitmap* icon,
	BMenu* menu, TBarView* barView)
	:
	BMenuItem(menu, new BMessage(B_REFS_RECEIVED)),
	fWidth(width),
	fHeight(height),
	fIcon(icon),
	fMenu(menu),
	fBarView(barView),
	fInitStatus(B_NO_INIT)
{
	if (fIcon == NULL || fMenu == NULL || fBarView == NULL)
		fInitStatus = B_BAD_VALUE;
	else
		fInitStatus = B_OK;
}


TBarMenuTitle::~TBarMenuTitle()
{
}


void
TBarMenuTitle::SetContentSize(float width, float height)
{
	fWidth = width;
	fHeight = height;
}


void
TBarMenuTitle::GetContentSize(float* width, float* height)
{
	*width = fWidth;
	*height = fHeight;
}


void
TBarMenuTitle::Draw()
{
	BMenu* menu = Menu();
	if (fInitStatus != B_OK || menu == NULL)
		return;

	BRect frame(Frame());
	rgb_color base = ui_color(B_MENU_BACKGROUND_COLOR);

	menu->PushState();

	BRect windowBounds = menu->Window()->Bounds();
	if (frame.right > windowBounds.right)
		frame.right = windowBounds.right;

	// fill in background
	if (IsSelected()) {
		be_control_look->DrawMenuItemBackground(menu, frame, frame, base,
			BControlLook::B_ACTIVATED);
	} else
		be_control_look->DrawButtonBackground(menu, frame, frame, base);

	menu->MovePenTo(ContentLocation());
	DrawContent();

	menu->PopState();
}


void
TBarMenuTitle::DrawContent()
{
	if (fInitStatus != B_OK)
		return;

	BMenu* menu = Menu();
	if (menu == NULL)
		return;

	menu->SetDrawingMode(B_OP_ALPHA);

	const BRect frame(Frame());
	BRect iconRect(fIcon->Bounds().OffsetToCopy(frame.LeftTop()));

	float widthOffset = rintf((frame.Width() - iconRect.Width()) / 2);
	float heightOffset = rintf((frame.Height() - iconRect.Height()) / 2);

	// cut-off the leaf
	bool isLeafMenu = dynamic_cast<TDeskbarMenu*>(fMenu) != NULL;
	if (isLeafMenu)
		iconRect.OffsetBy(widthOffset, frame.Height() - iconRect.Height() + 2);
	else
		iconRect.OffsetBy(widthOffset, heightOffset);

	// clip to menu item frame
	if (iconRect.Width() > frame.Width()) {
		float diff = iconRect.Width() - frame.Width();
		BRect mask(iconRect.InsetByCopy(floorf(diff / 2), 0));
		BRegion clipping(mask);
		menu->ConstrainClippingRegion(&clipping);
	}

	menu->DrawBitmapAsync(fIcon, iconRect);
	menu->ConstrainClippingRegion(NULL);
}


status_t
TBarMenuTitle::Invoke(BMessage* message)
{
	if (fInitStatus != B_OK || fBarView == NULL)
		return fInitStatus;

	BLooper* looper = fBarView->Looper();
	if (looper->Lock()) {
		// tell barview to add the refs to the deskbar menu
		fBarView->HandleDeskbarMenu(NULL);
		looper->Unlock();
	}

	return BMenuItem::Invoke(message);
}
