/*
 * Copyright 2001-2003 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2004-2011, Haiku Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


//!	The mail daemon's settings


#include <MailSettings.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <String.h>
#include <Window.h>

#include <MailPrivate.h>


//	#pragma mark - BMailSettings


BMailSettings::BMailSettings()
{
	Reload();
}


BMailSettings::~BMailSettings()
{
}


status_t
BMailSettings::InitCheck() const
{
	return B_OK;
}


status_t
BMailSettings::Save(bigtime_t /*timeout*/)
{
	status_t ret;
	//
	// Find chain-saving directory
	//

	BPath path;
	ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (ret != B_OK) {
		fprintf(stderr, "Couldn't find user settings directory: %s\n",
			strerror(ret));
		return ret;
	}

	path.Append("Mail");

	status_t result = BPrivate::WriteMessageFile(fData, path,
		"new_mail_daemon");
	if (result < B_OK)
		return result;

	BMessenger("application/x-vnd.Be-POST").SendMessage('mrrs');

	return B_OK;
}


status_t
BMailSettings::Reload()
{
	status_t ret;

	BPath path;
	ret = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (ret != B_OK) {
		fprintf(stderr, "Couldn't find user settings directory: %s\n",
			strerror(ret));
		return ret;
	}

	path.Append("Mail/new_mail_daemon");

	// open
	BFile settings(path.Path(),B_READ_ONLY);
	ret = settings.InitCheck();
	if (ret != B_OK) {
		fprintf(stderr, "Couldn't open settings file '%s': %s\n",
			path.Path(), strerror(ret));
		return ret;
	}

	// read settings
	BMessage tmp;
	ret = tmp.Unflatten(&settings);
	if (ret != B_OK) {
		fprintf(stderr, "Couldn't read settings from '%s': %s\n",
			path.Path(), strerror(ret));
		return ret;
	}

	// clobber old settings
	fData = tmp;
	return B_OK;
}


//	# pragma mark - Global settings


int32
BMailSettings::WindowFollowsCorner()
{
	return fData.FindInt32("WindowFollowsCorner");
}


void
BMailSettings::SetWindowFollowsCorner(int32 which_corner)
{
	if (fData.ReplaceInt32("WindowFollowsCorner",which_corner))
		fData.AddInt32("WindowFollowsCorner",which_corner);
}


uint32
BMailSettings::ShowStatusWindow()
{
	int32 showStatusWindow;
	if (fData.FindInt32("ShowStatusWindow", &showStatusWindow) != B_OK) {
		// show during send and receive
		return 2;
	}

	return showStatusWindow;
}


void
BMailSettings::SetShowStatusWindow(uint32 mode)
{
	if (fData.ReplaceInt32("ShowStatusWindow",mode))
		fData.AddInt32("ShowStatusWindow",mode);
}


bool
BMailSettings::DaemonAutoStarts()
{
	return fData.FindBool("DaemonAutoStarts");
}


void
BMailSettings::SetDaemonAutoStarts(bool does_it)
{
	if (fData.ReplaceBool("DaemonAutoStarts",does_it))
		fData.AddBool("DaemonAutoStarts",does_it);
}


BRect
BMailSettings::ConfigWindowFrame()
{
	return fData.FindRect("ConfigWindowFrame");
}


void
BMailSettings::SetConfigWindowFrame(BRect frame)
{
	if (fData.ReplaceRect("ConfigWindowFrame",frame))
		fData.AddRect("ConfigWindowFrame",frame);
}


BRect
BMailSettings::StatusWindowFrame()
{
	BRect frame;
	if (fData.FindRect("StatusWindowFrame", &frame) != B_OK)
		return BRect(100, 100, 200, 120);

	return frame;
}


void
BMailSettings::SetStatusWindowFrame(BRect frame)
{
	if (fData.ReplaceRect("StatusWindowFrame",frame))
		fData.AddRect("StatusWindowFrame",frame);
}


