/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hamish Morrison <hamish@lavabit.com>
 *		Axel Dörfler <axeld@pinc-software.de>
 */
 
#include "NetworkTimeView.h"

#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <File.h>
#include <FindDirectory.h>
#include <ListView.h>
#include <Path.h>
#include <ScrollView.h>
#include <TextControl.h>

#include "ntp.h"
#include "TimeMessages.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Time"


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
	
	if (oldSize != newSize)
		return true;

	char* oldBytes = new char[oldSize];
	fOldMessage.Flatten(oldBytes, oldSize);
	char* newBytes = new char[newSize];
	fMessage.Flatten(newBytes, newSize);
	
	int result = memcmp(oldBytes, newBytes, oldSize);	

	delete[] oldBytes;
	delete[] newBytes;

	if (result != 0)
		return true;
	else
		return false;
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
	for (int32 index = 0; fMessage.FindString(
		name, index, &string) == B_OK; index++)
		if (strcmp(string, value) == 0)
			return index;
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


NetworkTimeView::NetworkTimeView(const char* name)
	:
	BGroupView(name, B_VERTICAL, B_USE_DEFAULT_SPACING),
	fSettings(),
	fUpdateThread(-1)
{
	fSettings.Load();
	_InitView();
}


void
NetworkTimeView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSetDefaultServer:
		{
			int32 sel = fServerListView->CurrentSelection();
			if (sel < 0)
				fServerListView->Select(fSettings.GetDefaultServer());
			else {
				fSettings.SetDefaultServer(sel);
				Looper()->PostMessage(new BMessage(kMsgChange));
			}
			break;
		}
		case kMsgAddServer:
			fSettings.AddServer(fServerTextControl->Text());
			_UpdateServerList();
			Looper()->PostMessage(new BMessage(kMsgChange));
			break;

		case kMsgRemoveServer:
			fSettings.RemoveServer(((BStringItem*)
				fServerListView->ItemAt(
					fServerListView->
					CurrentSelection()))->Text());
			_UpdateServerList();
			Looper()->PostMessage(new BMessage(kMsgChange));
			break;

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
			fSettings.SetSynchronizeAtBoot(
				fSynchronizeAtBootCheckBox->Value());
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
				if (status == B_OK) return;

				const char* errorString;
				message->FindString("error string", &errorString);	
				char buffer[256];

				int32 errorCode;
				if (message->FindInt32("error code", &errorCode)
					== B_OK)
					snprintf(buffer, sizeof(buffer),
						B_TRANSLATE("The following error occured "
						"while synchronizing:\r\n%s: %s"),
						errorString, strerror(errorCode));
				else
					snprintf(buffer, sizeof(buffer),
						B_TRANSLATE("The following error occured "
						"while synchronizing:\r\n%s"),
						errorString);

				(new BAlert(B_TRANSLATE("Time"), buffer, 
					B_TRANSLATE("OK")))->Go();
			}
			break;
		}
		
		case kMsgRevert:
			fSettings.Revert();
		    fTryAllServersCheckBox->SetValue(
		    	fSettings.GetTryAllServers());
			fSynchronizeAtBootCheckBox->SetValue(
				fSettings.GetSynchronizeAtBoot());
			_UpdateServerList();
			break;
	}
}


void
NetworkTimeView::AttachedToWindow()
{
	fServerListView->SetTarget(this);
	fAddButton->SetTarget(this);
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
	fServerTextControl = new BTextControl(NULL, NULL, NULL);

	fAddButton = new BButton("add", B_TRANSLATE("Add"),
		new BMessage(kMsgAddServer));
	fRemoveButton = new BButton("remove", B_TRANSLATE("Remove"),
		new BMessage(kMsgRemoveServer));
	fResetButton = new BButton("reset", B_TRANSLATE("Reset"),
		new BMessage(kMsgResetServerList));

	fServerListView = new BListView("serverList");
	fServerListView->SetSelectionMessage(new
		BMessage(kMsgSetDefaultServer));
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
	fSynchronizeButton = new BButton("update", B_TRANSLATE("Synchronize"),
		new BMessage(kMsgSynchronize));
	fSynchronizeButton->SetExplicitAlignment(
		BAlignment(B_ALIGN_RIGHT, B_ALIGN_BOTTOM));

	const float kInset = be_control_look->DefaultItemSpacing();
	BLayoutBuilder::Group<>(this)
		.AddGroup(B_HORIZONTAL)
			.AddGroup(B_VERTICAL, 0)
				.Add(fServerTextControl)
				.Add(scrollView)
			.End()
			.AddGroup(B_VERTICAL, kInset / 2)
				.Add(fAddButton)
				.Add(fRemoveButton)
				.Add(fResetButton)
				.AddGlue()
			.End()
		.End()
		.AddGroup(B_HORIZONTAL)
			.AddGroup(B_VERTICAL, 0)
				.Add(fTryAllServersCheckBox)
				.Add(fSynchronizeAtBootCheckBox)
			.End()
			.Add(fSynchronizeButton)
		.End()
		.SetInsets(kInset, kInset, kInset, kInset);
}


void
NetworkTimeView::_UpdateServerList()
{
	while (fServerListView->RemoveItem(0L) != NULL);

	const char* server;
	int32 index = 0;

	while ((server = fSettings.GetServer(index++)) != NULL)
		fServerListView->AddItem(new BStringItem(server));

	fServerListView->Select(fSettings.GetDefaultServer());
	fServerListView->ScrollToSelection();
}


void
NetworkTimeView::_DoneSynchronizing()
{
	fUpdateThread = -1;
	fSynchronizeButton->SetLabel(B_TRANSLATE("Synchronize again"));
	fSynchronizeButton->Message()->what = kMsgSynchronize;
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

