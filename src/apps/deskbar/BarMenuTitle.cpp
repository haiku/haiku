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
#include "ExpandoMenuBar.h"


TBarMenuTitle::TBarMenuTitle(float width, float height, const BBitmap* icon,
	BMenu* menu, bool inexpando)
	:	BMenuItem(menu, new BMessage(B_REFS_RECEIVED)),
		fWidth(width),
		fHeight(height),
		fInExpando(inexpando),
		fIcon(icon)
{
}


TBarMenuTitle::~TBarMenuTitle()
{
}


void
TBarMenuTitle::SetWidthHeight(float width, float height)
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
	if (be_control_look == NULL) {
		BMenuItem::Draw();
		return;
	}

	// fill background if selected
	rgb_color base = Menu()->LowColor();
	BRect rect = Frame();

	BRect windowBounds = Menu()->Window()->Bounds();
	if (rect.right > windowBounds.right)
		rect.right = windowBounds.right;

	if (IsSelected()) {
		be_control_look->DrawMenuItemBackground(Menu(), rect, rect, base,
			BControlLook::B_ACTIVATED);
	} else {
		be_control_look->DrawButtonBackground(Menu(), rect, rect, base);
	}

	// draw content
	DrawContent();

	// make sure we restore state
	Menu()->SetLowColor(base);
}


void
TBarMenuTitle::DrawContent()
{
	BMenu* menu = Menu();
	BRect frame(Frame());

	if (be_control_look != NULL) {
		menu->SetDrawingMode(B_OP_ALPHA);

		if (fIcon != NULL) {
			BRect dstRect(fIcon->Bounds());
			dstRect.OffsetTo(frame.LeftTop());
			dstRect.OffsetBy(rintf(((frame.Width() - dstRect.Width()) / 2)
				- 1.0f), rintf(((frame.Height() - dstRect.Height()) / 2)
				+ 2.0f));

			menu->DrawBitmapAsync(fIcon, dstRect);
		}
		return;
	}

	rgb_color menuColor = menu->LowColor();
	rgb_color dark = tint_color(menuColor, B_DARKEN_1_TINT);
	rgb_color light = tint_color(menuColor, B_LIGHTEN_2_TINT);
	rgb_color black = {0, 0, 0, 255};

	bool inExpandoMode = dynamic_cast<TExpandoMenuBar*>(menu) != NULL;

	BRect bounds(menu->Window()->Bounds());
	if (bounds.right < frame.right)
		frame.right = bounds.right;

	menu->SetDrawingMode(B_OP_COPY);

	if (!IsSelected() && !menu->IsRedrawAfterSticky()) {
		menu->BeginLineArray(8);
		menu->AddLine(frame.RightTop(), frame.LeftTop(), light);
		menu->AddLine(frame.LeftBottom(), frame.RightBottom(), dark);
		menu->AddLine(frame.LeftTop(),
			frame.LeftBottom()+BPoint(0, inExpandoMode ? 0 : -1), light);
		menu->AddLine(frame.RightBottom(), frame.RightTop(), dark);
		if (inExpandoMode) {
			frame.top += 1;
			menu->AddLine(frame.LeftTop(), frame.RightTop() + BPoint(-1, 0),
				light);
		}

		menu->EndLineArray();

		frame.InsetBy(1, 1);
		menu->SetHighColor(menuColor);
		menu->FillRect(frame);
		menu->SetHighColor(black);
		frame.InsetBy(-1, -1);
		if (inExpandoMode)
			frame.top -= 1;
	}

	ASSERT(IsEnabled());
	if (IsSelected() && !menu->IsRedrawAfterSticky()) {
		menu->SetHighColor(tint_color(menuColor, B_HIGHLIGHT_BACKGROUND_TINT));
		menu->FillRect(frame);

		if (menu->IndexOf(this) > 0) {
			menu->SetHighColor(tint_color(menuColor, B_DARKEN_4_TINT));
			menu->StrokeLine(frame.LeftTop(), frame.LeftBottom());
		}

		menu->SetHighColor(black);
	}

	menu->SetDrawingMode(B_OP_ALPHA);

	if (fIcon != NULL) {
		BRect dstRect(fIcon->Bounds());
		dstRect.OffsetTo(frame.LeftTop());
		dstRect.OffsetBy(rintf(((frame.Width() - dstRect.Width()) / 2) - 1.0f),
			rintf(((frame.Height() - dstRect.Height()) / 2) - 0.0f));

		menu->DrawBitmapAsync(fIcon, dstRect);
	}
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