int32
BMailSettings::StatusWindowWorkspaces()
{
	uint32 workspaces;
	if (fData.FindInt32("StatusWindowWorkSpace", (int32*)&workspaces) != B_OK)
		return B_ALL_WORKSPACES;

	return workspaces;
}


void
BMailSettings::SetStatusWindowWorkspaces(int32 workspace)
{
	if (fData.ReplaceInt32("StatusWindowWorkSpace",workspace))
		fData.AddInt32("StatusWindowWorkSpace",workspace);

	BMessage msg('wsch');
	msg.AddInt32("StatusWindowWorkSpace",workspace);
	BMessenger("application/x-vnd.Be-POST").SendMessage(&msg);
}


int32
BMailSettings::StatusWindowLook()
{
	return fData.FindInt32("StatusWindowLook");
}


void
BMailSettings::SetStatusWindowLook(int32 look)
{
	if (fData.ReplaceInt32("StatusWindowLook",look))
		fData.AddInt32("StatusWindowLook",look);

	BMessage msg('lkch');
	msg.AddInt32("StatusWindowLook",look);
	BMessenger("application/x-vnd.Be-POST").SendMessage(&msg);
}


bigtime_t
BMailSettings::AutoCheckInterval()
{
	bigtime_t value;
	if (fData.FindInt64("AutoCheckInterval", &value) != B_OK) {
		// every 5 min
		return 5 * 60 * 1000 * 1000;
	}
	return value;
}


void
BMailSettings::SetAutoCheckInterval(bigtime_t interval)
{
	if (fData.ReplaceInt64("AutoCheckInterval",interval))
		fData.AddInt64("AutoCheckInterval",interval);
}


bool
BMailSettings::CheckOnlyIfPPPUp()
{
	return fData.FindBool("CheckOnlyIfPPPUp");
}


void
BMailSettings::SetCheckOnlyIfPPPUp(bool yes)
{
	if (fData.ReplaceBool("CheckOnlyIfPPPUp",yes))
		fData.AddBool("CheckOnlyIfPPPUp",yes);
}


bool
BMailSettings::SendOnlyIfPPPUp()
{
	return fData.FindBool("SendOnlyIfPPPUp");
}


void
BMailSettings::SetSendOnlyIfPPPUp(bool yes)
{
	if (fData.ReplaceBool("SendOnlyIfPPPUp",yes))
		fData.AddBool("SendOnlyIfPPPUp",yes);
}


int32
BMailSettings::DefaultOutboundAccount()
{
	return fData.FindInt32("DefaultOutboundAccount");
}


void
BMailSettings::SetDefaultOutboundAccount(int32 to)
{
	if (fData.ReplaceInt32("DefaultOutboundAccount",to))
		fData.AddInt32("DefaultOutboundAccount",to);
}


BMailAccounts::BMailAccounts()
{
	BPath path;
	status_t status = AccountsPath(path);
	if (status != B_OK)
		return;

	BDirectory dir(path.Path());
	if (dir.InitCheck() != B_OK)
		return;

	std::vector<time_t> creationTimeList;
	BEntry entry;
	while (dir.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
		BNode node(&entry);
		time_t creationTime;
		if (node.GetCreationTime(&creationTime) != B_OK)
			continue;

		BMailAccountSettings* account = new BMailAccountSettings(entry);
		if (account->InitCheck() != B_OK) {
			delete account;
			continue;
		}

		// sort by creation time
		int insertIndex = -1;
		for (unsigned int i = 0; i < creationTimeList.size(); i++) {
			if (creationTimeList[i] > creationTime) {
				insertIndex = i;
				break;
			}
		}
		if (insertIndex < 0) {
			fAccounts.AddItem(account);
			creationTimeList.push_back(creationTime);
		} else {
			fAccounts.AddItem(account, insertIndex);
			creationTimeList.insert(creationTimeList.begin() + insertIndex,
				creationTime);
		}
	}
}


