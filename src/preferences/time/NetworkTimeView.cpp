/*
 * Copyright 2011-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Hamish Morrison, hamish@lavabit.com
 *		John Scipione, jscipione@gmail.com
 */


#include "NetworkTimeView.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <File.h>
#include <FindDirectory.h>
#include <Invoker.h>
#include <ListItem.h>
#include <ListView.h>
#include <Path.h>
#include <ScrollView.h>
#include <Size.h>
#include <TextControl.h>

#include "ntp.h"
#include "TimeMessages.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Time"


//	#pragma mark - Settings


Settings::Settings()
	:
	fMessage(kMsgNetworkTimeSettings)
{
	ResetToDefaults();
	Load();
}


Settings::~Settings()
{
	Save();
}


void
Settings::AddServer(const char* server)
{
	if (_GetStringByValue("server", server) == B_ERROR)
		fMessage.AddString("server", server);
}


const char*
Settings::GetServer(int32 index) const
{
	const char* server;
	fMessage.FindString("server", index, &server);
	return server;
}


void
Settings::RemoveServer(const char* server)
{
	int32 index = _GetStringByValue("server", server);
	if (index != B_ERROR) {
		fMessage.RemoveData("server", index);

		int32 count;
		fMessage.GetInfo("server", NULL, &count);
		if (GetDefaultServer() >= count)
			SetDefaultServer(count - 1);
	}
}


void
Settings::SetDefaultServer(int32 index)
{
	if (fMessage.ReplaceInt32("default server", index) != B_OK)
		fMessage.AddInt32("default server", index);
}


int32
Settings::GetDefaultServer() const
{
	int32 index;
	fMessage.FindInt32("default server", &index);
	return index;
}


void
Settings::SetTryAllServers(bool boolean)
{
	fMessage.ReplaceBool("try all servers", boolean);
}


bool
Settings::GetTryAllServers() const
{
	bool boolean;
	fMessage.FindBool("try all servers", &boolean);
	return boolean;
}


void
Settings::SetSynchronizeAtBoot(bool boolean)
{
	fMessage.ReplaceBool("synchronize at boot", boolean);
}


bool
Settings::GetSynchronizeAtBoot() const
{
	bool boolean;
	fMessage.FindBool("synchronize at boot", &boolean);
	return boolean;
}


void
Settings::ResetServersToDefaults()
{
	fMessage.RemoveName("server");

	fMessage.AddString("server", "pool.ntp.org");
	fMessage.AddString("server", "de.pool.ntp.org");
	fMessage.AddString("server", "time.nist.gov");

	if (fMessage.ReplaceInt32("default server", 0) != B_OK)
		fMessage.AddInt32("default server", 0);
}


void
Settings::ResetToDefaults()
{
	fMessage.MakeEmpty();
	ResetServersToDefaults();

	fMessage.AddBool("synchronize at boot", true);
	fMessage.AddBool("try all servers", true);
}


void
Settings::Revert()
{
	fMessage = fOldMessage;
}


bool
Settings::SettingsChanged()
{
	ssize_t oldSize = fOldMessage.FlattenedSize();
	ssize_t newSize = fMessage.FlattenedSize();

	if (oldSize != newSize || oldSize < 0 || newSize < 0)
		return true;

	char* oldBytes = new (std::nothrow) char[oldSize];
	if (oldBytes == NULL)
		return true;

	fOldMessage.Flatten(oldBytes, oldSize);
	char* newBytes = new (std::nothrow) char[newSize];
	if (newBytes == NULL) {
		delete[] oldBytes;
		return true;
	}
	fMessage.Flatten(newBytes, newSize);

	int result = memcmp(oldBytes, newBytes, oldSize);

	delete[] oldBytes;
	delete[] newBytes;

	return result != 0;
}


