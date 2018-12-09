/*
 * Copyright 2013-2016, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_PROTOCOL_H
#define IMAP_PROTOCOL_H


#include <map>

#include <MailProtocol.h>
#include <ObjectList.h>

#include <locks.h>

#include "Commands.h"
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
									IMAP::Protocol& protocol, bool idle);
			void				WorkerQuit(IMAPConnectionWorker* worker);

			void				MessageStored(IMAPFolder& folder,
									entry_ref& ref, BFile& stream,
									uint32 fetchFlags, BMessage& attributes);

			status_t			UpdateMessageFlags(IMAPFolder& folder,
									uint32 uid, uint32 flags);

	virtual	status_t			SyncMessages();
	virtual	status_t			MarkMessageAsRead(const entry_ref& ref,
									read_flags flags = B_READ);

	virtual void				MessageReceived(BMessage* message);
	const ::Settings*			Settings() const { return &fSettings; }

protected:
	virtual status_t			HandleFetchBody(const entry_ref& ref,
									const BMessenger& replyTo);

			void				ReadyToRun();

private:
			IMAPFolder*			_CreateFolder(const BString& mailbox,
									const BString& separator);
			IMAPFolder*			_FolderFor(ino_t directory);
			status_t			_EnqueueCheckMailboxes();

protected:
	typedef std::map<IMAPFolder*, IMAPConnectionWorker*> WorkerMap;
	typedef std::map<ino_t, IMAPFolder*> FolderNodeMap;

			::Settings			fSettings;
			mutex				fWorkerLock;
			BObjectList<IMAPConnectionWorker> fWorkers;
			WorkerMap			fWorkerMap;
			FolderMap			fFolders;
			FolderNodeMap		fFolderNodeMap;
};


#endif	// IMAP_PROTOCOL_H