status_t
BMailAccounts::AccountsPath(BPath& path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;
	return path.Append("Mail/accounts");
}


BMailAccounts::~BMailAccounts()
{
	for (int i = 0; i < fAccounts.CountItems(); i++)
		delete fAccounts.ItemAt(i);
}


int32
BMailAccounts::CountAccounts()
{
	return fAccounts.CountItems();
}


BMailAccountSettings*
BMailAccounts::AccountAt(int32 index)
{
	return fAccounts.ItemAt(index);
}


BMailAccountSettings*
BMailAccounts::AccountByID(int32 id)
{
	for (int i = 0; i < fAccounts.CountItems(); i++) {
		BMailAccountSettings* account = fAccounts.ItemAt(i);
		if (account->AccountID() == id)
			return account;
	}
	return NULL;
}


BMailAccountSettings*
BMailAccounts::AccountByName(const char* name)
{
	for (int i = 0; i < fAccounts.CountItems(); i++) {
		BMailAccountSettings* account = fAccounts.ItemAt(i);
		if (strcmp(account->Name(), name) == 0)
			return account;
	}
	return NULL;
}


using std::vector;


AddonSettings::AddonSettings()
	:
	fModified(false)
{

}


bool
AddonSettings::Load(const BMessage& message)
{
	const char* addonPath = NULL;
	if (message.FindString("add-on path", &addonPath) != B_OK)
		return false;
	if (get_ref_for_path(addonPath, &fAddonRef) != B_OK)
		return false;
	if (message.FindMessage("settings", &fSettings) != B_OK)
		return false;
	fModified = false;
	return true;
}


bool
AddonSettings::Save(BMessage& message)
{
	BPath path(&fAddonRef);
	message.AddString("add-on path", path.Path());
	message.AddMessage("settings", &fSettings);
	fModified = false;
	return true;
}


void
AddonSettings::SetAddonRef(const entry_ref& ref)
{
	fAddonRef = ref;
}


const entry_ref&
AddonSettings::AddonRef() const
{
	return fAddonRef;
}


const BMessage&
AddonSettings::Settings() const
{
	return fSettings;
}


BMessage&
AddonSettings::EditSettings()
{
	fModified = true;
	return fSettings;
}


bool
AddonSettings::HasBeenModified()
{
	return fModified;
}


bool
MailAddonSettings::Load(const BMessage& message)
{
	if (!AddonSettings::Load(message))
		return false;

	type_code typeFound;
	int32 countFound;
	message.GetInfo("filters", &typeFound, &countFound);
	if (typeFound != B_MESSAGE_TYPE)
		return false;

	for (int i = 0; i < countFound; i++) {
		int32 index = AddFilterSettings();
		AddonSettings& filterSettings = fFiltersSettings[index];
		BMessage filterMessage;
		message.FindMessage("filters", i, &filterMessage);
		if (!filterSettings.Load(filterMessage))
			RemoveFilterSettings(index);
	}
	return true;
}


bool
MailAddonSettings::Save(BMessage& message)
{
	if (!AddonSettings::Save(message))
		return false;

	for (int i = 0; i < CountFilterSettings(); i++) {
		BMessage filter;
		AddonSettings& filterSettings = fFiltersSettings[i];
		filterSettings.Save(filter);
		message.AddMessage("filters", &filter);
	}
	return true;
}


int32
MailAddonSettings::CountFilterSettings()
{
	return fFiltersSettings.size();
}


int32
MailAddonSettings::AddFilterSettings(const entry_ref* ref)
{
	AddonSettings filterSettings;
	if (ref != NULL)
		filterSettings.SetAddonRef(*ref);
	fFiltersSettings.push_back(filterSettings);
	return fFiltersSettings.size() - 1;
}