status_t
Settings::Load()
{
	status_t status;

	BPath path;
	if ((status = _GetPath(path)) != B_OK)
		return status;

	BFile file(path.Path(), B_READ_ONLY);
	if ((status = file.InitCheck()) != B_OK)
		return status;

	BMessage load;
	if ((status = load.Unflatten(&file)) != B_OK)
		return status;

	if (load.what != kMsgNetworkTimeSettings)
		return B_BAD_TYPE;

	fMessage = load;
	fOldMessage = fMessage;
	return B_OK;
}


status_t
Settings::Save()
{
	status_t status;

	BPath path;
	if ((status = _GetPath(path)) != B_OK)
		return status;

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if ((status = file.InitCheck()) != B_OK)
		return status;

	file.SetSize(0);

	return fMessage.Flatten(&file);
}


int32
Settings::_GetStringByValue(const char* name, const char* value)
{
	const char* string;
	for (int32 index = 0; fMessage.FindString(name, index, &string) == B_OK;
			index++) {
		if (strcmp(string, value) == 0)
			return index;
	}

	return B_ERROR;
}


status_t
Settings::_GetPath(BPath& path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	path.Append("networktime settings");

	return B_OK;
}


//	#pragma mark - NetworkTimeView


NetworkTimeView::NetworkTimeView(const char* name)
	:
	BGroupView(name, B_VERTICAL, B_USE_DEFAULT_SPACING),
	fSettings(),
	fServerTextControl(NULL),
	fAddButton(NULL),
	fRemoveButton(NULL),
	fResetButton(NULL),
	fServerListView(NULL),
	fTryAllServersCheckBox(NULL),
	fSynchronizeAtBootCheckBox(NULL),
	fSynchronizeButton(NULL),
	fTextColor(ui_color(B_CONTROL_TEXT_COLOR)),
	fInvalidColor(ui_color(B_FAILURE_COLOR)),
	fUpdateThread(-1)
{
	fSettings.Load();
	_InitView();
}


NetworkTimeView::~NetworkTimeView()
{
	delete fServerTextControl;
	delete fAddButton;
	delete fRemoveButton;
	delete fResetButton;
	delete fServerListView;
	delete fTryAllServersCheckBox;
	delete fSynchronizeAtBootCheckBox;
	delete fSynchronizeButton;
}


void
NetworkTimeView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSetDefaultServer:
		{
			int32 currentSelection = fServerListView->CurrentSelection();
			if (currentSelection < 0)
				fServerListView->Select(fSettings.GetDefaultServer());
			else {
				fSettings.SetDefaultServer(currentSelection);
				Looper()->PostMessage(new BMessage(kMsgChange));
			}
			break;
		}

		case kMsgServerEdited:
		{
			bool isValid = _IsValidServerName(fServerTextControl->Text());
			fServerTextControl->TextView()->SetFontAndColor(0,
				fServerTextControl->TextView()->TextLength(), NULL, 0,
				isValid ? &fTextColor : &fInvalidColor);
			fAddButton->SetEnabled(isValid);
			break;
		}

		case kMsgAddServer:
			if (!_IsValidServerName(fServerTextControl->Text()))
				break;

			fSettings.AddServer(fServerTextControl->Text());
			_UpdateServerList();
			fServerTextControl->SetText("");
			Looper()->PostMessage(new BMessage(kMsgChange));
			break;

		case kMsgRemoveServer:
		{
			int32 currentSelection = fServerListView->CurrentSelection();
			if (currentSelection < 0)
				break;

			fSettings.RemoveServer(((BStringItem*)
				fServerListView->ItemAt(currentSelection))->Text());
			_UpdateServerList();
			Looper()->PostMessage(new BMessage(kMsgChange));
			break;
		}

		case kMsgResetServerList:
			fSettings.ResetServersToDefaults();
			_UpdateServerList();
			Looper()->PostMessage(new BMessage(kMsgChange));
			break;

		case kMsgTryAllServers:
			fSettings.SetTryAllServers(
				fTryAllServersCheckBox->Value());
			Looper()->PostMessage(new BMessage(kMsgChange));
			break;

		case kMsgSynchronizeAtBoot:
			fSettings.SetSynchronizeAtBoot(fSynchronizeAtBootCheckBox->Value());
			Looper()->PostMessage(new BMessage(kMsgChange));
			break;

		case kMsgStopSynchronization:
			if (fUpdateThread >= B_OK)
				kill_thread(fUpdateThread);

			_DoneSynchronizing();
			break;

		case kMsgSynchronize:
		{
			if (fUpdateThread >= B_OK)
				break;

			BMessenger* messenger = new BMessenger(this);
			update_time(fSettings, messenger, &fUpdateThread);
			fSynchronizeButton->SetLabel(B_TRANSLATE("Stop"));
			fSynchronizeButton->Message()->what = kMsgStopSynchronization;
			break;
		}

		case kMsgSynchronizationResult:
		{
			_DoneSynchronizing();

			status_t status;
			if (message->FindInt32("status", (int32 *)&status) == B_OK) {
				if (status == B_OK)
					return;

				const char* errorString;
				message->FindString("error string", &errorString);
				char buffer[256];

				int32 errorCode;
				if (message->FindInt32("error code", &errorCode) == B_OK) {
					snprintf(buffer, sizeof(buffer),
						B_TRANSLATE("The following error occured "
							"while synchronizing:\n%s: %s"),
						errorString, strerror(errorCode));
				} else {
					snprintf(buffer, sizeof(buffer),
						B_TRANSLATE("The following error occured "
							"while synchronizing:\n%s"),
						errorString);
				}

				BAlert* alert = new BAlert(B_TRANSLATE("Time"), buffer,
					B_TRANSLATE("OK"));
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go();
			}
			break;
		}

		case kMsgRevert:
			fSettings.Revert();
			fTryAllServersCheckBox->SetValue(fSettings.GetTryAllServers());
			fSynchronizeAtBootCheckBox->SetValue(
				fSettings.GetSynchronizeAtBoot());
			_UpdateServerList();
			break;

		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
