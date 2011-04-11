/*
 * Copyright 2004-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Doerfler, axeld@pinc-software.de
 */

//!	Keyboard input server addon

#include "TeamMonitorWindow.h"

#include <stdio.h>

#include <CardLayout.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Roster.h>
#include <ScrollView.h>
#include <Screen.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <TextView.h>

#include <syscalls.h>
#include <tracker_private.h>

#include "KeyboardInputDevice.h"
#include "TeamListItem.h"


class TeamDescriptionView : public BBox {
public:
							TeamDescriptionView();
	virtual					~TeamDescriptionView();

	virtual void			MessageReceived(BMessage* message);
	virtual void			Draw(BRect bounds);
	virtual void			GetPreferredSize(float* _width, float* _height);

			void			CtrlAltDelPressed(bool keyDown);

			void			SetItem(TeamListItem* item);
			TeamListItem*	Item() { return fItem; }

private:
			const char*		fText[3];
			TeamListItem*	fItem;
			int32			fSeconds;
			BMessageRunner*	fRebootRunner;
};


static const uint32 kMsgUpdate = 'TMup';
static const uint32 kMsgLaunchTerminal = 'TMlt';
const uint32 TM_CANCEL = 'TMca';
const uint32 TM_FORCE_REBOOT = 'TMfr';
const uint32 TM_KILL_APPLICATION = 'TMka';
const uint32 TM_QUIT_APPLICATION = 'TMqa';
const uint32 TM_RESTART_DESKTOP = 'TMrd';
const uint32 TM_SELECTED_TEAM = 'TMst';

static const uint32 kMsgRebootTick = 'TMrt';


TeamMonitorWindow::TeamMonitorWindow()
	: BWindow(BRect(0, 0, 350, 300), "Team Monitor",
		B_TITLED_WINDOW_LOOK, B_MODAL_ALL_WINDOW_FEEL,
		B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
			| B_CLOSE_ON_ESCAPE,
		B_ALL_WORKSPACES),
	fQuitting(false),
	fUpdateRunner(NULL)
{
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
	float inset = 10;
	layout->SetInsets(inset, inset, inset, inset);
	layout->SetSpacing(inset);
	SetLayout(layout);

	layout->View()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fListView = new BListView("teams");
	fListView->SetSelectionMessage(new BMessage(TM_SELECTED_TEAM));

	BScrollView* scrollView = new BScrollView("scroll_teams", fListView,
		0, B_SUPPORTS_LAYOUT, false, true, B_FANCY_BORDER);
	layout->AddView(scrollView);

	BGroupView* groupView = new BGroupView(B_HORIZONTAL);
	layout->AddView(groupView);

	fKillButton = new BButton("kill", "Kill Application",
		new BMessage(TM_KILL_APPLICATION));
	groupView->AddChild(fKillButton);
	fKillButton->SetEnabled(false);
	
	fQuitButton = new BButton("quit", "Quit Application",
		new BMessage(TM_QUIT_APPLICATION));
	groupView->AddChild(fQuitButton);
	fQuitButton->SetEnabled(false);

	groupView->GroupLayout()->AddItem(BSpaceLayoutItem::CreateGlue());

	fDescriptionView = new TeamDescriptionView;
	layout->AddView(fDescriptionView);

	groupView = new BGroupView(B_HORIZONTAL);
	layout->AddView(groupView);

	BButton* forceReboot = new BButton("force", "Force Reboot",
		new BMessage(TM_FORCE_REBOOT));
	groupView->GroupLayout()->AddView(forceReboot);

	BSpaceLayoutItem* glue = BSpaceLayoutItem::CreateGlue();
	glue->SetExplicitMinSize(BSize(inset, -1));
	groupView->GroupLayout()->AddItem(glue);

	fRestartButton = new BButton("restart", "Restart the Desktop",
		new BMessage(TM_RESTART_DESKTOP));
	SetDefaultButton(fRestartButton);
	groupView->GroupLayout()->AddView(fRestartButton);

	glue = BSpaceLayoutItem::CreateGlue();
	glue->SetExplicitMinSize(BSize(inset, -1));
	groupView->GroupLayout()->AddItem(glue);

	fCancelButton = new BButton("cancel", "Cancel",
		new BMessage(TM_CANCEL));
	SetDefaultButton(fCancelButton);
	groupView->GroupLayout()->AddView(fCancelButton);

	BSize preferredSize = layout->View()->PreferredSize();
	if (preferredSize.width > Bounds().Width())
		ResizeTo(preferredSize.width, Bounds().Height());
	if (preferredSize.height > Bounds().Height())
		ResizeTo(Bounds().Width(), preferredSize.height);

	BRect screenFrame = BScreen(this).Frame();
	BPoint point;
	point.x = (screenFrame.Width() - Bounds().Width()) / 2;
	point.y = (screenFrame.Height() - Bounds().Height()) / 2;

	if (screenFrame.Contains(point))
		MoveTo(point);

	SetSizeLimits(Bounds().Width(), Bounds().Width() * 2,
		Bounds().Height(), screenFrame.Height());

	fRestartButton->Hide();

	AddShortcut('T', B_COMMAND_KEY | B_OPTION_KEY,
		new BMessage(kMsgLaunchTerminal));
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
}


