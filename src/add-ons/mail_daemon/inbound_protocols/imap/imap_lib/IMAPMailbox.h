/*
 * Copyright 2010, Haiku Inc. All Rights Reserved.
 * Copyright 2010 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_MAILBOX_H
#define IMAP_MAILBOX_H


#include <DataIO.h>

#include "IMAPHandler.h"
#include "IMAPProtocol.h"


class IMAPMailboxListener {
public:
	virtual						~IMAPMailboxListener() {}

	virtual void				HeaderFetched(int32 uid, BPositionIO* data,
									bool bodyIsComing) {}
	virtual void				BodyFetched(int32 uid, BPositionIO* data) {}

	virtual	void				NewMessagesToFetch(int32 nMessages) {}
	virtual	void				FetchEnd() {}
};


class IMAPStorage;


class IMAPMailbox : public IMAPProtocol {
public:
								IMAPMailbox(IMAPStorage& storage);
								~IMAPMailbox();

			void				SetListener(IMAPMailboxListener* listener);
			IMAPMailboxListener&	Listener() { return *fIMAPMailboxListener; }

			/*! Select mailbox and sync with the storage. */
			status_t			SelectMailbox(const char* mailbox);
			BString				Mailbox();
			status_t			Sync();

			bool				SupportWatching();
			status_t			StartWatchingMailbox(sem_id startedSem = -1);
			status_t			StopWatchingMailbox();

			status_t			CheckMailbox();

			//! Low level fetch functions.
			status_t			FetchMinMessage(int32 messageNumber,
									BPositionIO** data = NULL);
			status_t			FetchBody(int32 messageNumber,
									BPositionIO* data);

			void				SetFetchBodyLimit(int32 limit);
			int32				FetchBodyLimit() { return fFetchBodyLimit; }
			status_t			FetchMessage(int32 messageNumber);
			status_t			FetchMessages(int32 firstMessage,
									int32 lastMessage);
			status_t			FetchBody(int32 messageNumber);

			status_t			SetFlags(int32 messageNumber, int32 flags);
			status_t			AppendMessage(BPositionIO& message, off_t size,
									int32 flags = 0, time_t time = -1);

			int32				GetCurrentMessageCount()
									{ return fMessageList.size(); }
	const	MinMessageList&		GetMessageList() { return fMessageList; }
			IMAPStorage&		GetStorage() { return fStorage; }

			int32				UIDToMessageNumber(int32 uid);
			int32				MessageNumberToUID(int32 messageNumber);

			status_t			DeleteMessage(int32 uid, bool permanently);
private:
			void				_InstallUnsolicitedHandler(bool install);

			MinMessageList		fMessageList;

			IMAPStorage&		fStorage;
			IMAPMailboxListener* fIMAPMailboxListener;
			IMAPMailboxListener	fNULLListener;

			MailboxSelectHandler fMailboxSelectHandler;
			CapabilityHandler	fCapabilityHandler;
			ExistsHandler		fExistsHandler;
			ExpungeHandler		fExpungeHandler;
			FlagsHandler		fFlagsHandler;

			vint32				fWatching;

			BString				fSelectedMailbox;

			int32				fFetchBodyLimit;
};


#endif // IMAP_MAILBOX_H