NetworkTimeView::AttachedToWindow()
{
	fServerTextControl->SetTarget(this);
	fServerListView->SetTarget(this);
	fAddButton->SetTarget(this);
	fAddButton->SetEnabled(false);
	fRemoveButton->SetTarget(this);
	fResetButton->SetTarget(this);
	fTryAllServersCheckBox->SetTarget(this);
	fSynchronizeAtBootCheckBox->SetTarget(this);
	fSynchronizeButton->SetTarget(this);
}


bool
NetworkTimeView::CheckCanRevert()
{
	return fSettings.SettingsChanged();
}


void
NetworkTimeView::_InitView()
{
	fServerTextControl = new BTextControl(NULL, NULL,
		new BMessage(kMsgAddServer));
	fServerTextControl->SetModificationMessage(new BMessage(kMsgServerEdited));

	const float kButtonWidth = fServerTextControl->Frame().Height();

	fAddButton = new BButton("add", "+", new BMessage(kMsgAddServer));
	fAddButton->SetToolTip(B_TRANSLATE("Add"));
	fAddButton->SetExplicitSize(BSize(kButtonWidth, kButtonWidth));

	fRemoveButton = new BButton("remove", "−", new BMessage(kMsgRemoveServer));
	fRemoveButton->SetToolTip(B_TRANSLATE("Remove"));
	fRemoveButton->SetExplicitSize(BSize(kButtonWidth, kButtonWidth));

	fServerListView = new BListView("serverList");
	fServerListView->SetExplicitMinSize(BSize(B_SIZE_UNSET, kButtonWidth * 4));
	fServerListView->SetSelectionMessage(new BMessage(kMsgSetDefaultServer));
	BScrollView* scrollView = new BScrollView("serverScrollView",
		fServerListView, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	_UpdateServerList();

	fTryAllServersCheckBox = new BCheckBox("tryAllServers",
		B_TRANSLATE("Try all servers"), new BMessage(kMsgTryAllServers));
	fTryAllServersCheckBox->SetValue(fSettings.GetTryAllServers());

	fSynchronizeAtBootCheckBox = new BCheckBox("autoUpdate",
		B_TRANSLATE("Synchronize at boot"),
		new BMessage(kMsgSynchronizeAtBoot));
	fSynchronizeAtBootCheckBox->SetValue(fSettings.GetSynchronizeAtBoot());

	fResetButton = new BButton("reset",
		B_TRANSLATE("Reset to default server list"),
		new BMessage(kMsgResetServerList));

	fSynchronizeButton = new BButton("update", B_TRANSLATE("Synchronize"),
		new BMessage(kMsgSynchronize));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
			.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
				.Add(fServerTextControl)
				.Add(fAddButton)
			.End()
			.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
				.Add(scrollView)
				.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
					.Add(fRemoveButton)
					.AddGlue()
				.End()
			.End()
		.End()
		.AddGroup(B_HORIZONTAL)
			.AddGroup(B_VERTICAL, 0)
				.Add(fTryAllServersCheckBox)
				.Add(fSynchronizeAtBootCheckBox)
			.End()
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(fResetButton)
			.AddGlue()
			.Add(fSynchronizeButton)
		.End()
		.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
			B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING);
}