TeamMonitorWindow::~TeamMonitorWindow()
{
}


void
TeamMonitorWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case SYSTEM_SHUTTING_DOWN:
			fQuitting = true;
			break;

		case kMsgUpdate:
			UpdateList();
			break;

		case kMsgCtrlAltDelPressed:
			bool keyDown;
			if (msg->FindBool("key down", &keyDown) != B_OK)
				break;

			fDescriptionView->CtrlAltDelPressed(keyDown);
			break;

		case kMsgLaunchTerminal:
			be_roster->Launch("application/x-vnd.Haiku-Terminal");
			break;

		case TM_FORCE_REBOOT:
			_kern_shutdown(true);
			break;
			
		case TM_KILL_APPLICATION:
		{
			TeamListItem* item = dynamic_cast<TeamListItem*>(fListView->ItemAt(
				fListView->CurrentSelection()));
			if (item != NULL) {
				kill_team(item->GetInfo()->team);
				UpdateList();
			}
			break;
		}
		case TM_QUIT_APPLICATION:
		{
			TeamListItem* item = dynamic_cast<TeamListItem*>(fListView->ItemAt(
				fListView->CurrentSelection()));
			if (item != NULL) {
				BMessenger messenger(item->AppSignature()->String(),
					item->GetInfo()->team);
				messenger.SendMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		case TM_RESTART_DESKTOP:
		{
			if (!be_roster->IsRunning(kTrackerSignature))
				be_roster->Launch(kTrackerSignature);
			if (!be_roster->IsRunning(kDeskbarSignature))
				be_roster->Launch(kDeskbarSignature);
			fRestartButton->Hide();
			SetDefaultButton(fCancelButton);
			break;
		}
		case TM_SELECTED_TEAM:
		{
			fKillButton->SetEnabled(fListView->CurrentSelection() >= 0);
			TeamListItem* item = dynamic_cast<TeamListItem*>(fListView->ItemAt(
				fListView->CurrentSelection()));
			fDescriptionView->SetItem(item);
			fQuitButton->SetEnabled(item != NULL && item->IsApplication());
			break;
		}
		case TM_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


bool
TeamMonitorWindow::QuitRequested()
{
	Disable();
	return fQuitting;
}


void
TeamMonitorWindow::UpdateList()
{
	bool changed = false;

	for (int32 i = 0; i < fListView->CountItems(); i++) {
		TeamListItem* item = dynamic_cast<TeamListItem*>(fListView->ItemAt(i));
		if (item != NULL)
			item->SetFound(false);
	}

	int32 cookie = 0;
	team_info info;
	while (get_next_team_info(&cookie, &info) == B_OK) {
		if (info.team <=16)
			continue;

		bool found = false;
		for (int32 i = 0; i < fListView->CountItems(); i++) {
			TeamListItem* item = dynamic_cast<TeamListItem*>(fListView->ItemAt(i));
			if (item != NULL && item->GetInfo()->team == info.team) {
				item->SetFound(true);
				found = true;
			}
		}

		if (!found) {
			TeamListItem* item = new TeamListItem(info);

			fListView->AddItem(item,
				item->IsSystemServer() ? fListView->CountItems() : 0);
			item->SetFound(true);
			changed = true;
		}
	}

	for (int32 i = fListView->CountItems() - 1; i >= 0; i--) {
		TeamListItem* item = dynamic_cast<TeamListItem*>(fListView->ItemAt(i));
		if (item != NULL && !item->Found()) {
			if (item == fDescriptionView->Item()) {
				fDescriptionView->SetItem(NULL);
				fKillButton->SetEnabled(false);
				fQuitButton->SetEnabled(false);
			}

			delete fListView->RemoveItem(i);
			changed = true;
		}
	}

	if (changed)
		fListView->Invalidate();

	bool desktopRunning = be_roster->IsRunning(kTrackerSignature)
		&& be_roster->IsRunning(kDeskbarSignature);
	if (!desktopRunning && fRestartButton->IsHidden()) {
		fRestartButton->Show();
		SetDefaultButton(fRestartButton);
		fRestartButton->Parent()->Layout(true);
	}

	fRestartButton->SetEnabled(!desktopRunning);
}


void
TeamMonitorWindow::Enable()
{
	if (Lock()) {
		if (IsHidden()) {
			BMessage message(kMsgUpdate);
			fUpdateRunner = new BMessageRunner(this, &message, 1000000LL);

			UpdateList();
			Show();
		}
		Unlock();
	}
}


void
TeamMonitorWindow::Disable()
{
	fListView->DeselectAll();
	delete fUpdateRunner;
	fUpdateRunner = NULL;
	Hide();
}


//	#pragma mark -


TeamDescriptionView::TeamDescriptionView()
	: BBox("description view", B_WILL_DRAW | B_PULSE_NEEDED, B_NO_BORDER),
	fItem(NULL),
	fSeconds(4),
	fRebootRunner(NULL)
{
/*
	BTextView* textView = new BTextView("description");
	textView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	textView->MakeEditable(false);
	textView->SetText("Select an application from the list above and click "
		"the \"Kill Application\" button in order to close it.\n\n"
		"Hold CONTROL+ALT+DELETE for %ld seconds to reboot.");
	view->AddChild(textView);
	((BCardLayout*)view->GetLayout())->SetVisibleItem((int32)0);
*/
	SetFont(be_plain_font);

	fText[0] = "Select an application from the list above and click the";
	fText[1] = "\"Kill Application\" button in order to close it.";
	fText[2] = "Hold CONTROL+ALT+DELETE for %ld seconds to reboot.";

	float width, height;
	GetPreferredSize(&width, &height);
	SetExplicitMinSize(BSize(width, -1));
	SetExplicitPreferredSize(BSize(width, height));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, height));
}


