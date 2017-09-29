/*
 * Copyright 2010-2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NOTIFICATION_H
#define _NOTIFICATION_H


#include <Archivable.h>
#include <Entry.h>
#include <List.h>
#include <String.h>


// notification types
enum notification_type {
	B_INFORMATION_NOTIFICATION,
	B_IMPORTANT_NOTIFICATION,
	B_ERROR_NOTIFICATION,
	B_PROGRESS_NOTIFICATION
};

class BBitmap;


class BNotification : public BArchivable {
public:
								BNotification(notification_type type);
								BNotification(BMessage* archive);
	virtual						~BNotification();

			status_t			InitCheck() const;

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive, bool deep = true) const;

			const char*			SourceSignature() const;
			const char*			SourceName() const;

			notification_type	Type() const;

			const char*			Group() const;
			void				SetGroup(const BString& group);

			const char*			Title() const;
			void				SetTitle(const BString& title);

			const char*			Content() const;
			void				SetContent(const BString& content);

			const char*			MessageID() const;
			void				SetMessageID(const BString& id);

			float				Progress() const;
			void				SetProgress(float progress);

			const char*			OnClickApp() const;
			void				SetOnClickApp(const BString& app);

			const entry_ref*	OnClickFile() const;
			status_t			SetOnClickFile(const entry_ref* file);

			status_t			AddOnClickRef(const entry_ref* ref);
			int32				CountOnClickRefs() const;
			const entry_ref*	OnClickRefAt(int32 index) const;

			status_t			AddOnClickArg(const BString& arg);
			int32				CountOnClickArgs() const;
			const char*			OnClickArgAt(int32 index) const;

			const BBitmap*		Icon() const;
			status_t			SetIcon(const BBitmap* icon);

			status_t			Send(bigtime_t timeout = -1);

private:
	virtual	void _ReservedNotification1();
	virtual	void _ReservedNotification2();
	virtual	void _ReservedNotification3();
	virtual	void _ReservedNotification4();
	virtual	void _ReservedNotification5();
	virtual	void _ReservedNotification6();
	virtual	void _ReservedNotification7();
	virtual	void _ReservedNotification8();

			status_t			fInitStatus;

			BString				fSourceSignature;
			BString				fSourceName;
			notification_type	fType;
			BString				fGroup;
			BString				fTitle;
			BString				fContent;
			BString				fID;
			float				fProgress;

			BString				fApp;
			entry_ref*			fFile;
			BList				fRefs;
			BList				fArgv;
			BBitmap*			fBitmap;

			uint32				_reserved[8];
};


#endif	// _NOTIFICATION_H
