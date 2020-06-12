/*
 * Copyright 2004-2020, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Doerfler, axeld@pinc-software.de
 */

//!	Keyboard input server addon

#include "TeamMonitorWindow.h"

#include <stdio.h>

#include <Application.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <IconView.h>
#include <LayoutBuilder.h>
#include <LocaleRoster.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Roster.h>
#include <ScrollView.h>
#include <Screen.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <StringFormat.h>
#include <StringView.h>

#include <syscalls.h>
#include <tracker_private.h>

#include "KeyboardInputDevice.h"
#include "TeamListItem.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Team monitor"


TeamMonitorWindow* gTeamMonitorWindow = NULL;


struct TeamQuitter {
	team_id team;
	thread_id thread;
	BLooper* window;
};


status_t
QuitTeamThreadFunction(void* data)
{
	TeamQuitter* teamQuitter = reinterpret_cast<TeamQuitter*>(data);
	if (teamQuitter == NULL)
		return B_ERROR;

	status_t status;
	BMessenger messenger(NULL, teamQuitter->team, &status);
	if (status != B_OK)
		return status;

	BMessage message(B_QUIT_REQUESTED);
	BMessage reply;

	messenger.SendMessage(&message, &reply, 3000000, 3000000);

	bool result;
	if (reply.what != B_REPLY
		|| reply.FindBool("result", &result) != B_OK
		|| result == false) {
		message.what = kMsgQuitFailed;
		message.AddPointer("TeamQuitter", teamQuitter);
		message.AddInt32("error", reply.what);
		if (teamQuitter->window != NULL)
			teamQuitter->window->PostMessage(&message);
		return reply.what;
	}

	return B_OK;
}


filter_result
FilterLocaleChanged(BMessage* message, BHandler** target,
	BMessageFilter *filter)
{
	if (message->what == B_LOCALE_CHANGED && gTeamMonitorWindow != NULL)
		gTeamMonitorWindow->LocaleChanged();

	return B_DISPATCH_MESSAGE;
}


filter_result
FilterKeyDown(BMessage* message, BHandler** target,
	BMessageFilter *filter)
{
	if (message->what == B_KEY_DOWN && gTeamMonitorWindow != NULL) {
		if (gTeamMonitorWindow->HandleKeyDown(message))
			return B_SKIP_MESSAGE;
	}

	return B_DISPATCH_MESSAGE;
}


class TeamDescriptionView : public BView {
public:
							TeamDescriptionView();
	virtual					~TeamDescriptionView();

	virtual void			MessageReceived(BMessage* message);

			void			CtrlAltDelPressed(bool keyDown);

			void			SetItem(TeamListItem* item);
			TeamListItem*	Item() { return fItem; }

private:
			TeamListItem*	fItem;
			int32			fSeconds;
			BMessageRunner*	fRebootRunner;
			IconView*		fIconView;
			BCardLayout*	fLayout;
			BStringView*	fInfoTextView;

