/*
 * Copyright 2010, Haiku Inc. All Rights Reserved.
 * Copyright 2010 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_STORAGE_H
#define IMAP_STORAGE_H


#include <map>
#include <string.h>

#include <File.h>
#include <kernel/OS.h>
#include <Path.h>
#include <String.h>

#include "IMAPHandler.h"


enum FileFlags {
	kSeen = 0x01,
	kAnswered = 0x02,
	kFlagged = 0x04,
	kDeleted = 0x08,
	kDraft = 0x10
};


struct StorageMailEntry {
	int32		uid;
	int32		flags;
	node_ref	nodeRef;
	BString		fileName;
};


typedef std::map<int32, StorageMailEntry> MailEntryMap;


class IMAPStorage {
public:
			enum MessageState {
				kHeaderDownloaded = 0x01,
				kBodyDownloaded = 0x02
			};

								IMAPStorage();
								~IMAPStorage();

			void				SetTo(const char* dir);

			status_t			StartReadDatabase();
			status_t			WaitForDatabaseRead();

			status_t			AddNewMessage(int32 uid, int32 flags,
									BPositionIO** file = NULL);
			status_t			OpenMessage(int32 uid, BPositionIO** file);

			/*! Remove the message from the storage. */
			status_t			DeleteMessage(int32 uid);

			status_t			SetFlags(int32 uid, int32 flags);
			int32				GetFlags(int32 uid);

			status_t			SetFileName(int32 uid, const BString& name);
			status_t			FileRenamed(const entry_ref& from,
									const entry_ref& to);

	const	MailEntryMap&		GetFiles() { return fMailEntryMap; }
			bool				HasFile(int32 uid);

			status_t			SetCompleteMessageSize(int32 uid, int32 size);

			bool				BodyFetched(int32 uid);

			StorageMailEntry*	GetEntryForRef(const node_ref& ref);
			int32				RefToUID(const entry_ref& ref);
			bool				UIDToRef(int32 uid, entry_ref& ref);

			status_t			ReadUniqueID(BNode& node, int32& uid);

private:
	static	status_t			_ReadFilesThreadFunction(void* data);
			status_t			_ReadFiles();

			status_t			_WriteFlags(int32 flags, BNode& node);
			status_t			_WriteUniqueID(BNode& node, int32 uid);

private:
			BPath				fMailboxPath;

			sem_id				fLoadDatabaseLock;
			MailEntryMap		fMailEntryMap;
};


class IMAPMailbox;
typedef std::vector<int32> MessageNumberList;

class IMAPMailboxSync {
public:
			status_t			Sync(IMAPStorage& storage,
									 IMAPMailbox& mailbox);
	const	MessageNumberList&	ToFetchList() { return fToFetchList; }

private:
			MessageNumberList	fToFetchList;
};


#endif // IMAP_STORAGE_H
