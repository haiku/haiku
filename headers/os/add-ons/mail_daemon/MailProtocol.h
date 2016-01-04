/*
 * Copyright 2004-2016, Haiku, Inc. All Rights Reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _MAIL_PROTOCOL_H
#define _MAIL_PROTOCOL_H


#include <map>

#include <Looper.h>
#include <OS.h>
#include <ObjectList.h>
#include <Entry.h>
#include <File.h>

#include <E-mail.h>
#include <MailSettings.h>


class BMailFilter;
class BMailSettingsView;
class BView;


class BMailNotifier {
public:
	virtual						~BMailNotifier() {}

	virtual BMailNotifier*		Clone() = 0;

	virtual	void				ShowError(const char* error) = 0;
	virtual	void				ShowMessage(const char* message) = 0;

	virtual void				SetTotalItems(uint32 items) = 0;
	virtual void				SetTotalItemsSize(uint64 size) = 0;
	virtual	void				ReportProgress(uint32 items, uint64 bytes,
									const char* message = NULL) = 0;
	virtual void				ResetProgress(const char* message = NULL) = 0;
};


typedef status_t BMailFilterAction;

#define B_NO_MAIL_ACTION		0
#define B_MOVE_MAIL_ACTION		1
#define B_DELETE_MAIL_ACTION	2


class BMailProtocol : public BLooper {
public:
								BMailProtocol(const char* name,
									const BMailAccountSettings& settings);
	virtual						~BMailProtocol();

			const BMailAccountSettings& AccountSettings() const;

			void				SetMailNotifier(BMailNotifier* mailNotifier);
			BMailNotifier*		MailNotifier() const;

			//! We take ownership of the filters
			bool				AddFilter(BMailFilter* filter);
			int32				CountFilter() const;
			BMailFilter*		FilterAt(int32 index) const;
			BMailFilter*		RemoveFilter(int32 index);
			bool				RemoveFilter(BMailFilter* filter);

	virtual void				MessageReceived(BMessage* message);

			// Mail storage operations
	virtual	status_t			MoveMessage(const entry_ref& ref,
									BDirectory& dir);
	virtual	status_t			DeleteMessage(const entry_ref& ref);

			// Convenience methods that call the BMailNotifier
			void				ShowError(const char* error);
			void				ShowMessage(const char* message);

#if __GNUC__ > 2
			// Unhide virtual base methods
			using BHandler::AddFilter;
			using BHandler::RemoveFilter;
#endif

protected:
		 	void				SetTotalItems(uint32 items);
			void				SetTotalItemsSize(uint64 size);
			void				ReportProgress(uint32 items, uint64 bytes,
									const char* message = NULL);
			void				ResetProgress(const char* message = NULL);
			void				NotifyNewMessagesToFetch(int32 nMessages);

			// Filter notifications
			BMailFilterAction	ProcessHeaderFetched(entry_ref& ref,
									BFile& mail, BMessage& attributes);
			void				NotifyBodyFetched(const entry_ref& ref,
									BFile& mail, BMessage& attributes);
			BMailFilterAction	ProcessMessageFetched(entry_ref& ref,
									BFile& mail, BMessage& attributes);
			void				NotifyMessageReadyToSend(const entry_ref& ref,
									BFile& mail);
			void				NotifyMessageSent(const entry_ref& ref,
									BFile& mail);

			void				LoadFilters(
									const BMailProtocolSettings& settings);

private:
	static	BString				_LooperName(const char* name,
									const BMailAccountSettings& settings);
			BMailFilter*		_LoadFilter(const BMailAddOnSettings& settings);
			BMailFilterAction	_ProcessHeaderFetched(entry_ref& ref,
									BFile& mail, BMessage& attributes);
			void				_NotifyBodyFetched(const entry_ref& ref,
									BFile& mail, BMessage& attributes);

protected:
			const BMailAccountSettings fAccountSettings;
			BMailNotifier*		fMailNotifier;

private:
			BObjectList<BMailFilter> fFilterList;
			std::map<entry_ref, image_id> fFilterImages;
};


class BInboundMailProtocol : public BMailProtocol {
public:
								BInboundMailProtocol(const char* name,
									const BMailAccountSettings& settings);
	virtual						~BInboundMailProtocol();

	virtual void				MessageReceived(BMessage* message);

	virtual	status_t			SyncMessages() = 0;
	virtual status_t			FetchBody(const entry_ref& ref,
									BMessenger* replyTo);
	virtual	status_t			MarkMessageAsRead(const entry_ref& ref,
									read_flags flags = B_READ);
	virtual	status_t			DeleteMessage(const entry_ref& ref);
	virtual	status_t			AppendMessage(const entry_ref& ref);

	static	void				ReplyBodyFetched(const BMessenger& replyTo,
									const entry_ref& ref, status_t status);

protected:
	virtual status_t			HandleFetchBody(const entry_ref& ref,
									const BMessenger& replyTo) = 0;

	virtual	status_t			HandleDeleteMessage(const entry_ref& ref) = 0;

			void				NotiyMailboxSynchronized(status_t status);
};


class BOutboundMailProtocol : public BMailProtocol {
public:
								BOutboundMailProtocol(const char* name,
									const BMailAccountSettings& settings);
	virtual						~BOutboundMailProtocol();

	virtual	status_t			SendMessages(const BMessage& message,
									off_t totalBytes);

	virtual void				MessageReceived(BMessage* message);

protected:
	virtual	status_t			HandleSendMessages(const BMessage& message,
									off_t totalBytes) = 0;
};


// Your protocol needs to export these hooks in order to be picked up
extern "C" BInboundMailProtocol* instantiate_inbound_protocol(
	const BMailAccountSettings& settings);
extern "C" BOutboundMailProtocol* instantiate_outbound_protocol(
	const BMailAccountSettings& settings);

extern "C" BMailSettingsView* instantiate_protocol_settings_view(
	const BMailAccountSettings& accountSettings,
	const BMailProtocolSettings& settings);


#endif	// _MAIL_PROTOCOL_H
