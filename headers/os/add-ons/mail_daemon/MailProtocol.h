/* Protocol - the base class for protocol filters
 *
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011 Clemens Zeidler. All rights reserved.
*/
#ifndef MAIL_PROTOCOL_H
#define MAIL_PROTOCOL_H


#include <map>
#include <vector>

#include <Handler.h>
#include <Looper.h>
#include <OS.h>
#include <ObjectList.h>
#include <Entry.h>
#include <File.h>

#include <E-mail.h>
#include <MailSettings.h>


class BMailFilter;


class BMailNotifier {
public:
	virtual						~BMailNotifier() {}

	virtual BMailNotifier*		Clone() = 0;

	virtual	void				ShowError(const char* error) = 0;
	virtual	void				ShowMessage(const char* message) = 0;

	virtual void				SetTotalItems(uint32 items) = 0;
	virtual void				SetTotalItemsSize(uint64 size) = 0;
	virtual	void				ReportProgress(uint32 messages, uint64 bytes,
									const char* message = NULL) = 0;
	virtual void				ResetProgress(const char* message = NULL) = 0;
};


class BMailProtocol : BLooper {
public:
								BMailProtocol(
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

	virtual	void				FileRenamed(const entry_ref& from,
									const entry_ref& to);
	virtual	void				FileDeleted(const node_ref& node);

			// Convenience methods that call the BMailNotifier
			void				ShowError(const char* error);
			void				ShowMessage(const char* message);

protected:
		 	void				SetTotalItems(uint32 items);
			void				SetTotalItemsSize(uint64 size);
			void				ReportProgress(uint32 messages, uint64 bytes,
									const char* message = NULL);
			void				ResetProgress(const char* message = NULL);

			// Filter notifications
			void				NotifyNewMessagesToFetch(int32 nMessages);
			void				NotifyHeaderFetched(const entry_ref& ref,
									BFile* mail);
			void				NotifyBodyFetched(const entry_ref& ref,
									BFile* mail);
			void				NotifyMessageReadyToSend(const entry_ref& ref,
									BFile* mail);
			void				NotifyMessageSent(const entry_ref& ref,
									BFile* mail);

			void				LoadFilters(
									const BMailProtocolSettings& settings);

private:
			BMailFilter*		_LoadFilter(BMailAddOnSettings* filterSettings);

protected:
			const BMailAccountSettings fAccountSettings;
			BMailNotifier*		fMailNotifier;

private:
			BObjectList<BMailFilter> fFilterList;
			std::map<entry_ref, image_id> fFilterImages;
};


class BInboundMailProtocol : public BMailProtocol {
public:
								BInboundMailProtocol(
									const BMailAccountSettings& settings);
	virtual						~BInboundMailProtocol();

	virtual void				MessageReceived(BMessage* message);

	virtual	status_t			SyncMessages() = 0;
	virtual status_t			FetchBody(const entry_ref& ref) = 0;
	virtual	status_t			MarkMessageAsRead(const entry_ref& ref,
									read_flags flag = B_READ);
	virtual	status_t			DeleteMessage(const entry_ref& ref) = 0;
	virtual	status_t			AppendMessage(const entry_ref& ref);

protected:
			void				NotiyMailboxSynchronized(status_t status);
};


class BOutboundMailProtocol : public BMailProtocol {
public:
								BOutboundMailProtocol(
									const BMailAccountSettings& settings);
	virtual						~BOutboundMailProtocol();

	virtual void				MessageReceived(BMessage* message);

	virtual	status_t			SendMessages(
									const std::vector<entry_ref>& mails,
									size_t totalBytes) = 0;
};


// Your protocol needs to export these hooks in order to be picked up
extern "C" _EXPORT BInboundMailProtocol* instantiate_inbound_protocol(
	const BMailAccountSettings& settings);
extern "C" _EXPORT BOutboundMailProtocol* instantiate_outbound_protocol(
	const BMailAccountSettings& settings);
extern "C" _EXPORT BView* instantiate_protocol_config_panel(
	BMailAccountSettings& settings);


#endif // MAIL_PROTOCOL_H
