/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 *		Stephan Aßmus, superstippi@gmx.de
 *		Brian Hill, supernova@tycho.email
 */


#include <Notification.h>

#include <new>

#include <stdlib.h>
#include <string.h>

#include <notification/Notifications.h>

#include <Bitmap.h>
#include <Message.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>


BNotification::BNotification(notification_type type)
	:
	BArchivable(),
	fInitStatus(B_OK),
	fType(type),
	fProgress(0.f),
	fFile(NULL),
	fBitmap(NULL)
{
	team_info teamInfo;
	get_team_info(B_CURRENT_TEAM, &teamInfo);
	app_info appInfo;
	be_roster->GetRunningAppInfo(teamInfo.team, &appInfo);

	int32 iconSize = B_LARGE_ICON;
	fBitmap = new BBitmap(BRect(0, 0, iconSize - 1, iconSize - 1), 0, B_RGBA32);
	if (fBitmap) {
		if (BNodeInfo::GetTrackerIcon(&appInfo.ref, fBitmap,
			icon_size(iconSize)) != B_OK) {
			delete fBitmap;
			fBitmap = NULL;
		}
	}
	fSourceSignature = appInfo.signature;
	BPath path(&appInfo.ref);
	if (path.InitCheck() == B_OK)
		fSourceName = path.Leaf();
}


BNotification::BNotification(BMessage* archive)
	:
	BArchivable(archive),
	fInitStatus(B_OK),
	fProgress(0.0f),
	fFile(NULL),
	fBitmap(NULL)
{
	BString appName;
	if (archive->FindString("_appname", &appName) == B_OK)
		fSourceName = appName;

	BString signature;
	if (archive->FindString("_signature", &signature) == B_OK)
		fSourceSignature = signature;

	int32 type;
	if (archive->FindInt32("_type", &type) == B_OK)
		fType = (notification_type)type;
	else
		fInitStatus = B_ERROR;

	BString group;
	if (archive->FindString("_group", &group) == B_OK)
		SetGroup(group);

	BString title;
	if (archive->FindString("_title", &title) == B_OK)
		SetTitle(title);

	BString content;
	if (archive->FindString("_content", &content) == B_OK)
		SetContent(content);

	BString messageID;
	if (archive->FindString("_messageID", &messageID) == B_OK)
		SetMessageID(messageID);

	float progress;
	if (type == B_PROGRESS_NOTIFICATION
		&& archive->FindFloat("_progress", &progress) == B_OK)
		SetProgress(progress);

	BString onClickApp;
	if (archive->FindString("_onClickApp", &onClickApp) == B_OK)
		SetOnClickApp(onClickApp);

	entry_ref onClickFile;
	if (archive->FindRef("_onClickFile", &onClickFile) == B_OK)
		SetOnClickFile(&onClickFile);

	entry_ref onClickRef;
	int32 index = 0;
	while (archive->FindRef("_onClickRef", index++, &onClickRef) == B_OK)
		AddOnClickRef(&onClickRef);

	BString onClickArgv;
	index = 0;
	while (archive->FindString("_onClickArgv", index++, &onClickArgv) == B_OK)
		AddOnClickArg(onClickArgv);

	status_t ret = B_OK;
	BMessage icon;
	if ((ret = archive->FindMessage("_icon", &icon)) == B_OK) {
		BBitmap bitmap(&icon);
		ret = bitmap.InitCheck();
		if (ret == B_OK)
			ret = SetIcon(&bitmap);
	}
}


BNotification::~BNotification()
{
	delete fFile;
	delete fBitmap;

	for (int32 i = fRefs.CountItems() - 1; i >= 0; i--)
		delete (entry_ref*)fRefs.ItemAtFast(i);

	for (int32 i = fArgv.CountItems() - 1; i >= 0; i--)
		free(fArgv.ItemAtFast(i));
}


status_t
BNotification::InitCheck() const
{
	return fInitStatus;
}


BArchivable*
BNotification::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BNotification"))
		return new(std::nothrow) BNotification(archive);

	return NULL;
}


status_t
BNotification::Archive(BMessage* archive, bool deep) const
{
	status_t status = BArchivable::Archive(archive, deep);

	if (status == B_OK)
		status = archive->AddString("_appname", fSourceName);

	if (status == B_OK)
		status = archive->AddString("_signature", fSourceSignature);

	if (status == B_OK)
		status = archive->AddInt32("_type", (int32)fType);

	if (status == B_OK && Group() != NULL)
		status = archive->AddString("_group", Group());

	if (status == B_OK && Title() != NULL)
		status = archive->AddString("_title", Title());

	if (status == B_OK && Content() != NULL)
		status = archive->AddString("_content", Content());

	if (status == B_OK && MessageID() != NULL)
		status = archive->AddString("_messageID", MessageID());

	if (status == B_OK && Type() == B_PROGRESS_NOTIFICATION)
		status = archive->AddFloat("_progress", Progress());

	if (status == B_OK && OnClickApp() != NULL)
		status = archive->AddString("_onClickApp", OnClickApp());

	if (status == B_OK && OnClickFile() != NULL)
		status = archive->AddRef("_onClickFile", OnClickFile());

	if (status == B_OK) {
		for (int32 i = 0; i < CountOnClickRefs(); i++) {
			status = archive->AddRef("_onClickRef", OnClickRefAt(i));
			if (status != B_OK)
				break;
		}
	}

	if (status == B_OK) {
		for (int32 i = 0; i < CountOnClickArgs(); i++) {
			status = archive->AddString("_onClickArgv", OnClickArgAt(i));
			if (status != B_OK)
				break;
		}
	}

	if (status == B_OK) {
		const BBitmap* icon = Icon();
		if (icon != NULL) {
			BMessage iconArchive;
			status = icon->Archive(&iconArchive);
			if (status == B_OK)
				archive->AddMessage("_icon", &iconArchive);
		}
	}

	return status;
}


