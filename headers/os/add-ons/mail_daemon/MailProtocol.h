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


class MailNotifier {
public:
	virtual						~MailNotifier() {}

	virtual MailNotifier*		Clone() = 0;

	virtual	void				ShowError(const char* error) = 0;
	virtual	void				ShowMessage(const char* message) = 0;

	virtual void				SetTotalItems(int32 items) = 0;
	virtual void				SetTotalItemsSize(int32 size) = 0;
	virtual	void				ReportProgress(int bytes, int messages,
									const char* message = NULL) = 0;
	virtual void				ResetProgress(const char* message = NULL) = 0;
};


class MailProtocol;


class MailFilter {
public:
								MailFilter(MailProtocol& protocol,
									AddonSettings* settings);
	virtual						~MailFilter();

	//! Message hooks if filter is installed to a inbound protocol
	virtual	void				HeaderFetched(const entry_ref& ref,
									BFile* file);
	virtual	void				BodyFetched(const entry_ref& ref, BFile* file);
	virtual void				MailboxSynced(status_t status);

	//! Message hooks if filter is installed to a outbound protocol
	virtual	void				MessageReadyToSend(const entry_ref& ref,
									BFile* file);
	virtual	void				MessageSent(const entry_ref& ref,
									BFile* file);
protected:
			MailProtocol&		fMailProtocol;
			AddonSettings*		fAddonSettings;
};


class MailProtocolThread;


class MailProtocol {
public:
								MailProtocol(BMailAccountSettings* settings);
	virtual						~MailProtocol();

	virtual	void				SetStopNow() {}

			BMailAccountSettings&	AccountSettings();

			void				SetProtocolThread(
									MailProtocolThread* protocolThread);
	virtual	void				AddedToLooper() {}
			MailProtocolThread*	Looper();
			/*! Add handler to the handler list. The handler is installed /
			removed to the according BLooper automatically. */
			bool				AddHandler(BHandler* handler);
			//! Does not delete handler
			bool				RemoveHandler(BHandler* handler);

			void				SetMailNotifier(MailNotifier* mailNotifier);

	virtual	void				ShowError(const char* error);
	virtual	void				ShowMessage(const char* message);
	virtual void				SetTotalItems(int32 items);
	virtual void				SetTotalItemsSize(int32 size);
	virtual void				ReportProgress(int bytes, int messages,
									const char* message = NULL);
	virtual void				ResetProgress(const char* message = NULL);

			//! MailProtocol takes ownership of the filters
			bool				AddFilter(MailFilter* filter);
			int32				CountFilter();
			MailFilter*			FilterAt(int32 index);
			MailFilter*			RemoveFilter(int32 index);
			bool				RemoveFilter(MailFilter* filter);

			void				NotifyNewMessagesToFetch(int32 nMessages);
			void				NotifyHeaderFetched(const entry_ref& ref,
									BFile* mail);
			void				NotifyBodyFetched(const entry_ref& ref,
									BFile* mail);
			void				NotifyMessageReadyToSend(const entry_ref& ref,
									BFile* mail);
			void				NotifyMessageSent(const entry_ref& ref,
									BFile* mail);

			//! mail storage operations
	virtual	status_t			MoveMessage(const entry_ref& ref,
									BDirectory& dir);
	virtual	status_t			DeleteMessage(const entry_ref& ref);

	virtual	void				FileRenamed(const entry_ref& from,
									const entry_ref& to);
	virtual	void				FileDeleted(const node_ref& node);

protected:
			void				LoadFilters(MailAddonSettings& settings);

			BMailAccountSettings	fAccountSettings;
			MailNotifier*		fMailNotifier;

private:
			MailFilter*			_LoadFilter(AddonSettings* filterSettings);

			MailProtocolThread*	fProtocolThread;
			BObjectList<BHandler>	fHandlerList;
			BObjectList<MailFilter>	fFilterList;
			std::map<entry_ref, image_id>	fFilterImages;
};


class InboundProtocol : public MailProtocol {
public:
								InboundProtocol(BMailAccountSettings* settings);
	virtual						~InboundProtocol();

	virtual	status_t			SyncMessages() = 0;
	virtual status_t			FetchBody(const entry_ref& ref) = 0;
	virtual	status_t			MarkMessageAsRead(const entry_ref& ref,
									read_flags flag = B_READ);
	virtual	status_t			DeleteMessage(const entry_ref& ref) = 0;
	virtual	status_t			AppendMessage(const entry_ref& ref);
};


class OutboundProtocol : public MailProtocol {
public:
								OutboundProtocol(
									BMailAccountSettings* settings);
	virtual						~OutboundProtocol();

	virtual	status_t			SendMessages(const std::vector<entry_ref>&
									mails, size_t totalBytes) = 0;
};


class MailProtocolThread : public BLooper {
public:
								MailProtocolThread(MailProtocol* protocol);
	virtual	void				MessageReceived(BMessage* message);

			MailProtocol*		Protocol() { return fMailProtocol; }

			void				SetStopNow();
			/*! These function post a message to the loop to trigger the action.
			*/
			void				TriggerFileMove(const entry_ref& ref,
									BDirectory& dir);
			void				TriggerFileDeletion(const entry_ref& ref);

			void				TriggerFileRenamed(const entry_ref& from,
									const entry_ref& to);
			void				TriggerFileDeleted(const node_ref& node);
private:
			MailProtocol*		fMailProtocol;
};


class InboundProtocolThread : public MailProtocolThread {
public:
								InboundProtocolThread(
									InboundProtocol* protocol);
								~InboundProtocolThread();

			void				MessageReceived(BMessage* message);

			void				SyncMessages();
			void				FetchBody(const entry_ref& ref,
									BMessenger* listener = NULL);
			void				MarkMessageAsRead(const entry_ref& ref,
									read_flags flag = B_READ);
			void				DeleteMessage(const entry_ref& ref);
			void				AppendMessage(const entry_ref& ref);
private:
			void				_NotiyMailboxSynced(status_t status);

			InboundProtocol*	fProtocol;
};


class OutboundProtocolThread : public MailProtocolThread {
public:
								OutboundProtocolThread(
									OutboundProtocol* protocol);
								~OutboundProtocolThread();

			void				MessageReceived(BMessage* message);

			void				SendMessages(const std::vector<entry_ref>&
									mails, size_t totalBytes);

private:
			OutboundProtocol*	fProtocol;
};


#endif // MAIL_PROTOCOL_H
