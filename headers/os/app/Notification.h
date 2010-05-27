/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NOTIFICATION_H
#define _NOTIFICATION_H

#include <Entry.h>

// notification types
enum notification_type {
	B_INFORMATION_NOTIFICATION,
	B_IMPORTANT_NOTIFICATION,
	B_ERROR_NOTIFICATION,
	B_PROGRESS_NOTIFICATION
};

class BBitmap;
class BList;

class BNotification {
public:
						BNotification(notification_type type);
						~BNotification();

	notification_type	Type() const;

	const char*			Application() const;
	void				SetApplication(const char* app);

	const char*			Title() const;
	void				SetTitle(const char* title);

	const char*			Content() const;
	void				SetContent(const char* content);

	const char*			MessageID() const;
	void				SetMessageID(const char* id);

	float				Progress() const;
	void				SetProgress(float progress);

	const char*			OnClickApp() const;
	void				SetOnClickApp(const char* app);

	entry_ref*			OnClickFile() const;
	void				SetOnClickFile(const entry_ref* file);

	BList*				OnClickRefs() const;
	void				AddOnClickRef(const entry_ref* ref);

	BList*				OnClickArgv() const;
	void				AddOnClickArg(const char* arg);

	BBitmap*			Icon() const;
	void				SetIcon(BBitmap* icon);

private:
	notification_type	fType;
	char*				fAppName;
	char*				fTitle;
	char*				fContent;
	char*				fID;
	float				fProgress;
	char*				fApp;
	entry_ref*			fFile;
	BList*				fRefs;
	BList*				fArgv;
	BBitmap*			fBitmap;
};

#endif	// _NOTIFICATION_H