const char*
BNotification::SourceSignature() const
{
	return fSourceSignature;
}


const char*
BNotification::SourceName() const
{
	return fSourceName;
}


notification_type
BNotification::Type() const
{
	return fType;
}


const char*
BNotification::Group() const
{
	if (fGroup == "")
		return NULL;
	return fGroup;
}


void
BNotification::SetGroup(const BString& group)
{
	fGroup = group;
}


const char*
BNotification::Title() const
{
	if (fTitle == "")
		return NULL;
	return fTitle;
}


void
BNotification::SetTitle(const BString& title)
{
	fTitle = title;
}


const char*
BNotification::Content() const
{
	if (fContent == "")
		return NULL;
	return fContent;
}


void
BNotification::SetContent(const BString& content)
{
	fContent = content;
}


const char*
BNotification::MessageID() const
{
	if (fID == "")
		return NULL;
	return fID;
}


void
BNotification::SetMessageID(const BString& id)
{
	fID = id;
}


float
BNotification::Progress() const
{
	if (fType != B_PROGRESS_NOTIFICATION)
		return -1;
	return fProgress;
}


void
BNotification::SetProgress(float progress)
{
	if (progress < 0)
		fProgress = 0;
	else if (progress > 1)
		fProgress = 1;
	else
		fProgress = progress;
}


const char*
BNotification::OnClickApp() const
{
	if (fApp == "")
		return NULL;
	return fApp;
}


void
BNotification::SetOnClickApp(const BString& app)
{
	fApp = app;
}


const entry_ref*
BNotification::OnClickFile() const
{
	return fFile;
}


status_t
BNotification::SetOnClickFile(const entry_ref* file)
{
	delete fFile;

	if (file != NULL) {
		fFile = new(std::nothrow) entry_ref(*file);
		if (fFile == NULL)
			return B_NO_MEMORY;
	} else
		fFile = NULL;

	return B_OK;
}


status_t
BNotification::AddOnClickRef(const entry_ref* ref)
{
	if (ref == NULL)
		return B_BAD_VALUE;

	entry_ref* clonedRef = new(std::nothrow) entry_ref(*ref);
	if (clonedRef == NULL || !fRefs.AddItem(clonedRef))
		return B_NO_MEMORY;

	return B_OK;
}


int32
BNotification::CountOnClickRefs() const
{
	return fRefs.CountItems();
}


const entry_ref*
BNotification::OnClickRefAt(int32 index) const
{
	return (entry_ref*)fRefs.ItemAt(index);
}


status_t
BNotification::AddOnClickArg(const BString& arg)
{
	char* clonedArg = strdup(arg.String());
	if (clonedArg == NULL || !fArgv.AddItem(clonedArg))
		return B_NO_MEMORY;

	return B_OK;
}


int32
BNotification::CountOnClickArgs() const
{
	return fArgv.CountItems();
}


const char*
BNotification::OnClickArgAt(int32 index) const
{
	return (char*)fArgv.ItemAt(index);
}


const BBitmap*
BNotification::Icon() const
{
	return fBitmap;
}


status_t
BNotification::SetIcon(const BBitmap* icon)
{
	delete fBitmap;

	if (icon != NULL) {
		fBitmap = new(std::nothrow) BBitmap(icon);
		if (fBitmap == NULL)
			return B_NO_MEMORY;
		return fBitmap->InitCheck();
	}

	fBitmap = NULL;
	return B_OK;
}


status_t
BNotification::Send(bigtime_t timeout)
{
	BMessage msg(kNotificationMessage);

	// Archive notification
	status_t ret = Archive(&msg);

	// Custom time out
	if (ret == B_OK && timeout > 0)
		ret = msg.AddInt64("timeout", timeout);

	// Send message
	if (ret == B_OK) {
		BMessenger server(kNotificationServerSignature);
		ret = server.SendMessage(&msg);
	}

	return ret;
}


void BNotification::_ReservedNotification1() {}
void BNotification::_ReservedNotification2() {}
void BNotification::_ReservedNotification3() {}
void BNotification::_ReservedNotification4() {}
void BNotification::_ReservedNotification5() {}
void BNotification::_ReservedNotification6() {}
void BNotification::_ReservedNotification7() {}
void BNotification::_ReservedNotification8() {}
