/*
 * Copyright 2009-2010, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Copyright 2013-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "TeamsWindow.h"


#include <new>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <Application.h>
#include <Button.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Menu.h>
#include <MenuField.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "AppMessageCodes.h"
#include "SettingsManager.h"
#include "TargetHostInterface.h"
#include "TargetHostInterfaceRoster.h"
#include "TeamsListView.h"


enum {
	MSG_TEAM_SELECTION_CHANGED = 'tesc',
	MSG_CHOSE_CORE_FILE = 'chcf',
	MSG_SWITCH_TARGET_CONNECTION = 'stco'
};


TeamsWindow::TeamsWindow(SettingsManager* settingsManager)
	:
	BWindow(BRect(100, 100, 500, 250), "Teams", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fTargetHostInterface(NULL),
	fTeamsListView(NULL),
	fAttachTeamButton(NULL),
	fCreateTeamButton(NULL),
	fLoadCoreButton(NULL),
	fConnectionField(NULL),
	fCoreSelectionPanel(NULL),
	fSettingsManager(settingsManager),
	fListeners()
{
	team_info info;
	get_team_info(B_CURRENT_TEAM, &info);
	fCurrentTeam = info.team;
}


TeamsWindow::~TeamsWindow()
{
}


/*static*/ TeamsWindow*
TeamsWindow::Create(SettingsManager* settingsManager)
{
	TeamsWindow* self = new TeamsWindow(settingsManager);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
TeamsWindow::Zoom(BPoint, float, float)
{
	BSize preferredSize = fTeamsListView->PreferredSize();
	ResizeBy(preferredSize.Width() - Bounds().Width(),
		0.0);

	// if the new size would extend us past the screen border,
	// move sufficiently to the left to bring us back within the bounds
	// + a bit of extra margin so we're not flush against the edge.
	BScreen screen;
	float offset = screen.Frame().right - Frame().right;
	if (offset < 0)
		MoveBy(offset - 5.0, 0.0);
}


void
TeamsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_DEBUG_THIS_TEAM:
		{
			TeamRow* row = dynamic_cast<TeamRow*>(
				fTeamsListView->CurrentSelection());
			if (row != NULL) {
				BMessage message(MSG_DEBUG_THIS_TEAM);
				message.AddInt32("team", row->TeamID());
				message.AddPointer("interface", fTargetHostInterface);
				be_app_messenger.SendMessage(&message);
			}
			break;
		}

		case MSG_TEAM_SELECTION_CHANGED:
		{
			TeamRow* row = dynamic_cast<TeamRow*>(
				fTeamsListView->CurrentSelection());
			bool enabled = false;
			if (row != NULL) {
				team_id id = row->TeamID();
				if (id != fCurrentTeam && id != B_SYSTEM_TEAM)
					enabled = true;
			}

			fAttachTeamButton->SetEnabled(enabled);
			break;
		}

		case MSG_SHOW_START_TEAM_WINDOW:
		{
			BMessage startMessage(*message);
			startMessage.AddPointer("interface", fTargetHostInterface);
			be_app_messenger.SendMessage(&startMessage);
			break;
		}

		case MSG_CHOSE_CORE_FILE:
		case B_CANCEL:
		{
			delete fCoreSelectionPanel;
			fCoreSelectionPanel = NULL;
			if (message->what == B_CANCEL)
				break;

			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;

			BMessage coreMessage(MSG_LOAD_CORE_TEAM);
			coreMessage.AddPointer("interface", fTargetHostInterface);
			coreMessage.AddRef("core", &ref);
			be_app_messenger.SendMessage(&coreMessage);
			break;
		}

		case MSG_LOAD_CORE_TEAM:
		{
			try {
				BMessenger messenger(this);
				fCoreSelectionPanel = new BFilePanel(B_OPEN_PANEL, &messenger,
					NULL, 0, false, new BMessage(MSG_CHOSE_CORE_FILE));
				fCoreSelectionPanel->Show();
			} catch (...) {
				delete fCoreSelectionPanel;
				fCoreSelectionPanel = NULL;
			}
			break;
		}

		case MSG_SWITCH_TARGET_CONNECTION:
		{
			TargetHostInterface* interface;
			if (message->FindPointer("interface", reinterpret_cast<void**>(
					&interface)) != B_OK) {
				break;
			}

			if (interface == fTargetHostInterface)
				break;

			fTargetHostInterface = interface;
			_NotifySelectedInterfaceChanged(interface);
			fLoadCoreButton->SetEnabled(fTargetHostInterface->IsLocal());
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
TeamsWindow::QuitRequested()
{
	_SaveSettings();

	be_app_messenger.SendMessage(MSG_TEAMS_WINDOW_CLOSED);
	return true;
}


void
TeamsWindow::AddListener(Listener* listener)
{
	AutoLocker<TeamsWindow> locker(this);
	fListeners.Add(listener);
}


void
TeamsWindow::RemoveListener(Listener* listener)
{
	AutoLocker<TeamsWindow> locker(this);
	fListeners.Remove(listener);
}


// #pragma mark --


void
TeamsWindow::_Init()
{
	BMessage settings;
	_LoadSettings(settings);

	BRect frame;
	if (settings.FindRect("teams window frame", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}

	BMenu* connectionMenu = new BMenu("Connection");
	ObjectDeleter<BMenu> menuDeleter(connectionMenu);
	connectionMenu->SetLabelFromMarked(true);

	TargetHostInterfaceRoster* roster = TargetHostInterfaceRoster::Default();
	for (int32 i = 0; i < roster->CountActiveInterfaces(); i++) {
		TargetHostInterface* interface = roster->ActiveInterfaceAt(i);
		BMenuItem* item = new BMenuItem(interface->GetTargetHost()->Name(),
			new BMessage(MSG_SWITCH_TARGET_CONNECTION));
		if (item->Message()->AddPointer("interface", interface) != B_OK) {
			delete item;
			throw std::bad_alloc();
		}

		if (interface->IsLocal()) {
			item->SetMarked(true);
			fTargetHostInterface = interface;
		}

		connectionMenu->AddItem(item);
	}

	BGroupLayout* connectionLayout = NULL;

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.GetLayout(&connectionLayout)
			.Add(fConnectionField = new BMenuField("Connected to:",
				connectionMenu))
			.AddGlue()
			.Add(fCreateConnectionButton = new BButton("Create new connection"
					B_UTF8_ELLIPSIS, new BMessage(
						MSG_SHOW_CONNECTION_CONFIG_WINDOW)))
		.End()
		.Add(fTeamsListView = new TeamsListView("TeamsList"))
		.SetInsets(1.0f, 1.0f, 1.0f, 5.0f)
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(fAttachTeamButton = new BButton("Attach", new BMessage(
					MSG_DEBUG_THIS_TEAM)))
			.AddGlue()
			.Add(fCreateTeamButton = new BButton("Start new team"
					B_UTF8_ELLIPSIS, new BMessage(MSG_SHOW_START_TEAM_WINDOW)))
			.Add(fLoadCoreButton = new BButton("Load core" B_UTF8_ELLIPSIS,
					new BMessage(MSG_LOAD_CORE_TEAM)))
			.End()
		.End();

	connectionLayout->SetVisible(false);

	menuDeleter.Detach();

	AddListener(fTeamsListView);

	connectionMenu->SetTargetForItems(this);

	fTeamsListView->SetInvocationMessage(new BMessage(MSG_DEBUG_THIS_TEAM));
	fTeamsListView->SetSelectionMessage(new BMessage(
			MSG_TEAM_SELECTION_CHANGED));

	fAttachTeamButton->SetEnabled(false);
	fCreateTeamButton->SetTarget(this);
	fLoadCoreButton->SetTarget(this);
	fCreateConnectionButton->SetTarget(be_app);

	_NotifySelectedInterfaceChanged(fTargetHostInterface);
}


status_t
TeamsWindow::_OpenSettings(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("Debugger settings");

	return file.SetTo(path.Path(), mode);
}


status_t
TeamsWindow::_LoadSettings(BMessage& settings)
{
	BFile file;
	status_t status = _OpenSettings(file, B_READ_ONLY);
	if (status < B_OK)
		return status;

	return settings.Unflatten(&file);
}


status_t
TeamsWindow::_SaveSettings()
{
	BFile file;
	status_t status = _OpenSettings(file,
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);

	if (status < B_OK)
		return status;

	BMessage settings('hdbg');
	status = settings.AddRect("teams window frame", Frame());
	if (status != B_OK)
		return status;

	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}


void
TeamsWindow::_NotifySelectedInterfaceChanged(TargetHostInterface* interface)
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->SelectedInterfaceChanged(interface);
	}
}


// #pragma mark - TeamsWindow::Listener


TeamsWindow::Listener::~Listener()
{
}
