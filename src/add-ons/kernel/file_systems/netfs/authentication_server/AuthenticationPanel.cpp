// AuthenticationPanel.cpp

#include <stdio.h>

#include <Screen.h>

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Message.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>

#include "AuthenticationPanel.h"

enum {
	MSG_PANEL_OK,
	MSG_PANEL_CANCEL,
};

// constructor
AuthenticationPanel::AuthenticationPanel(BRect frame)
	: Panel(frame, "Name Panel",
			B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
			B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	  fCancelled(false),
	  fExitSem(B_ERROR)
{
	fExitSem = create_sem(0, "Authentication Panel");

	BRect controlFrame(0.0, 0.0, frame.Width(), 15.0);

	fNameTC = new BTextControl(controlFrame, "name", "Username", "", NULL,
							   B_FOLLOW_LEFT | B_FOLLOW_RIGHT);

	fPassTC = new BTextControl(controlFrame, "pass", "Password", "", NULL,
							   B_FOLLOW_LEFT | B_FOLLOW_RIGHT);

	fKeepUsingCB = new BCheckBox(controlFrame, "again",
								 "Use login for all shares of this host",
								 NULL, B_FOLLOW_LEFT | B_FOLLOW_RIGHT);

	BRect buttonFrame(0.0, 0.0, 20.0, 15.0);
	fOkB = new BButton(buttonFrame, "ok", "OK",
					   new BMessage(MSG_PANEL_OK));
	fCancelB = new BButton(buttonFrame, "cancel", "Cancel",
						   new BMessage(MSG_PANEL_CANCEL));

}

// destructor
AuthenticationPanel::~AuthenticationPanel()
{
	delete_sem(fExitSem);
}

// QuitRequested
bool
AuthenticationPanel::QuitRequested()
{
	fCancelled = true;
	release_sem(fExitSem);
	return false;
}

// MessageReceived
void
AuthenticationPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PANEL_CANCEL:
			Cancel();
			break;
		case MSG_PANEL_OK: {
			release_sem(fExitSem);
			break;
		}
		default:
			Panel::MessageReceived(message);
	}
}

// GetAuthentication
bool
AuthenticationPanel::GetAuthentication(const char* server,
									   const char* share,
									   const char* previousUser,
									   const char* previousPass,
									   bool previousKeep,
									   bool badPassword,
									   char* user, char* pass, bool* keep)
{
	// configure panel and layout controls
	BString helper("Enter login for: ");
	helper << (server ? server : "<unkown host>") << "/";
	helper << (share ? share : "<unkown share>");

	// ignore the previous password, if it didn't work
	if (badPassword)
		previousPass = NULL;

	SetTitle(helper.String());

	BPoint offset(0.0, 5.0);

	fNameTC->SetText(previousUser ? previousUser : "");
	fNameTC->ResizeToPreferred();
	fNameTC->MoveTo(BPoint(10.0, 10.0));

	fPassTC->SetText(previousPass ? previousPass : "");
	fPassTC->ResizeToPreferred();
	fPassTC->MoveTo(fNameTC->Frame().LeftBottom() + offset);

	fKeepUsingCB->SetValue(previousKeep);
	fKeepUsingCB->ResizeToPreferred();
	fKeepUsingCB->MoveTo(fPassTC->Frame().LeftBottom() + offset);

	fCancelB->ResizeToPreferred();

	fOkB->ResizeToPreferred();
	fOkB->MoveTo(fKeepUsingCB->Frame().RightBottom() + offset + offset - fOkB->Frame().RightTop());

	fCancelB->MoveTo(fOkB->Frame().LeftTop() - BPoint(10.0, 0.0) - fCancelB->Frame().RightTop());

	BRect frame(fNameTC->Frame().LeftTop(), fOkB->Frame().RightBottom());

	// work arround buggy BTextControl resizing
	BRect nameFrame = fNameTC->Frame();
	BRect passFrame = fPassTC->Frame();

	nameFrame.right = nameFrame.left + frame.Width();
	passFrame.right = passFrame.left + frame.Width();

	float divider = fNameTC->Divider();

	if (fPassTC->Divider() > divider)
		divider = fPassTC->Divider();

	delete fNameTC;
	fNameTC = new BTextControl(nameFrame, "name", "Username", "", NULL,
							   B_FOLLOW_LEFT | B_FOLLOW_RIGHT);
	fNameTC->SetText(previousUser ? previousUser : "");

	delete fPassTC;
	fPassTC = new BTextControl(passFrame, "pass", "Password", "", NULL,
							   B_FOLLOW_LEFT | B_FOLLOW_RIGHT);

	fPassTC->TextView()->HideTyping(true);
	fPassTC->SetText(previousPass ? previousPass : "");

	fNameTC->SetDivider(divider);
	fPassTC->SetDivider(divider);


	// create background view
	frame.InsetBy(-10.0, -10.0);

	BBox* bg = new BBox(frame, "bg", B_FOLLOW_ALL,
						B_FRAME_EVENTS | B_WILL_DRAW | B_NAVIGABLE_JUMP,
						B_PLAIN_BORDER);

	bg->AddChild(fNameTC);
	bg->AddChild(fPassTC);
	bg->AddChild(fKeepUsingCB);

	bg->AddChild(fOkB);
	bg->AddChild(fCancelB);

	frame.OffsetTo(-10000.0, -10000.0);
	frame = _CalculateFrame(frame);
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());

	AddChild(bg);
	SetDefaultButton(fOkB);
	fNameTC->MakeFocus(true);

	// start window thread
	Show();

	// let the window jitter, if the previous password was invalid
	if (badPassword) {
		BPoint leftTop = Frame().LeftTop();
		const float jitterOffsets[] = { -10, 0, 10, 0 };
		const int32 jitterOffsetCount = sizeof(jitterOffsets) / sizeof(float);
		for (int32 i = 0; i < 30; i++) {
			float offset = jitterOffsets[i % jitterOffsetCount];
			MoveTo(leftTop.x + offset, leftTop.y);
			snooze(10000);
		}
		MoveTo(leftTop);
	}

	// block calling thread
	acquire_sem(fExitSem);

	// window wants to quit
	Lock();

	strcpy(user, fNameTC->Text());
	strcpy(pass, fPassTC->Text());
	*keep = fKeepUsingCB->Value() == B_CONTROL_ON;

	Quit();
	return fCancelled;
}

// Cancel
void
AuthenticationPanel::Cancel()
{
	fCancelled = true;
//	release_sem(fExitSem);

	Panel::Cancel();
}


// _CalculateFrame
BRect
AuthenticationPanel::_CalculateFrame(BRect frame)
{
	BScreen screen(B_MAIN_SCREEN_ID);
	BRect screenFrame = screen.Frame();
	if (!screenFrame.Contains(frame)) {
		float width = frame.Width();
		float height = frame.Height();
		BPoint center;
		center.x = screenFrame.left + screenFrame.Width() / 2.0;
		center.y = screenFrame.top + screenFrame.Height() / 4.0;
		frame.left = center.x - width / 2.0;
		frame.right = frame.left + width;
		frame.top = center.y - height / 2.0;
		frame.bottom = frame.top + height;
	}
	return frame;
}