			BStringView*	fTeamName;
			BStringView*	fSysComponent;
			BStringView*	fQuitOverdue;
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
	:
	BWindow(BRect(0, 0, 350, 100), B_TRANSLATE("Team monitor"),
		B_TITLED_WINDOW_LOOK, B_MODAL_ALL_WINDOW_FEEL,
		B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS
			| B_CLOSE_ON_ESCAPE | B_AUTO_UPDATE_SIZE_LIMITS,
		B_ALL_WORKSPACES),
	fQuitting(false),
	fUpdateRunner(NULL)
{
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
	float inset = 10;
	layout->SetInsets(inset, inset, inset, inset);
	layout->SetSpacing(inset);
	SetLayout(layout);

	layout->View()->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fListView = new BListView("teams");
	fListView->SetSelectionMessage(new BMessage(TM_SELECTED_TEAM));

	BScrollView* scrollView = new BScrollView("scroll_teams", fListView,
		0, B_SUPPORTS_LAYOUT, false, true, B_FANCY_BORDER);
	scrollView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 150));

	fKillButton = new BButton("kill", B_TRANSLATE("Kill application"),
		new BMessage(TM_KILL_APPLICATION));
	fKillButton->SetEnabled(false);

	fQuitButton = new BButton("quit", B_TRANSLATE("Quit application"),
		new BMessage(TM_QUIT_APPLICATION));
	fQuitButton->SetEnabled(false);

	fDescriptionView = new TeamDescriptionView;

	BButton* forceReboot = new BButton("force", B_TRANSLATE("Force reboot"),
		new BMessage(TM_FORCE_REBOOT));

	fRestartButton = new BButton("restart", B_TRANSLATE("Restart the desktop"),
		new BMessage(TM_RESTART_DESKTOP));

	BButton* openTerminal = new BButton("terminal",
		B_TRANSLATE("Open Terminal"), new BMessage(kMsgLaunchTerminal));

	fCancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(TM_CANCEL));
	SetDefaultButton(fCancelButton);

	BGroupLayoutBuilder(layout)
		.Add(scrollView, 10)
		.AddGroup(B_HORIZONTAL)
			.SetInsets(0, 0, 0, 0)
			.Add(fKillButton)
			.Add(fQuitButton)
			.AddGlue()
			.End()
		.Add(fDescriptionView)
		.AddGroup(B_HORIZONTAL)
			.SetInsets(0, 0, 0, 0)
			.Add(forceReboot)
			.AddGlue()
			.Add(fRestartButton)
			.AddGlue(inset)
			.Add(openTerminal)
			.Add(fCancelButton);

	CenterOnScreen();

	fRestartButton->Hide();

	AddShortcut('T', B_COMMAND_KEY | B_OPTION_KEY,
		new BMessage(kMsgLaunchTerminal));
	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	gLocalizedNamePreferred
		= BLocaleRoster::Default()->IsFilesystemTranslationPreferred();

	gTeamMonitorWindow = this;

	this->AddCommonFilter(new BMessageFilter(B_ANY_DELIVERY,
		B_ANY_SOURCE, B_KEY_DOWN, FilterKeyDown));

	if (be_app->Lock()) {
		be_app->AddCommonFilter(new BMessageFilter(B_ANY_DELIVERY,
			B_ANY_SOURCE, B_LOCALE_CHANGED, FilterLocaleChanged));
		be_app->Unlock();
	}
}


TeamMonitorWindow::~TeamMonitorWindow()
{
	while (fTeamQuitterList.ItemAt(0) != NULL) {
		TeamQuitter* teamQuitter = reinterpret_cast<TeamQuitter*>
			(fTeamQuitterList.RemoveItem((int32) 0));
		if (teamQuitter != NULL) {
			status_t status;
			wait_for_thread(teamQuitter->thread, &status);
			delete teamQuitter;
		}
	}
}


void
TeamMonitorWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case SYSTEM_SHUTTING_DOWN:
			fQuitting = true;
			break;

		case kMsgUpdate:
			_UpdateList();
			break;

		case kMsgCtrlAltDelPressed:
			bool keyDown;
			if (msg->FindBool("key down", &keyDown) != B_OK)
				break;

			fDescriptionView->CtrlAltDelPressed(keyDown);
			break;

		case kMsgDeselectAll:
			fListView->DeselectAll();
			break;

		case kMsgLaunchTerminal:
			be_roster->Launch("application/x-vnd.Haiku-Terminal");
			PostMessage(B_QUIT_REQUESTED);
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
				_UpdateList();
			}
			break;
		}
		case TM_QUIT_APPLICATION:
		{
			TeamListItem* item = dynamic_cast<TeamListItem*>(fListView->ItemAt(
				fListView->CurrentSelection()));
			if (item != NULL) {
				QuitTeam(item);
			}
			break;
		}
		case kMsgQuitFailed:
			MarkUnquittableTeam(msg);
			break;

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


void
TeamMonitorWindow::Show()
{
	fListView->MakeFocus();
	BWindow::Show();
}


bool
TeamMonitorWindow::QuitRequested()
{
	Disable();
	return fQuitting;
}