void
NetworkTimeView::_UpdateServerList()
{
	BListItem* item;
	while ((item = fServerListView->RemoveItem((int32)0)) != NULL)
		delete item;

	const char* server;
	int32 index = 0;
	while ((server = fSettings.GetServer(index++)) != NULL)
		fServerListView->AddItem(new BStringItem(server));

	fServerListView->Select(fSettings.GetDefaultServer());
	fServerListView->ScrollToSelection();

	fRemoveButton->SetEnabled(fServerListView->CountItems() > 0);
}


void
NetworkTimeView::_DoneSynchronizing()
{
	fUpdateThread = -1;
	fSynchronizeButton->SetLabel(B_TRANSLATE("Synchronize again"));
	fSynchronizeButton->Message()->what = kMsgSynchronize;
}


bool
NetworkTimeView::_IsValidServerName(const char* serverName)
{
	if (serverName == NULL || *serverName == '\0')
		return false;

	for (int32 i = 0; serverName[i] != '\0'; i++) {
		char c = serverName[i];
		// Simple URL validation, no scheme should be present
		if (!(isalnum(c) || c == '.' || c == '-' || c == '_'))
			return false;
	}

	return true;
}


//	#pragma mark - update functions


int32
update_thread(void* params)
{
	BList* list = (BList*)params;
	BMessenger* messenger = (BMessenger*)list->ItemAt(1);

	const char* errorString = NULL;
	int32 errorCode = 0;
	status_t status = update_time(*(Settings*)list->ItemAt(0),
		&errorString, &errorCode);

	BMessage result(kMsgSynchronizationResult);
	result.AddInt32("status", status);
	result.AddString("error string", errorString);
	if (errorCode != 0)
		result.AddInt32("error code", errorCode);

	messenger->SendMessage(&result);
	delete messenger;

	return B_OK;
}


status_t
update_time(const Settings& settings, BMessenger* messenger,
	thread_id* thread)
{
	BList* params = new BList(2);
	params->AddItem((void*)&settings);
	params->AddItem((void*)messenger);
	*thread = spawn_thread(update_thread, "ntpUpdate", 64, params);

	return resume_thread(*thread);
}


status_t
update_time(const Settings& settings, const char** errorString,
	int32* errorCode)
{
	int32 defaultServer = settings.GetDefaultServer();

	status_t status = B_ENTRY_NOT_FOUND;
	const char* server = settings.GetServer(defaultServer);

	if (server != NULL)
		status = ntp_update_time(server, errorString, errorCode);

	if (status != B_OK && settings.GetTryAllServers()) {
		for (int32 index = 0; ; index++) {
			if (index == defaultServer)
				index++;

			server = settings.GetServer(index);
			if (server == NULL)
				break;

			status = ntp_update_time(server, errorString, errorCode);
			if (status == B_OK)
				break;
		}
	}

	return status;
}