TeamDescriptionView::~TeamDescriptionView()
{
	delete fRebootRunner;
}


void
TeamDescriptionView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgRebootTick:
			fSeconds--;
			if (fSeconds == 0)
				Window()->PostMessage(TM_FORCE_REBOOT);
			else
				Invalidate();
			break;

		default:
			BBox::MessageReceived(message);
	}
}


void
TeamDescriptionView::CtrlAltDelPressed(bool keyDown)
{
	if (!(keyDown ^ (fRebootRunner != NULL)))
		return;

	delete fRebootRunner;
	fRebootRunner = NULL;
	fSeconds = 4;

	if (keyDown) {
		BMessage tick(kMsgRebootTick);
		fRebootRunner = new BMessageRunner(this, &tick, 1000000LL);
	} else
		Invalidate();
}


void
TeamDescriptionView::Draw(BRect rect)
{
	rect = Bounds();

	font_height	fontInfo;
	GetFontHeight(&fontInfo);
	float height
		= ceil(fontInfo.ascent + fontInfo.descent + fontInfo.leading + 2);

	if (fItem) {
		// draw icon and application path
		BRect frame(rect);
		frame.Set(frame.left, frame.top, frame.left + 31, frame.top + 31);
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		DrawBitmap(fItem->LargeIcon(), frame);
		SetDrawingMode(B_OP_COPY);

		BPoint line(frame.right + 9, frame.top + fontInfo.ascent);
		if (!fItem->IsSystemServer())
			line.y += (frame.Height() - height) / 2;
		else
			line.y += (frame.Height() - 2 * height) / 2;

		BString path = fItem->Path()->Path();
		TruncateString(&path, B_TRUNCATE_MIDDLE, rect.right - line.x);
		DrawString(path.String(), line);

		if (fItem->IsSystemServer()) {
			line.y += height;
			//SetFont(be_bold_font);
			DrawString("(This team is a system component)", line);
			//SetFont(be_plain_font);
		}
	} else {
		BPoint line(rect.left, rect.top + fontInfo.ascent);

		for (int32 i = 0; i < 2; i++) {
			DrawString(fText[i], line);
			line.y += height;
		}

		char text[256];
		if (fSeconds >= 0)
			snprintf(text, sizeof(text), fText[2], fSeconds);
		else
			strcpy(text, "Booom!");

		line.y += height;
		DrawString(text, line);
	}
}


void
TeamDescriptionView::GetPreferredSize(float* _width, float* _height)
{
	if (_width != NULL) {
		float width = 0;
		for (int32 i = 0; i < 3; i++) {
			float stringWidth = ceilf(StringWidth(fText[i]));
			if (stringWidth > width)
				width = stringWidth;
		}

		if (width < 330)
			width = 330;

		*_width = width;
	}

	if (_height != NULL) {
		font_height	fontInfo;
		GetFontHeight(&fontInfo);

		float height = 4 * ceil(fontInfo.ascent + fontInfo.descent
			+ fontInfo.leading + 2);
		if (height < 32)
			height = 32;

		*_height = height;
	}
}


void
TeamDescriptionView::SetItem(TeamListItem* item)
{
	fItem = item;
	Invalidate();
}