void
TeamMonitorWindow::Enable()
{
	if (Lock()) {
		if (IsHidden()) {
			BMessage message(kMsgUpdate);
			fUpdateRunner = new BMessageRunner(this, &message, 1000000LL);

			_UpdateList();
			Show();
		}
		Unlock();
	}

	// Not sure why this is needed, but without it the layout isn't correct
	// when the window is first shown and the buttons at the bottom aren't
	// visible.
	InvalidateLayout();
}


void
TeamMonitorWindow::Disable()
{
	delete fUpdateRunner;
	fUpdateRunner = NULL;
	Hide();
	fListView->DeselectAll();
	for (int32 i = 0; i < fListView->CountItems(); i++) {
		TeamListItem* item = dynamic_cast<TeamListItem*>(fListView->ItemAt(i));
		if (item != NULL)
			item->SetRefusingToQuit(false);
	}
}


void
TeamMonitorWindow::LocaleChanged()
{
	BLocaleRoster::Default()->Refresh();
	gLocalizedNamePreferred
		= BLocaleRoster::Default()->IsFilesystemTranslationPreferred();

	for (int32 i = 0; i < fListView->CountItems(); i++) {
		TeamListItem* item
			= dynamic_cast<TeamListItem*>(fListView->ItemAt(i));
		if (item != NULL)
			item->CacheLocalizedName();
	}
}


void
TeamMonitorWindow::QuitTeam(TeamListItem* item)
{
	if (item == NULL)
		return;

	TeamQuitter* teamQuitter = new TeamQuitter;
	teamQuitter->team = item->GetInfo()->team;
	teamQuitter->window = this;
	teamQuitter->thread = spawn_thread(QuitTeamThreadFunction,
		"team quitter", B_DISPLAY_PRIORITY, teamQuitter);

	if (teamQuitter->thread < 0) {
		delete teamQuitter;
		return;
	}

	fTeamQuitterList.AddItem(teamQuitter);

	if (resume_thread(teamQuitter->thread) != B_OK) {
		fTeamQuitterList.RemoveItem(teamQuitter);
		delete teamQuitter;
	}
}


void
TeamMonitorWindow::MarkUnquittableTeam(BMessage* message)
{
	if (message == NULL)
		return;

	int32 reply;
	if (message->FindInt32("error", &reply) != B_OK)
		return;

	TeamQuitter* teamQuitter;
	if (message->FindPointer("TeamQuitter",
		reinterpret_cast<void**>(&teamQuitter)) != B_OK)
		return;

	for (int32 i = 0; i < fListView->CountItems(); i++) {
		TeamListItem* item
			= dynamic_cast<TeamListItem*>(fListView->ItemAt(i));
		if (item != NULL && item->GetInfo()->team == teamQuitter->team) {
			item->SetRefusingToQuit(true);
			fListView->Select(i);
			fListView->InvalidateItem(i);
			fDescriptionView->SetItem(item);
			break;
		}
	}

	fTeamQuitterList.RemoveItem(teamQuitter);
	delete teamQuitter;
}


bool
TeamMonitorWindow::HandleKeyDown(BMessage* msg)
{
	uint32 rawChar = msg->FindInt32("raw_char");
	uint32 modifier = msg->FindInt32("modifiers");

	// Ignore the system modifier namespace
	if ((modifier & (B_CONTROL_KEY | B_COMMAND_KEY))
			== (B_CONTROL_KEY | B_COMMAND_KEY))
		return false;

	bool quit = false;
	bool kill = false;
	switch (rawChar) {
		case B_DELETE:
			if (modifier & B_SHIFT_KEY)
				kill = true;
			else
				quit = true;
			break;
		case 'q':
		case 'Q':
			quit = true;
			break;
		case 'k':
		case 'K':
			kill = true;
			break;
	}

	if (quit) {
		PostMessage(TM_QUIT_APPLICATION);
		return true;
	} else if (kill) {
		PostMessage(TM_KILL_APPLICATION);
		return true;
	}

	return false;
}


void
TeamMonitorWindow::_UpdateList()
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
			TeamListItem* item
				= dynamic_cast<TeamListItem*>(fListView->ItemAt(i));
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


