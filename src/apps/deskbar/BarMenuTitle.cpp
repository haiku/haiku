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

#include "BarApp.h"
#include "BarView.h"
#include "BarWindow.h"


TBarMenuTitle::TBarMenuTitle(float width, float height, const BBitmap* icon,
	BMenu* menu, bool expando)
	:
	BMenuItem(menu, new BMessage(B_REFS_RECEIVED)),
	fWidth(width),
	fHeight(height),
	fInExpando(expando),
	fIcon(icon)
{
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
	if (menu == NULL)
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
	if (fIcon == NULL)
		return;

	BMenu* menu = Menu();
	BRect frame(Frame());
	BRect iconRect(fIcon->Bounds());

	menu->SetDrawingMode(B_OP_ALPHA);
	iconRect.OffsetTo(frame.LeftTop());

	float widthOffset = rintf((frame.Width() - iconRect.Width()) / 2);
	float heightOffset = 0;
	if (frame.Height() > iconRect.Height() + 2)
		heightOffset = rintf((frame.Height() - iconRect.Height()) / 2);
	iconRect.OffsetBy(widthOffset - 1.0f, heightOffset + 2.0f);

	menu->DrawBitmapAsync(fIcon, iconRect);
}


status_t
TBarMenuTitle::Invoke(BMessage* message)
{
	TBarView* barview = dynamic_cast<TBarApp*>(be_app)->BarView();
	if (barview) {
		BLooper* looper = barview->Looper();
		if (looper->Lock()) {
			// tell barview to add the refs to the deskbar menu
			barview->HandleDeskbarMenu(NULL);
			looper->Unlock();
		}
	}

	return BMenuItem::Invoke(message);
}
