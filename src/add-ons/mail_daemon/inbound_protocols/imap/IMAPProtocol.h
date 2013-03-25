/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_PROTOCOL_H
#define IMAP_PROTOCOL_H


#include <set>

#include <MailProtocol.h>
#include <ObjectList.h>

#include "Settings.h"


class IMAPConnectionWorker;
namespace IMAP {
	class Protocol;
}


typedef std::set<BString> StringSet;


class IMAPProtocol : public BInboundMailProtocol {
public:
								IMAPProtocol(
									const BMailAccountSettings& settings);
	virtual						~IMAPProtocol();

			status_t			CheckSubscribedFolders(
									IMAP::Protocol& protocol);

	virtual	status_t			SyncMessages();
	virtual status_t			FetchBody(const entry_ref& ref);
	virtual	status_t			MarkMessageAsRead(const entry_ref& ref,
									read_flags flags = B_READ);
	virtual	status_t			DeleteMessage(const entry_ref& ref);
	virtual	status_t			AppendMessage(const entry_ref& ref);

	virtual void				MessageReceived(BMessage* message);

protected:
			void				ReadyToRun();

protected:
			Settings			fSettings;
			BObjectList<IMAPConnectionWorker> fWorkers;
			StringSet			fKnownMailboxes;
};


#endif	// IMAP_PROTOCOL_H
