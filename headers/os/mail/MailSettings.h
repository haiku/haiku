/*
 * Copyright 2004-2012, Haiku Inc. All rights reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011 Clemens Zeidler.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIL_SETTINGS_H
#define MAIL_SETTINGS_H


#include <vector>

#include <Archivable.h>
#include <Entry.h>
#include <List.h>
#include <Message.h>
#include <ObjectList.h>
#include <String.h>


class BPath;


typedef enum {
	B_MAIL_SHOW_STATUS_WINDOW_NEVER         = 0,
	B_MAIL_SHOW_STATUS_WINDOW_WHEN_SENDING	= 1,
	B_MAIL_SHOW_STATUS_WINDOW_WHEN_ACTIVE	= 2,
	B_MAIL_SHOW_STATUS_WINDOW_ALWAYS        = 3
} b_mail_status_window_option;


class BMailSettings {
public:
								BMailSettings();
								~BMailSettings();

			status_t			Save();
			status_t			Reload();
			status_t			InitCheck() const;

			// Global settings
			int32				WindowFollowsCorner();
			void				SetWindowFollowsCorner(int32 which_corner);

			uint32				ShowStatusWindow();
			void				SetShowStatusWindow(uint32 mode);

			bool				DaemonAutoStarts();
			void				SetDaemonAutoStarts(bool does_it);

			void				SetConfigWindowFrame(BRect frame);
			BRect				ConfigWindowFrame();

			void				SetStatusWindowFrame(BRect frame);
			BRect				StatusWindowFrame();

			int32				StatusWindowWorkspaces();
			void				SetStatusWindowWorkspaces(int32 workspaces);

			int32				StatusWindowLook();
			void				SetStatusWindowLook(int32 look);

			bigtime_t			AutoCheckInterval();
			void				SetAutoCheckInterval(bigtime_t);

			bool				CheckOnlyIfPPPUp();
			void				SetCheckOnlyIfPPPUp(bool yes);

			bool				SendOnlyIfPPPUp();
			void				SetSendOnlyIfPPPUp(bool yes);

			int32				DefaultOutboundAccount();
			void				SetDefaultOutboundAccount(int32 to);

private:
			BMessage			fData;
			uint32				_reserved[4];
};


class BMailAddOnSettings : public BMessage {
public:
								BMailAddOnSettings();
	virtual						~BMailAddOnSettings();

	virtual status_t			Load(const BMessage& message);
	virtual	status_t			Save(BMessage& message);

			void				SetAddOnRef(const entry_ref& ref);
			const entry_ref&	AddOnRef() const;

	virtual	bool				HasBeenModified() const;

private:
			const char*			_RelativizePath(const BPath& path) const;

private:
			BMessage			fOriginalSettings;
			entry_ref			fRef;
			entry_ref			fOriginalRef;
};


class BMailProtocolSettings : public BMailAddOnSettings {
public:
								BMailProtocolSettings();
	virtual						~BMailProtocolSettings();

	virtual	status_t			Load(const BMessage& message);
	virtual	status_t			Save(BMessage& message);

			int32				CountFilterSettings() const;
			int32				AddFilterSettings(const entry_ref* ref = NULL);
			void				RemoveFilterSettings(int32 index);
			bool				MoveFilterSettings(int32 from, int32 to);
			BMailAddOnSettings*	FilterSettingsAt(int32 index) const;

	virtual	bool				HasBeenModified() const;

private:
			BObjectList<BMailAddOnSettings> fFiltersSettings;
};


class BMailAccountSettings {
public:
								BMailAccountSettings();
								BMailAccountSettings(BEntry account);
								~BMailAccountSettings();

			status_t			InitCheck() { return fStatus; }

			void				SetAccountID(int32 id);
			int32				AccountID() const;

			void				SetName(const char* name);
			const char*			Name() const;

			void				SetRealName(const char* realName);
			const char*			RealName() const;

			void				SetReturnAddress(const char* returnAddress);
			const char*			ReturnAddress() const;

			bool				SetInboundAddOn(const char* name);
			bool				SetOutboundAddOn(const char* name);
			const entry_ref&	InboundAddOnRef() const;
			const entry_ref&	OutboundAddOnRef() const;

			BMailProtocolSettings& InboundSettings();
			const BMailProtocolSettings& InboundSettings() const;
			BMailProtocolSettings& OutboundSettings();
			const BMailProtocolSettings& OutboundSettings() const;

			bool				HasInbound();
			bool				HasOutbound();

			void				SetInboundEnabled(bool enabled = true);
			bool				IsInboundEnabled() const;
			void				SetOutboundEnabled(bool enabled = true);
			bool				IsOutboundEnabled() const;

			status_t			Reload();
			status_t			Save();
			status_t			Delete();

			bool				HasBeenModified() const;

			const BEntry&		AccountFile() const;

private:
			status_t			_CreateAccountFilePath();
			status_t			_GetAddOnRef(const char* subPath,
									const char* name, entry_ref& ref);

private:
			status_t			fStatus;
			BEntry				fAccountFile;

			int32				fAccountID;

			BString				fAccountName;
			BString				fRealName;
			BString				fReturnAdress;

			BMailProtocolSettings fInboundSettings;
			BMailProtocolSettings fOutboundSettings;

			bool				fInboundEnabled;
			bool				fOutboundEnabled;

			bool				fModified;
};


class BMailAccounts {
public:
								BMailAccounts();
								~BMailAccounts();

	static	status_t			AccountsPath(BPath& path);

			int32				CountAccounts();
			BMailAccountSettings*	AccountAt(int32 index);

			BMailAccountSettings*	AccountByID(int32 id);
			BMailAccountSettings*	AccountByName(const char* name);
private:
			BObjectList<BMailAccountSettings>	fAccounts;
};


#endif	/* ZOIDBERG_MAIL_SETTINGS_H */
