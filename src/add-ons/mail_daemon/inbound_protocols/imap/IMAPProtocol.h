/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_PROTOCOL_H
#define IMAP_PROTOCOL_H


#include <map>

#include <MailProtocol.h>
#include <ObjectList.h>

#include "Settings.h"


class IMAPConnectionWorker;
class IMAPFolder;
namespace IMAP {
	class Protocol;
}


typedef std::map<BString, IMAPFolder*> FolderMap;


class IMAPProtocol : public BInboundMailProtocol {
public:
								IMAPProtocol(
									const BMailAccountSettings& settings);
	virtual						~IMAPProtocol();

			status_t			CheckSubscribedFolders(
									IMAP::Protocol& protocol);
			sem_id				FolderChangeSemaphore() const
									{ return fFolderChangeSemaphore; }

	virtual	status_t			SyncMessages();
	virtual status_t			FetchBody(const entry_ref& ref);
	virtual	status_t			MarkMessageAsRead(const entry_ref& ref,
									read_flags flags = B_READ);
	virtual	status_t			DeleteMessage(const entry_ref& ref);
	virtual	status_t			AppendMessage(const entry_ref& ref);

	virtual void				MessageReceived(BMessage* message);

protected:
			void				ReadyToRun();

private:
			IMAPFolder*			_CreateFolder(const BString& mailbox,
									const BString& separator);
			status_t			_CreateFolderChangeSemaphore();

protected:
			Settings			fSettings;
			BObjectList<IMAPConnectionWorker> fWorkers;
			FolderMap			fFolders;
			sem_id				fFolderChangeSemaphore;
};


#endif	// IMAP_PROTOCOL_H
