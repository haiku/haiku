/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_INBOUND_PROTOCOL_H
#define IMAP_INBOUND_PROTOCOL_H


#include "MailProtocol.h"

#include <Handler.h>
#include <Locker.h>
#include <Message.h>

#include "MailSettings.h"

#include "IMAPMailbox.h"
#include "IMAPStorage.h"


class DispatcherIMAPListener : public IMAPMailboxListener {
public:
								DispatcherIMAPListener(MailProtocol& protocol,
									IMAPStorage& storage);

			bool				Lock();
			void				Unlock();

			void				HeaderFetched(int32 uid, BPositionIO* data,
									bool bodyIsComming);
			void				BodyFetched(int32 uid, BPositionIO* data);

			void				NewMessagesToFetch(int32 nMessages);
			void				FetchEnd();
private:
			MailProtocol&		fProtocol;
			IMAPStorage&		fStorage;
};


class IMAPInboundProtocol;


/*! Just wait for a IDLE (watching) IMAP response in this thread. */
class IMAPMailboxThread {
public:
								IMAPMailboxThread(IMAPInboundProtocol& protocol,
									IMAPMailbox& mailbox);
								~IMAPMailboxThread();

			bool				IsWatching();
			status_t			SyncAndStartWatchingMailbox();
			status_t			StopWatchingMailbox();

private:
	static	status_t			_WatchThreadFunction(void* data);
			void				_Watch();

private:
			IMAPInboundProtocol& fProtocol;
			IMAPMailbox&		fIMAPMailbox;

			BLocker				fLock;
			bool				fIsWatching;

			thread_id			fThread;
			sem_id				fWatchSyncSem;
};


class IMAPInboundProtocol;


class MailboxWatcher : public BHandler {
public:
								MailboxWatcher(IMAPInboundProtocol* protocol);
								~MailboxWatcher();

			void				StartWatching(const char* mailboxDir);
			void				MessageReceived(BMessage* message);
private:
			node_ref			fWatchDir;
			IMAPInboundProtocol*	fProtocol;
};


class IMAPInboundProtocol : public InboundProtocol {
public:
								IMAPInboundProtocol(
									BMailAccountSettings* settings,
									const char* mailbox);
	virtual						~IMAPInboundProtocol();

			//! thread safe interface
	virtual	status_t			Connect(const char* server,
									const char* username, const char* password,
									bool useSSL = true, int32 port = -1);
	virtual	status_t			Disconnect();
			status_t			Reconnect();
			bool				IsConnected();

	virtual	void				SetStopNow();

			void				AddedToLooper();
			void				UpdateSettings(const BMessage& settings);

			bool				InterestingEntry(const entry_ref& ref);

	virtual	status_t			SyncMessages();
	virtual	status_t			FetchBody(const entry_ref& ref);
	virtual	status_t			MarkMessageAsRead(const entry_ref& ref,
									read_flags flag = B_READ);

	virtual	status_t			DeleteMessage(const entry_ref& ref);
	virtual	status_t			AppendMessage(const entry_ref& ref);

	virtual	status_t			DeleteMessage(node_ref& node);
	//! these should be thread save
	virtual	void				FileRenamed(const entry_ref& from,
									const entry_ref& to);
	virtual	void				FileDeleted(const node_ref& node);

protected:
			BString				fServer;
			BString				fUsername;
			BString				fPassword;
			bool				fUseSSL;
			bool				fIsConnected;

			BString				fMailboxName;
			BPath				fMailboxPath;

			IMAPStorage			fStorage;
			IMAPMailbox			fIMAPMailbox;
			DispatcherIMAPListener fDispatcherIMAPListener;

			MailboxWatcher*		fINBOXWatcher;
			IMAPMailboxThread*	fIMAPMailboxThread;
};


#endif // IMAP_INBOUND_PROTOCOL_H