bool
MailAddonSettings::RemoveFilterSettings(int32 index)
{
	fFiltersSettings.erase(fFiltersSettings.begin() + index);
	return true;
}


bool
MailAddonSettings::MoveFilterSettings(int32 from, int32 to)
{
	if (from < 0 || from >= (int32)fFiltersSettings.size() || to < 0
		|| to >= (int32)fFiltersSettings.size())
		return false;
	AddonSettings fromSettings = fFiltersSettings[from];
	fFiltersSettings.erase(fFiltersSettings.begin() + from);
	if (to == (int32)fFiltersSettings.size())
		fFiltersSettings.push_back(fromSettings);
	else {
		std::vector<AddonSettings>::iterator it = fFiltersSettings.begin() + to;
		fFiltersSettings.insert(it, fromSettings);
	}
	return true;
}


AddonSettings*
MailAddonSettings::FilterSettingsAt(int32 index)
{
	if (index < 0 || index >= (int32)fFiltersSettings.size())
		return NULL;
	return &fFiltersSettings[index];
}


bool
MailAddonSettings::HasBeenModified()
{
	if (AddonSettings::HasBeenModified())
		return true;
	for (unsigned int i = 0; i < fFiltersSettings.size(); i++) {
		if (fFiltersSettings[i].HasBeenModified())
			return true;
	}
	return false;
}


//	#pragma mark -


BMailAccountSettings::BMailAccountSettings()
	:
	fStatus(B_OK),
	fInboundEnabled(true),
	fOutboundEnabled(true),
	fModified(true)
{
	fAccountID = real_time_clock();
}


BMailAccountSettings::BMailAccountSettings(BEntry account)
	:
	fAccountFile(account),
	fModified(false)
{
	fStatus = Reload();
}


BMailAccountSettings::~BMailAccountSettings()
{

}


void
BMailAccountSettings::SetAccountID(int32 id)
{
	fModified = true;
	fAccountID = id;
}


int32
BMailAccountSettings::AccountID()
{
	return fAccountID;
}


void
BMailAccountSettings::SetName(const char* name)
{
	fModified = true;
	fAccountName = name;
}


const char*
BMailAccountSettings::Name() const
{
	return fAccountName;
}


void
BMailAccountSettings::SetRealName(const char* realName)
{
	fModified = true;
	fRealName = realName;
}


const char*
BMailAccountSettings::RealName() const
{
	return fRealName;
}


void
BMailAccountSettings::SetReturnAddress(const char* returnAddress)
{
	fModified = true;
	fReturnAdress = returnAddress;
}


const char*
BMailAccountSettings::ReturnAddress() const
{
	return fReturnAdress;
}


bool
BMailAccountSettings::SetInboundAddon(const char* name)
{
	BPath path;
	status_t status = find_directory(B_BEOS_ADDONS_DIRECTORY, &path);
	if (status != B_OK)
		return false;
	path.Append("mail_daemon");
	path.Append("inbound_protocols");
	path.Append(name);
	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);
	fInboundSettings.SetAddonRef(ref);

	fModified = true;
	return true;
}


bool
BMailAccountSettings::SetOutboundAddon(const char* name)
{
	BPath path;
	status_t status = find_directory(B_BEOS_ADDONS_DIRECTORY, &path);
	if (status != B_OK)
		return false;
	path.Append("mail_daemon");
	path.Append("outbound_protocols");
	path.Append(name);
	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);
	fOutboundSettings.SetAddonRef(ref);

	fModified = true;
	return true;
}


const entry_ref&
BMailAccountSettings::InboundPath() const
{
	return fInboundSettings.AddonRef();
}


const entry_ref&
BMailAccountSettings::OutboundPath() const
{
	return fOutboundSettings.AddonRef();
}


MailAddonSettings&
BMailAccountSettings::InboundSettings()
{
	return fInboundSettings;
}


MailAddonSettings&
BMailAccountSettings::OutboundSettings()
{
	return fOutboundSettings;
}


