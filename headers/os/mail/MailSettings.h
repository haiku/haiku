/*
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011 Clemens Zeidler.
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

			status_t			Save(bigtime_t timeout = B_INFINITE_TIMEOUT);
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


class AddonSettings {
public:
								AddonSettings();

			bool				Load(const BMessage& message);
			bool				Save(BMessage& message);

			void				SetAddonRef(const entry_ref& ref);
	const	entry_ref&			AddonRef() const;

	const	BMessage&			Settings() const;
			BMessage&			EditSettings();

			bool				HasBeenModified();

private:
			BMessage			fSettings;
			entry_ref			fAddonRef;

			bool				fModified;
};


class MailAddonSettings : public AddonSettings {
public:
			bool				Load(const BMessage& message);
			bool				Save(BMessage& message);

			int32				CountFilterSettings();
			int32				AddFilterSettings(const entry_ref* ref = NULL);
			bool				RemoveFilterSettings(int32 index);
			bool				MoveFilterSettings(int32 from, int32 to);
			AddonSettings*		FilterSettingsAt(int32 index);

			bool				HasBeenModified();

private:
			std::vector<AddonSettings>	fFiltersSettings;
};


class BMailAccountSettings {
	public:
								BMailAccountSettings();
								BMailAccountSettings(BEntry account);
								~BMailAccountSettings();

			status_t			InitCheck() { return fStatus; }

			void				SetAccountID(int32 id);
			int32				AccountID();

			void				SetName(const char* name);
	const	char*				Name() const;

			void				SetRealName(const char* realName);
	const	char*				RealName() const;

			void				SetReturnAddress(const char* returnAddress);
	const	char*				ReturnAddress() const;

			bool				SetInboundAddon(const char* name);
			bool				SetOutboundAddon(const char* name);
	const	entry_ref&			InboundPath() const;
	const	entry_ref&			OutboundPath() const;

			MailAddonSettings&	InboundSettings();
			MailAddonSettings&	OutboundSettings();

			bool				HasInbound();
			bool				HasOutbound();

			void				SetInboundEnabled(bool enabled = true);
			bool				IsInboundEnabled() const;
			void				SetOutboundEnabled(bool enabled = true);
			bool				IsOutboundEnabled() const;

			status_t			Reload();
			status_t			Save();
			status_t			Delete();

			bool				HasBeenModified();

	const	BEntry&				AccountFile();

private:
			status_t			_CreateAccountFilePath();

private:
			status_t			fStatus;
			BEntry				fAccountFile;

			int32				fAccountID;

			BString				fAccountName;
			BString				fRealName;
			BString				fReturnAdress;

			MailAddonSettings	fInboundSettings;
			MailAddonSettings	fOutboundSettings;

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
