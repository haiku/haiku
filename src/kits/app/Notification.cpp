/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <List.h>
#include <Message.h>
#include <Notification.h>


BNotification::BNotification(notification_type type)
	:
	fType(type),
	fAppName(NULL),
	fTitle(NULL),
	fContent(NULL),
	fID(NULL),
	fApp(NULL),
	fFile(NULL),
	fBitmap(NULL)
{
	fRefs = new BList();
	fArgv = new BList();
}


BNotification::~BNotification()
{
	if (fAppName)
		free(fAppName);
	if (fTitle)
		free(fTitle);
	if (fContent)
		free(fContent);
	if (fID)
		free(fID);
	if (fApp)
		free(fApp);

	delete fRefs;
	delete fArgv;
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
BNotification::SetApplication(const char* app)
{
	free(fAppName);
	fAppName = NULL;

	if (app)
		fAppName = strdup(app);
}


const char*
BNotification::Title() const
{
	return fTitle;
}


void
BNotification::SetTitle(const char* title)
{
	free(fTitle);
	fTitle = NULL;

	if (title)
		fTitle = strdup(title);
}


const char*
BNotification::Content() const
{
	return fContent;
}


void
BNotification::SetContent(const char* content)
{
	free(fContent);
	fContent = NULL;

	if (content)
		fContent = strdup(content);
}


const char*
BNotification::MessageID() const
{
	return fID;
}


void
BNotification::SetMessageID(const char* id)
{
	free(fID);
	fID = NULL;

	if (id)
		fID = strdup(id);
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
BNotification::SetOnClickApp(const char* app)
{
	free(fApp);
	fApp = NULL;

	if (app)
		fApp = strdup(app);
}


entry_ref*
BNotification::OnClickFile() const
{
	return fFile;
}


void
BNotification::SetOnClickFile(const entry_ref* file)
{
	fFile = (entry_ref*)file;
}


BList*
BNotification::OnClickRefs() const
{
	return fRefs;
}


void
BNotification::AddOnClickRef(const entry_ref* ref)
{
	fRefs->AddItem((void*)ref);
}


BList*
BNotification::OnClickArgv() const
{
	return fArgv;
}


void
BNotification::AddOnClickArg(const char* arg)
{
	fArgv->AddItem((void*)arg);
}


BBitmap*
BNotification::Icon() const
{
	return fBitmap;
}


void
BNotification::SetIcon(BBitmap* icon)
{
	fBitmap = icon;
}