bool
BMailAccountSettings::HasInbound()
{
	return BEntry(&fInboundSettings.AddonRef()).Exists();
}


bool
BMailAccountSettings::HasOutbound()
{
	return BEntry(&fOutboundSettings.AddonRef()).Exists();
}


void
BMailAccountSettings::SetInboundEnabled(bool enabled)
{
	fInboundEnabled = enabled;
	fModified = true;
}


bool
BMailAccountSettings::IsInboundEnabled() const
{
	return fInboundEnabled;
}


void
BMailAccountSettings::SetOutboundEnabled(bool enabled)
{
	fOutboundEnabled = enabled;
	fModified = true;
}


bool
BMailAccountSettings::IsOutboundEnabled() const
{
	return fOutboundEnabled;
}


status_t
BMailAccountSettings::Reload()
{
	BFile file(&fAccountFile, B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;
	BMessage settings;
	settings.Unflatten(&file);

	int32 id;
	if (settings.FindInt32("id", &id) == B_OK)
		fAccountID = id;
	settings.FindString("name", &fAccountName);
	settings.FindString("real_name", &fRealName);
	settings.FindString("return_address", &fReturnAdress);

	BMessage inboundSettings;
	settings.FindMessage("inbound", &inboundSettings);
	fInboundSettings.Load(inboundSettings);
	BMessage outboundSettings;
	settings.FindMessage("outbound", &outboundSettings);
	fOutboundSettings.Load(outboundSettings);

	if (settings.FindBool("inbound_enabled", &fInboundEnabled) != B_OK)
		fInboundEnabled = true;
	if (settings.FindBool("outbound_enabled", &fOutboundEnabled) != B_OK)
		fOutboundEnabled = true;

	fModified = false;
	return B_OK;
}


status_t
BMailAccountSettings::Save()
{
	fModified = false;

	BMessage settings;
	settings.AddInt32("id", fAccountID);
	settings.AddString("name", fAccountName);
	settings.AddString("real_name", fRealName);
	settings.AddString("return_address", fReturnAdress);

	BMessage inboundSettings;
	fInboundSettings.Save(inboundSettings);
	settings.AddMessage("inbound", &inboundSettings);
	BMessage outboundSettings;
	fOutboundSettings.Save(outboundSettings);
	settings.AddMessage("outbound", &outboundSettings);

	settings.AddBool("inbound_enabled", fInboundEnabled);
	settings.AddBool("outbound_enabled", fOutboundEnabled);

	status_t status = _CreateAccountFilePath();
	if (status != B_OK)
		return status;

	BFile file(&fAccountFile, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	status = file.InitCheck();
	if (status != B_OK)
		return status;
	return settings.Flatten(&file);
}


status_t
BMailAccountSettings::Delete()
{
	return fAccountFile.Remove();
}


bool
BMailAccountSettings::HasBeenModified()
{
	if (fInboundSettings.HasBeenModified())
		return true;
	if (fOutboundSettings.HasBeenModified())
		return true;
	return fModified;
}


const BEntry&
BMailAccountSettings::AccountFile()
{
	return fAccountFile;
}


status_t
BMailAccountSettings::_CreateAccountFilePath()
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;
	path.Append("Mail/accounts");
	create_directory(path.Path(), 777);

	if (fAccountFile.InitCheck() == B_OK)
		return B_OK;

	BString fileName = fAccountName;
	if (fileName == "")
		fileName << fAccountID;
	for (int i = 0; ; i++) {
		BString testFileName = fileName;
		if (i != 0) {
			testFileName += "_";
			testFileName << i;
		}
		BPath testPath(path);
		testPath.Append(testFileName);
		BEntry testEntry(testPath.Path());
		if (!testEntry.Exists()) {
			fileName = testFileName;
			break;
		}
	}

	path.Append(fileName);
	return fAccountFile.SetTo(path.Path());
}
