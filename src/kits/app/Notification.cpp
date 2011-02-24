/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include <Notification.h>

#include <new>

#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <Message.h>


BNotification::BNotification(notification_type type)
	:
	fType(type),
	fProgress(0),
	fFile(NULL),
	fBitmap(NULL)
{
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


notification_type
BNotification::Type() const
{
	return fType;
}


const char*
BNotification::Application() const
{
	return fAppName;
}


void
BNotification::SetApplication(const BString& app)
{
	fAppName = app;
}


const char*
BNotification::Title() const
{
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
	return fProgress;
}


void
BNotification::SetProgress(float progress)
{
	fProgress = progress;
}


const char*
BNotification::OnClickApp() const
{
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
	return (entry_ref*)fArgv.ItemAt(index);
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
