/* BMailStatusWindow - the status window while fetching/sending mails
**
** Copyright (c) 2001-2003 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <StringView.h>
#include <Screen.h>
#include <String.h>
#include <StatusBar.h>
#include <Roster.h>
#include <Locker.h>
#include <E-mail.h>

#include <stdio.h>
#include <assert.h>

class _EXPORT BMailStatusWindow;
class _EXPORT BMailStatusView;

#include "status.h"
#include "MailSettings.h"

#include <MDRLanguage.h>

/*------------------------------------------------

BMailStatusWindow

------------------------------------------------*/

static BLocker sLock;


BMailStatusWindow::BMailStatusWindow(BRect rect, const char *name, uint32 s)
	: BWindow(rect, name, B_MODAL_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
          B_NOT_CLOSABLE | B_NO_WORKSPACE_ACTIVATION | B_NOT_V_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE),
	fShowMode(s),
	fWindowMoved(0L)
{
	BRect frame(Bounds());
	frame.InsetBy(90.0 + 5.0, 5.0);

	BButton *button = new BButton(frame, "check_mail",
							MDR_DIALECT_CHOICE ("Check Mail Now","メールチェック"),
							new BMessage('mbth'), B_FOLLOW_LEFT_RIGHT,
							B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE);
	button->ResizeToPreferred();
	frame = button->Frame();

	button->ResizeTo(button->Bounds().Width(),25);
	button->SetTarget(be_app_messenger);

	frame.OffsetBy(0.0, frame.Height());
	frame.InsetBy(-90.0, 0.0);

	fMessageView = new BStringView(frame, "message_view", "",
								   B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	fMessageView->SetAlignment(B_ALIGN_CENTER);
	fMessageView->SetText(MDR_DIALECT_CHOICE ("No new messages.","未読メッセージはありません"));
	float framewidth = frame.Width();
	fMessageView->ResizeToPreferred();
	fMessageView->ResizeTo(framewidth,fMessageView->Bounds().Height());
	frame = fMessageView->Frame();

	frame.InsetBy(-5.0, -5.0);
	frame.top = 0.0;

	fDefaultView = new BBox(frame, "default_view", B_FOLLOW_LEFT_RIGHT,
							B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
	fDefaultView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fDefaultView->AddChild(button);
	fDefaultView->AddChild(fMessageView);

	fMinWidth = fDefaultView->Bounds().Width();
	fMinHeight = fDefaultView->Bounds().Height();
	ResizeTo(fMinWidth, fMinHeight);
	SetSizeLimits(fMinWidth, 2.0 * fMinWidth, fMinHeight, fMinHeight);

	BMailSettings general;
	if (general.InitCheck() == B_OK) {
		// set on-screen location

		frame = general.StatusWindowFrame();
		BScreen screen(this);
		if (screen.Frame().Contains(frame)) {
			MoveTo(frame.LeftTop());
			if (frame.Width() >= fMinWidth && frame.Height() >= fMinHeight) {
				float x_off_set = frame.Width() - fMinWidth;
				float y_off_set = 0; //---The height is constant

				ResizeBy(x_off_set, y_off_set);
				fDefaultView->ResizeBy(x_off_set, y_off_set);
				button->ResizeBy(x_off_set, y_off_set);
				fMessageView->ResizeBy(x_off_set, y_off_set);
			}
		}
		// set workspace for window

		uint32 workspace = general.StatusWindowWorkspaces();
		int32 workspacesCount = count_workspaces();
		uint32 workspacesMask = (workspacesCount > 31 ? 0 : 1L << workspacesCount) - 1;
		if ((workspacesMask & workspace) && (workspace != Workspaces()))
			SetWorkspaces(workspace);

		// set look

		SetBorderStyle(general.StatusWindowLook());
	}
	AddChild(fDefaultView);

	fFrame = Frame();

	if (fShowMode != B_MAIL_SHOW_STATUS_WINDOW_ALWAYS)
		Hide();
	Show();
}


BMailStatusWindow::~BMailStatusWindow()
{
	// remove all status_views, so we don't accidentally delete them
	while (BMailStatusView *status_view = (BMailStatusView *)fStatusViews.RemoveItem(0L))
		RemoveView(status_view);

	BMailSettings general;
	if (general.InitCheck() == B_OK) {
		// save the current status window properties
		general.SetStatusWindowFrame(Frame());
		general.SetStatusWindowWorkspaces((int32)Workspaces());
		general.Save();
	}
}


void
BMailStatusWindow::FrameMoved(BPoint /*origin*/)
{
	if (fLastWorkspace == current_workspace())
		fFrame = Frame();
}


void
BMailStatusWindow::WorkspaceActivated(int32 workspace, bool active)
{
	if (!active)
		return;

	MoveTo(fFrame.LeftTop());
	fLastWorkspace = workspace;

	// make the window visible if the screen's frame doesn't contain it
	BScreen screen;
	if (screen.Frame().bottom < fFrame.top)
		MoveTo(fFrame.left - 1, screen.Frame().bottom - fFrame.Height() - 4);
	if (screen.Frame().right < fFrame.left)
		MoveTo(fFrame.left - 1, screen.Frame().bottom - fFrame.Height() - 4);
}


void
BMailStatusWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case 'lkch':
		{
			int32 look;
			if (msg->FindInt32("StatusWindowLook", &look) == B_OK)
				SetBorderStyle(look);
			break;
		}
		case 'wsch':
		{
			uint32 workspaces;
			if (msg->FindInt32("StatusWindowWorkSpace", (int32 *)&workspaces) != B_OK)
				break;
			if (Workspaces() != B_ALL_WORKSPACES && workspaces != B_ALL_WORKSPACES)
				break;
			if (workspaces != Workspaces())
				SetWorkspaces(workspaces);
			break;
		}
		case 'DATA':
			msg->what = B_REFS_RECEIVED;
			be_roster->Launch(B_MAIL_TYPE, msg);
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}


void
BMailStatusWindow::SetDefaultMessage(const BString &message)
{
	if (Lock()) {
		fMessageView->SetText(message.String());
		Unlock();
	}
}


BMailStatusView *
BMailStatusWindow::NewStatusView(const char *description, bool upstream)
{
	if (!Lock())
		return NULL;

	BRect rect = Bounds();
	rect.top = fStatusViews.CountItems() * (fMinHeight + 1);
	rect.bottom = rect.top + fMinHeight;
	BMailStatusView *status = new BMailStatusView(rect, description, upstream);
	status->window = this;

	Unlock();
	return status;
}


void
BMailStatusWindow::ActuallyAddStatusView(BMailStatusView *status)
{
	if (!Lock())
		return;

	sLock.Lock();

	BRect rect = Bounds();
	rect.top = fStatusViews.CountItems() * (fMinHeight + 1);
	rect.bottom = rect.top + fMinHeight;

	status->MoveTo(rect.LeftTop());
	status->ResizeTo(rect.Width(), rect.Height());

	fStatusViews.AddItem((void *)status);

	status->Hide();
	AddChild(status);

	if (CountVisibleItems() == 1)
		fDefaultView->Hide();

	status->Show();
	SetSizeLimits(10.0, 2000.0, 10.0, 2000.0);

	// if the window doesn't fit on screen anymore, move it
	BScreen screen;
	if (screen.Frame().bottom < Frame().top + rect.bottom) {
		MoveBy(0, -fMinHeight - 1);
		fWindowMoved++;
	}

	ResizeTo(rect.Width(), rect.bottom);

	if (fShowMode != B_MAIL_SHOW_STATUS_WINDOW_ALWAYS
		&& fShowMode != B_MAIL_SHOW_STATUS_WINDOW_NEVER
		&& CountVisibleItems() == 1)
	{
		SetFlags(Flags() | B_AVOID_FOCUS);
		Show();
		SetFlags(Flags() ^ B_AVOID_FOCUS);
	}
	sLock.Unlock();
	Unlock();
}


void
BMailStatusWindow::RemoveView(BMailStatusView *view)
{
	if (!view || !Lock())
		return;

	sLock.Lock();
		// ToDo: although there already is the outer lock, this seems
		//	to help... (maybe we should investigate this further...)

	int32 i = fStatusViews.IndexOf(view);
	if (i < 0) {
		Unlock();
		return;
	}

	fStatusViews.RemoveItem((void *)view);
	if (RemoveChild(view)) {
		while ((view = (BMailStatusView *)fStatusViews.ItemAt(i++)) != NULL)
			view->MoveBy(0, -fMinHeight - 1);

		// the view will be deleted in the ChainRunner
		view = NULL;
	}

	if (fWindowMoved > 0) {
		fWindowMoved--;
		MoveBy(0, fMinHeight + 1);
	}

	if (CountVisibleItems() == 0) {
		if (fShowMode != B_MAIL_SHOW_STATUS_WINDOW_NEVER
			&& fShowMode != B_MAIL_SHOW_STATUS_WINDOW_ALWAYS) {
			while (!IsHidden())
				Hide();
		}

		fDefaultView->Show();

		SetSizeLimits(fMinWidth, 2.0 * fMinWidth, fMinHeight, fMinHeight);
		ResizeTo(fDefaultView->Frame().Width(), fDefaultView->Frame().Height());

		be_app->PostMessage('stwg');
			// notify that the status window is gone
	}
	else
		ResizeTo(Bounds().Width(), fStatusViews.CountItems() * fMinHeight);

	sLock.Unlock();
	Unlock();
}


int32
BMailStatusWindow::CountVisibleItems()
{
	if (fShowMode != B_MAIL_SHOW_STATUS_WINDOW_WHEN_SENDING)
		return fStatusViews.CountItems();

	int32 count = 0;
	for (int32 i = fStatusViews.CountItems(); i-- > 0;) {
		BMailStatusView *view = (BMailStatusView *)fStatusViews.ItemAt(i);
		if (view->is_upstream)
			count++;
	}
	return count;
}


bool
BMailStatusWindow::HasItems(void)
{
	return CountVisibleItems() > 0;
}


void
BMailStatusWindow::SetShowCriterion(uint32 when)
{
	if (!Lock())
		return;

	fShowMode = when;
	if (fShowMode == B_MAIL_SHOW_STATUS_WINDOW_ALWAYS
		|| (fShowMode != B_MAIL_SHOW_STATUS_WINDOW_NEVER && HasItems()))
	{
		while (IsHidden())
			Show();
	} else {
		while (!IsHidden())
			Hide();
	}
	Unlock();
}


void
BMailStatusWindow::SetBorderStyle(int32 look)
{
	switch (look) {
		case B_MAIL_STATUS_LOOK_TITLED:
			SetLook(B_TITLED_WINDOW_LOOK);
			break;
		case B_MAIL_STATUS_LOOK_FLOATING:
			SetLook(B_FLOATING_WINDOW_LOOK);
			break;
		case B_MAIL_STATUS_LOOK_THIN_BORDER:
			SetLook(B_BORDERED_WINDOW_LOOK);
			break;
		case B_MAIL_STATUS_LOOK_NO_BORDER:
			SetLook(B_NO_BORDER_WINDOW_LOOK);
			break;

		case B_MAIL_STATUS_LOOK_NORMAL_BORDER:
		default:
			SetLook(B_MODAL_WINDOW_LOOK);
	}
}


//	#pragma mark -
//------------------------------------------------
//
// BMailStatusView
//
//------------------------------------------------


BMailStatusView::BMailStatusView(BRect rect, const char *description,bool upstream)
		  : BBox(rect, description, B_FOLLOW_LEFT_RIGHT,
		  		 B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
		  		 B_PLAIN_BORDER)
{
	status = new BStatusBar(BRect(5, 5, Bounds().right - 5, Bounds().bottom - 5),
							"status_bar", description, "");
	status->SetResizingMode(B_FOLLOW_ALL_SIDES);
	status->SetBarHeight(12);

	if (!upstream) {
		const rgb_color downstreamColor = {48,176,48,255};	// upstream was: {255,100,50,255}
		status->SetBarColor(downstreamColor);
	}
	AddChild(status);

	items_now = 0;
	total_items = 0;
	pre_text[0] = 0;
	is_upstream = upstream;
}


BMailStatusView::~BMailStatusView()
{
}


void
BMailStatusView::AddProgress(int32 how_much)
{
	AddSelfToWindow();
	
	if (LockLooper()) {
		if (status->CurrentValue() == 0)
			strcpy(pre_text,status->TrailingText());
		char final[80];
		if (by_bytes) {
			sprintf(final,"%.1f / %.1f kb (%d / %d messages)",float(float(status->CurrentValue() + how_much) / 1024),float(float(status->MaxValue()) / 1024),(int)items_now+1,(int)total_items);
			status->Update(how_much,NULL,final);
		} else {
			sprintf(final,"%d / %d messages",(int)items_now,(int)total_items);
			status->Update(how_much,NULL,final);
		}
		UnlockLooper();
	}
}


void
BMailStatusView::SetMessage(const char *msg)
{
	AddSelfToWindow();

	if (LockLooper()) {
		status->SetTrailingText(msg);
		UnlockLooper();
	}
}


void
BMailStatusView::Reset(bool hide)
{
	if (LockLooper()) {
		char old[255];
		if ((pre_text[0] == 0) && !hide)
			strcpy(pre_text, status->TrailingText());
		if (hide)
			pre_text[0] = 0;

		strcpy(old,status->Label());
		status->Reset(old);
		status->SetTrailingText(pre_text);
		status->Draw(status->Bounds());
		pre_text[0] = 0;
		total_items = 0;
		items_now = 0;
		UnlockLooper();
	}
	if (hide && Window())
		window->RemoveView(this);
}


void
BMailStatusView::SetMaximum(int32 max_bytes)
{
	AddSelfToWindow();

	if (LockLooper()) {
		if (max_bytes < 0) {
			status->SetMaxValue(total_items);
			by_bytes = false;
		} else {
			status->SetMaxValue(max_bytes);
			by_bytes = true;
		}
		UnlockLooper();
	}
}


void
BMailStatusView::SetTotalItems(int32 items)
{
	AddSelfToWindow();
	total_items = items;
}


int32
BMailStatusView::CountTotalItems()
{
	return total_items;
}


void
BMailStatusView::AddItem(void)
{
	AddSelfToWindow();
	items_now++;

	if (!by_bytes)
		AddProgress(1);
}


void
BMailStatusView::AddSelfToWindow()
{
	if (Window() != NULL)
		return;
	
	window->ActuallyAddStatusView(this);
}

