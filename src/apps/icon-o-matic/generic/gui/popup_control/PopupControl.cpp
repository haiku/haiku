/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include "PopupControl.h"

#include <stdio.h>

#include <Message.h>
#include <Screen.h>

#include "PopupView.h"
#include "PopupWindow.h"

// constructor
PopupControl::PopupControl(const char* name, PopupView* child)
	: BView(BRect(0.0f, 0.0f, 10.0f, 10.0f),
			name, B_FOLLOW_NONE, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	  fPopupWindow(NULL),
	  fPopupChild(child),
	  fHPopupAlignment(0.5),
	  fVPopupAlignment(0.5)
{
}

// destructor
PopupControl::~PopupControl()
{
}

// MessageReceived
void
PopupControl::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_POPUP_SHOWN:
			PopupShown();
			break;
		case MSG_POPUP_HIDDEN:
			bool canceled;
			if (message->FindBool("canceled", &canceled) != B_OK)
				canceled = true;
			PopupHidden(canceled);
			HidePopup();
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}

// SetPopupLocation
void
PopupControl::SetPopupAlignment(float hPopupAlignment, float vPopupAlignment)
{
	fHPopupAlignment = hPopupAlignment;
	fVPopupAlignment = vPopupAlignment;
}

// SetPopupLocation
//
// overrides Alignment
void
PopupControl::SetPopupLocation(BPoint leftTop)
{
	fPopupLeftTop = leftTop;
	fHPopupAlignment = -1.0;
	fVPopupAlignment = -1.0;
}

// ShowPopup
void
PopupControl::ShowPopup(BPoint* offset)
{
	if (!fPopupWindow) {
		fPopupWindow = new PopupWindow(fPopupChild, this);
		fPopupWindow->RecalcSize();
		BRect frame(fPopupWindow->Frame());

		BPoint leftLocation;
		if (fHPopupAlignment >= 0.0 && fVPopupAlignment >= 0.0) {
			leftLocation = ConvertToScreen(Bounds().LeftTop());
			leftLocation.x -= fPopupWindow->Frame().Width() + 1.0;
			leftLocation.y -= fPopupWindow->Frame().Height() + 1.0;
			float totalWidth = Bounds().Width() + fPopupWindow->Frame().Width();
			float totalHeight = Bounds().Height() + fPopupWindow->Frame().Height();
			leftLocation.x += fHPopupAlignment * totalWidth;
			leftLocation.y += fHPopupAlignment * totalHeight;
		} else
			leftLocation = ConvertToScreen(fPopupLeftTop);

		frame.OffsetTo(leftLocation);
		BScreen screen(fPopupWindow);
		BRect dest(screen.Frame());
		// check if too big
		if (frame.Width() > dest.Width())
			frame.right = frame.left + dest.Width();
		if (frame.Height() > dest.Height())
			frame.bottom = frame.top + dest.Height();
		// check if out of screen
		float hOffset = 0.0;
		float vOffset = 0.0;
		if (frame.bottom > dest.bottom)
			vOffset = dest.bottom - frame.bottom;
		if (frame.top < dest.top)
			vOffset = dest.top - frame.top;
		if (frame.right > dest.right)
			hOffset = dest.right - frame.right;
		if (frame.left < dest.left)
			hOffset = dest.left - frame.left;
		// finally move/resize our popup window
		frame.OffsetBy(hOffset, vOffset);
		if (offset) {
			offset->x += hOffset;
			offset->y += vOffset;
		} 
		fPopupWindow->MoveTo(frame.LeftTop());
		fPopupWindow->ResizeTo(frame.Width(), frame.Height());
		fPopupWindow->Show();
	}
}

// HidePopup
void
PopupControl::HidePopup()
{
	if (fPopupWindow) {
		fPopupWindow->Lock();
		fPopupChild->SetPopupWindow(NULL);
		fPopupWindow->RemoveChild(fPopupChild);
		fPopupWindow->Quit();
		fPopupWindow = NULL;
	}
}

// PopupShown
void
PopupControl::PopupShown()
{
}

// PopupHidden
void
PopupControl::PopupHidden(bool canceled)
{
}


