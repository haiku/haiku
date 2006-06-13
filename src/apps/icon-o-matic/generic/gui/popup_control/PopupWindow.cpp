/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include <AppDefs.h>
#include <Message.h> 
#include <MessageFilter.h>
#include <View.h>

#include "PopupControl.h"
#include "PopupView.h"

#include "PopupWindow.h"

// MouseDownFilter

class MouseDownFilter : public BMessageFilter {
 public:
								MouseDownFilter(BWindow* window);

	virtual	filter_result		Filter(BMessage*, BHandler** target);

 private:
			BWindow*			fWindow;
};

// constructor
MouseDownFilter::MouseDownFilter(BWindow* window)
	: BMessageFilter(B_MOUSE_DOWN),
	  fWindow(window)
{
}

// Filter
filter_result
MouseDownFilter::Filter(BMessage* message, BHandler** target)
{
	if (fWindow) {
		if (BView* view = dynamic_cast<BView*>(*target)) {
			BPoint point;
			if (message->FindPoint("where", &point) == B_OK) {
				if (!fWindow->Frame().Contains(view->ConvertToScreen(point)))
					*target = fWindow;
			}
		}
	}
	return B_DISPATCH_MESSAGE;
}


// PopupWindow

// constructor
PopupWindow::PopupWindow(PopupView* child, PopupControl* control)
	: MWindow(BRect(0.0, 0.0, 10.0, 10.0), "popup",
			  B_NO_BORDER_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
			  B_ASYNCHRONOUS_CONTROLS),
	  fCanceled(true),
	  fControl(control)
{
	AddChild(child);
	child->SetPopupWindow(this);
	AddCommonFilter(new MouseDownFilter(this));
}

// destructor
PopupWindow::~PopupWindow()
{
}

// MessageReceived
void
PopupWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_MOUSE_DOWN:
			fCanceled = true;
			Hide();
			break;
		default:
			MWindow::MessageReceived(message);
			break;
	}
}

// Show
void
PopupWindow::Show()
{
	if (BLooper *looper = fControl->Looper())
		looper->PostMessage(MSG_POPUP_SHOWN, fControl);
	MWindow::Show();
}

// Hide
void
PopupWindow::Hide()
{
	if (BLooper *looper = fControl->Looper()) {
		BMessage msg(MSG_POPUP_HIDDEN);
		msg.AddBool("canceled", fCanceled);
		looper->PostMessage(&msg, fControl);
	}
	MWindow::Hide();
}

// QuitRequested
bool
PopupWindow::QuitRequested()
{
	return false;
}

// PopupDone
void
PopupWindow::PopupDone(bool canceled)
{
	fCanceled = canceled;
	Hide();
}