//	#pragma mark -


TeamDescriptionView::TeamDescriptionView()
	:
	BView("description view", B_WILL_DRAW),
	fItem(NULL),
	fSeconds(4),
	fRebootRunner(NULL)
{
	fTeamName = new BStringView("team name", "team name");
	fTeamName->SetTruncation(B_TRUNCATE_BEGINNING);
	fTeamName->SetExplicitSize(BSize(StringWidth("x") * 60, B_SIZE_UNSET));
	fSysComponent = new BStringView("system component", B_TRANSLATE(
		"(This team is a system component)"));
	fQuitOverdue = new BStringView("quit overdue", B_TRANSLATE(
		"If the application will not quit you may have to kill it."));
	fQuitOverdue->SetFont(be_bold_font);

	fInfoTextView = new BStringView("info text", "");
	fInfoTextView->SetLowUIColor(B_PANEL_BACKGROUND_COLOR);
	fInfoTextView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fIconView = new IconView();
	fIconView->SetExplicitAlignment(
		BAlignment(B_ALIGN_HORIZONTAL_UNSET, B_ALIGN_VERTICAL_CENTER));

	BView* teamPropertiesView = new BView("team properties", B_WILL_DRAW);
	teamPropertiesView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	BGridLayout* layout = new BGridLayout();
	teamPropertiesView->SetLayout(layout);

	BLayoutBuilder::Grid<>(layout)
		.SetInsets(0)
		.SetSpacing(B_USE_ITEM_SPACING, 0)
		.AddGlue(0, 0)
		.Add(fIconView, 0, 1, 1, 3)
		.AddGlue(1, 0)
		.Add(fTeamName, 1, 2)
		.Add(fSysComponent, 1, 3)
		.Add(fQuitOverdue, 1, 4)
		.AddGlue(0, 5)
		.AddGlue(2, 1);

	fLayout = new BCardLayout();
	SetLayout(fLayout);
	fLayout->AddView(fInfoTextView);
	fLayout->AddView(teamPropertiesView);

	SetItem(NULL);
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
				SetItem(fItem);
			break;

		default:
			BView::MessageReceived(message);
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
		Window()->PostMessage(kMsgDeselectAll);
		BMessage tick(kMsgRebootTick);
		fRebootRunner = new BMessageRunner(this, &tick, 1000000LL);
	}

	SetItem(NULL);
}


void
TeamDescriptionView::SetItem(TeamListItem* item)
{
	fItem = item;

	if (item == NULL) {
		if (fInfoTextView != NULL) {
			BFont font;
			fInfoTextView->GetFont(&font);
			font.SetFace(B_REGULAR_FACE);

			if (fRebootRunner != NULL && fSeconds < 4)
				font.SetFace(B_BOLD_FACE);

			fInfoTextView->SetFont(&font);

			static BStringFormat format(B_TRANSLATE("{0, plural,"
				"one{Hold CONTROL+ALT+DELETE for # second to reboot.}"
				"other{Hold CONTROL+ALT+DELETE for # seconds to reboot.}}"));
			BString text;
			format.Format(text, fSeconds);
			fInfoTextView->SetText(text);
		}
	} else {
		fTeamName->SetText(item->Path()->Path());

		if (item->IsSystemServer()) {
			if (fSysComponent->IsHidden(fSysComponent))
				fSysComponent->Show();
		} else {
			if (!fSysComponent->IsHidden(fSysComponent))
				fSysComponent->Hide();
		}

		if (item->IsRefusingToQuit()) {
			if (fQuitOverdue->IsHidden(fQuitOverdue))
				fQuitOverdue->Show();
		} else {
			if (!fQuitOverdue->IsHidden(fQuitOverdue))
				fQuitOverdue->Hide();
		}

		fIconView->SetIcon(item->Path()->Path());
	}

	if (fLayout == NULL)
		return;

	if (item == NULL)
		fLayout->SetVisibleItem((int32)0);
	else
		fLayout->SetVisibleItem((int32)1);

	Invalidate();
}


