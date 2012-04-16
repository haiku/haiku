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

#include <Application.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <IconView.h>
#include <LocaleRoster.h>
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


class AllShowingTextView : public BTextView {
public:
							AllShowingTextView(const char* name);
	virtual void			FrameResized(float width, float height);
};


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
	const	char*			fInfoString;
	const	char*			fSysComponentString;
	const	char*			fQuitOverdueString;
			BCardLayout*	fLayout;
			AllShowingTextView*	fIconTextView;
			AllShowingTextView*	fInfoTextView;
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
	BWindow(BRect(0, 0, 350, 400), B_TRANSLATE("Team monitor"),
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

	layout->View()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fListView = new BListView("teams");
	fListView->SetSelectionMessage(new BMessage(TM_SELECTED_TEAM));

	BScrollView* scrollView = new BScrollView("scroll_teams", fListView,
		0, B_SUPPORTS_LAYOUT, false, true, B_FANCY_BORDER);
	layout->AddView(scrollView);

	BGroupView* groupView = new BGroupView(B_HORIZONTAL);
	layout->AddView(groupView);

	fKillButton = new BButton("kill", B_TRANSLATE("Kill application"),
		new BMessage(TM_KILL_APPLICATION));
	groupView->AddChild(fKillButton);
	fKillButton->SetEnabled(false);
	
	fQuitButton = new BButton("quit", B_TRANSLATE("Quit application"),
		new BMessage(TM_QUIT_APPLICATION));
	groupView->AddChild(fQuitButton);
	fQuitButton->SetEnabled(false);

	groupView->GroupLayout()->AddItem(BSpaceLayoutItem::CreateGlue());

	fDescriptionView = new TeamDescriptionView;
	layout->AddView(fDescriptionView);

	groupView = new BGroupView(B_HORIZONTAL);
	layout->AddView(groupView);

	BButton* forceReboot = new BButton("force", B_TRANSLATE("Force reboot"),
		new BMessage(TM_FORCE_REBOOT));
	groupView->GroupLayout()->AddView(forceReboot);

	BSpaceLayoutItem* glue = BSpaceLayoutItem::CreateGlue();
	glue->SetExplicitMinSize(BSize(inset, -1));
	groupView->GroupLayout()->AddItem(glue);

	fRestartButton = new BButton("restart", B_TRANSLATE("Restart the desktop"),
		new BMessage(TM_RESTART_DESKTOP));
	SetDefaultButton(fRestartButton);
	groupView->GroupLayout()->AddView(fRestartButton);

	glue = BSpaceLayoutItem::CreateGlue();
	glue->SetExplicitMinSize(BSize(inset, -1));
	groupView->GroupLayout()->AddItem(glue);

	fCancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
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

	gLocalizedNamePreferred
		= BLocaleRoster::Default()->IsFilesystemTranslationPreferred();

	gTeamMonitorWindow = this;

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
			UpdateList();
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


//	#pragma mark -


TeamDescriptionView::TeamDescriptionView()
	:
	BView("description view", B_WILL_DRAW),
	fItem(NULL),
	fSeconds(4),
	fRebootRunner(NULL)
{
	fInfoString = B_TRANSLATE(
		"Select an application from the list above and click one of "
		"the buttons 'Kill application' and 'Quit application' "
		"in order to close it.\n\n"
		"Hold CONTROL+ALT+DELETE for %ld seconds to reboot.");

	fSysComponentString = B_TRANSLATE("(This team is a system component)");
	fQuitOverdueString = B_TRANSLATE("If the application will not quit "
		"you may have to kill it.");

	fInfoTextView = new AllShowingTextView("info text");
	fIconTextView = new AllShowingTextView("icon text");

	fIconView = new IconView();
	fIconView->SetExplicitAlignment(
		BAlignment(B_ALIGN_HORIZONTAL_UNSET, B_ALIGN_TOP));

	BView* teamPropertiesView = new BView("team properties", B_WILL_DRAW);
	teamPropertiesView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BGroupLayout* layout = new BGroupLayout(B_HORIZONTAL);
	teamPropertiesView->SetLayout(layout);
	BGroupLayoutBuilder(layout)
		.Add(fIconView)
		.Add(fIconTextView);

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
	int32 styleStart = 0;
	int32 styleEnd = 0;
	BTextView* view = NULL;

	if (item == NULL) {
		BString text;
		text.SetToFormat(fInfoString, fSeconds);
		fInfoTextView->SetText(text);
		if (fRebootRunner != NULL && fSeconds < 4) {
			styleStart = text.FindLast('\n');
			styleEnd = text.Length();
		}
		view = fInfoTextView;
	} else {
		BString text = item->Path()->Path();
		if (item->IsSystemServer())
			text << "\n" << fSysComponentString;
		if (item->IsRefusingToQuit()) {
			text << "\n\n" << fQuitOverdueString;
			styleStart = text.FindLast('\n');
			styleEnd = text.Length();
		}
		view = fIconTextView;
		fIconTextView->SetText(text);
		fIconView->SetIcon(item->Path()->Path());
	}

	if (styleStart != styleEnd && view != NULL) {
		BFont font;
		view->GetFont(&font);
		font.SetFace(B_BOLD_FACE);
		view->SetStylable(true);
		view->SetFontAndColor(styleStart, styleEnd, &font);
	}

	if (fLayout == NULL)
		return;

	if (item == NULL)
		fLayout->SetVisibleItem((int32)0);
	else
		fLayout->SetVisibleItem((int32)1);

	Invalidate();
}


//	#pragma mark -


AllShowingTextView::AllShowingTextView(const char* name)
	:
	BTextView(name, B_WILL_DRAW)
{
	MakeEditable(false);
	MakeSelectable(false);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
AllShowingTextView::FrameResized(float width, float height)
{
	float minHeight = TextHeight(0, CountLines() - 1);
	SetExplicitMinSize(BSize(B_SIZE_UNSET, minHeight));
	BTextView::FrameResized(width, minHeight);
}
