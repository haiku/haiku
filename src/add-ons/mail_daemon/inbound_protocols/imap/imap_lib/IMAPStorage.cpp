/*
 * Copyright 2010-2011, Haiku Inc. All Rights Reserved.
 * Copyright 2010 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "IMAPStorage.h"

#include <stdlib.h>

#include <Directory.h>
#include <Entry.h>
#include <NodeInfo.h>
#include <OS.h>

#include <mail_util.h>

#include "IMAPMailbox.h"


#define DEBUG_IMAP_STORAGE
#ifdef DEBUG_IMAP_STORAGE
#	include <stdio.h>
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) /* nothing */
#endif


status_t
IMAPMailboxSync::Sync(IMAPStorage& storage, IMAPMailbox& mailbox)
{
	const MailEntryMap& files = storage.GetFiles();
	const MinMessageList& messages = mailbox.GetMessageList();

	for (MailEntryMap::const_iterator it = files.begin(); it != files.end();
		it++) {
		const StorageMailEntry& mailEntry = (*it).second;
		bool found = false;
		for (unsigned int a = 0; a < messages.size(); a++) {
			const MinMessage& minMessage = messages[a];
			if (mailEntry.uid == minMessage.uid) {
				found = true;
				if (mailEntry.flags != minMessage.flags)
					storage.SetFlags(mailEntry.uid, minMessage.flags);
				break;
			}
		}
		if (!found)
			storage.DeleteMessage(mailEntry.uid);
	}

	for (unsigned int i = 0; i < messages.size(); i++) {
		const MinMessage& minMessage = messages[i];
		bool found = false;
		for (MailEntryMap::const_iterator it = files.begin(); it != files.end();
			it++) {
			const StorageMailEntry& mailEntry = (*it).second;

			if (mailEntry.uid == minMessage.uid) {
				found = true;
				break;
			}
		}
		if (!found)
			fToFetchList.push_back(i + 1);
	}

	mailbox.Listener().NewMessagesToFetch(fToFetchList.size());

	// fetch headers in big bunches if possible
	int32 fetchCount = 0;
	int32 start = -1;
	int32 end = -1;
	int32 lastId = -1;
	for (unsigned int i = 0; i < fToFetchList.size(); i++) {
		if (mailbox.StopNow())
			return B_ERROR;

		int32 current = fToFetchList[i];
		fetchCount++;

		if (start < 0) {
			start = current;
			lastId = current;
			continue;
		}
		if (current - 1 == lastId) {
			end = current;
			lastId = current;
			// limit to a fix number to make it interruptable see StopNow()
			if (fetchCount < 250)
				continue;
		}

		fetchCount = 0;
		mailbox.FetchMessages(start, end);
		start = -1;
		end = -1;
	}
	if (start > 0)
		mailbox.FetchMessages(start, end);

	return B_OK;
}


// #pragma mark -


IMAPStorage::IMAPStorage()
{
	fLoadDatabaseLock = create_sem(1, "sync lock");
}


IMAPStorage::~IMAPStorage()
{
	delete_sem(fLoadDatabaseLock);
}


void
IMAPStorage::SetTo(const char* dir)
{
	fMailboxPath.SetTo(dir);
}


status_t
IMAPStorage::StartReadDatabase()
{
	status_t status = create_directory(fMailboxPath.Path(), 0755);
	if (status != B_OK)
		return status;

	thread_id id = spawn_thread(_ReadFilesThreadFunction, "read mailbox",
		B_LOW_PRIORITY, this);
	if (id < 0)
		return id;

	// will be unlocked from thread
	acquire_sem(fLoadDatabaseLock);
	status = resume_thread(id);
	if (status != B_OK)
		release_sem(fLoadDatabaseLock);
	return status;
}


status_t
IMAPStorage::WaitForDatabaseRead()
{
	// just wait for thread
	if (acquire_sem(fLoadDatabaseLock) != B_OK)
		return B_ERROR;
	release_sem(fLoadDatabaseLock);
	return B_OK;
}


status_t
IMAPStorage::AddNewMessage(int32 uid, int32 flags, BPositionIO** file)
{
	if (file != NULL)
		*file = NULL;
	// TODO: make sure there is not a valid mail with this name
	BString fileName = "Downloading file... uid: ";
	fileName << uid;

	BPath filePath = fMailboxPath;
	filePath.Append(fileName);
	TRACE("AddNewMessage %s\n", filePath.Path());
	BFile* newFile = new BFile(filePath.Path(), B_READ_WRITE | B_CREATE_FILE
		| B_ERASE_FILE);
	if (newFile == NULL)
		return B_NO_MEMORY;

	StorageMailEntry storageEntry;
	storageEntry.uid = uid;
	storageEntry.flags = flags;
	storageEntry.fileName = fileName;
	newFile->GetNodeRef(&storageEntry.nodeRef);

	if (_WriteUniqueID(*newFile, uid) != B_OK) {
		delete newFile;
		return B_ERROR;
	}

	status_t status = _WriteFlags(flags, *newFile);
	if (status != B_OK) {
		delete newFile;
		return status;
	}

	if (file)
		*file = newFile;
	else
		delete newFile;
	fMailEntryMap[uid] = storageEntry;
	return B_OK;
}


status_t
IMAPStorage::OpenMessage(int32 uid, BPositionIO** file)
{
	*file = NULL;

	MailEntryMap::const_iterator it = fMailEntryMap.find(uid);
	if (it == fMailEntryMap.end())
		return B_BAD_VALUE;

	const StorageMailEntry& storageEntry = (*it).second;
	BPath filePath = fMailboxPath;
	filePath.Append(storageEntry.fileName);
	BFile* ourFile = new BFile(filePath.Path(), B_READ_WRITE);
	if (!ourFile)
		return B_NO_MEMORY;
	status_t status = ourFile->InitCheck();
	if (status != B_OK) {
		delete *file;
		return status;
	}
	*file = ourFile;
	return B_OK;
}


status_t
IMAPStorage::DeleteMessage(int32 uid)
{
	MailEntryMap::iterator it = fMailEntryMap.find(uid);
	if (it == fMailEntryMap.end())
		return B_ENTRY_NOT_FOUND;
	const StorageMailEntry& storageEntry = (*it).second;

	BPath filePath = fMailboxPath;
	filePath.Append(storageEntry.fileName);
	BEntry entry(filePath.Path());
	TRACE("IMAPStorage::DeleteMessage %s, %ld\n", filePath.Path(), uid);

	status_t status = entry.Remove();
	if (status != B_OK)
		return status;
	fMailEntryMap.erase(it);
	return B_OK;
}


status_t
IMAPStorage::SetFlags(int32 uid, int32 flags)
{
	MailEntryMap::iterator it = fMailEntryMap.find(uid);
	if (it == fMailEntryMap.end())
		return B_ENTRY_NOT_FOUND;
	StorageMailEntry& entry = (*it).second;

	BPath filePath = fMailboxPath;
	filePath.Append(entry.fileName);
	BNode node(filePath.Path());

	status_t status = _WriteFlags(flags, node);
	if (status != B_OK)
		return status;

	entry.flags = flags;
	return B_OK;
}


int32
IMAPStorage::GetFlags(int32 uid)
{
	MailEntryMap::const_iterator it = fMailEntryMap.find(uid);
	if (it == fMailEntryMap.end())
		return -1;
	const StorageMailEntry& entry = (*it).second;
	return entry.flags;
}


status_t
IMAPStorage::SetFileName(int32 uid, const BString& name)
{
	MailEntryMap::iterator it = fMailEntryMap.find(uid);
	if (it == fMailEntryMap.end())
		return -1;
	StorageMailEntry& storageEntry = (*it).second;
	BPath filePath = fMailboxPath;
	filePath.Append(storageEntry.fileName);
	BEntry entry(filePath.Path());
	status_t status = entry.Rename(name);
	if (status != B_OK)
		return status;
	storageEntry.fileName = name;
	return B_OK;
}


status_t
IMAPStorage::FileRenamed(const entry_ref& from, const entry_ref& to)
{
	int32 uid = RefToUID(from);
	if (uid < 0)
		return B_BAD_VALUE;
	fMailEntryMap[uid].fileName = to.name;
	return B_OK;
}


bool
IMAPStorage::HasFile(int32 uid)
{
	MailEntryMap::const_iterator it = fMailEntryMap.find(uid);
	if (it == fMailEntryMap.end())
		return false;
	return true;
}


status_t
IMAPStorage::SetCompleteMessageSize(int32 uid, int32 size)
{
	MailEntryMap::iterator it = fMailEntryMap.find(uid);
	if (it == fMailEntryMap.end())
		return false;
	StorageMailEntry& storageEntry = (*it).second;
	BPath filePath = fMailboxPath;
	filePath.Append(storageEntry.fileName);
	BNode node(filePath.Path());

	if (node.WriteAttr("MAIL:size", B_INT32_TYPE, 0, &size, sizeof(int32)) < 0)
		return B_ERROR;
	return B_OK;
}


StorageMailEntry*
IMAPStorage::GetEntryForRef(const node_ref& ref)
{
	for (MailEntryMap::iterator it = fMailEntryMap.begin();
		it != fMailEntryMap.end(); it++) {
		StorageMailEntry& mailEntry = (*it).second;
		if (mailEntry.nodeRef == ref)
			return &mailEntry;
	}
	return NULL;
}


bool
IMAPStorage::BodyFetched(int32 uid)
{
	MailEntryMap::iterator it = fMailEntryMap.find(uid);
	if (it == fMailEntryMap.end())
		return false;
	StorageMailEntry& storageEntry = (*it).second;
	BPath filePath = fMailboxPath;
	filePath.Append(storageEntry.fileName);
	BNode node(filePath.Path());

	char buffer[B_MIME_TYPE_LENGTH];
	BNodeInfo info(&node);
	info.GetType(buffer);
	if (strcmp(buffer, B_MAIL_TYPE) == 0)
		return true;
	return false;
}


int32
IMAPStorage::RefToUID(const entry_ref& ref)
{
	for (MailEntryMap::iterator it = fMailEntryMap.begin();
		it != fMailEntryMap.end(); it++) {
		StorageMailEntry& mailEntry = (*it).second;
		if (mailEntry.fileName == ref.name)
			return mailEntry.uid;
	}

	// not found try to fix internal name
	BDirectory dir(fMailboxPath.Path());
	if (!dir.Contains(ref.name))
		return -1;

	BNode node(&ref);
	int32 uid;
	if (ReadUniqueID(node, uid) != B_OK)
		return -1;

	MailEntryMap::iterator it = fMailEntryMap.find(uid);
	if (it == fMailEntryMap.end())
		return -1;
	it->second.fileName = ref.name;

	return uid;
}


bool
IMAPStorage::UIDToRef(int32 uid, entry_ref& ref)
{
	MailEntryMap::iterator it = fMailEntryMap.find(uid);
	if (it == fMailEntryMap.end())
		return false;

	StorageMailEntry& mailEntry = (*it).second;
	BPath filePath = fMailboxPath;
	filePath.Append(mailEntry.fileName);

	BEntry entry(filePath.Path());
	if (entry.GetRef(&ref) == B_OK)
		return true;
	return false;
}


status_t
IMAPStorage::_ReadFilesThreadFunction(void* data)
{
	IMAPStorage* storage = (IMAPStorage*)data;
	return storage->_ReadFiles();
}


status_t
IMAPStorage::_ReadFiles()
{
	fMailEntryMap.clear();

	BDirectory directory(fMailboxPath.Path());

	entry_ref ref;
	while (directory.GetNextRef(&ref) != B_ENTRY_NOT_FOUND) {
		BNode node(&ref);
		if (node.InitCheck() != B_OK || !node.IsFile())
			continue;

		char buffer[B_MIME_TYPE_LENGTH];
		BNodeInfo info(&node);
		info.GetType(buffer);

		// maybe interrupted downloads ignore them
		if (strcmp(buffer, "text/x-partial-email") != 0
			&& strcmp(buffer, B_MAIL_TYPE) != 0)
			continue;

		StorageMailEntry entry;
		entry.fileName = ref.name;
		if (ReadUniqueID(node, entry.uid) != B_OK) {
			TRACE("IMAPStorage::_ReadFilesThread() failed to read uid %s\n",
				ref.name);
			continue;
		}

		if (node.ReadAttr("MAIL:server_flags", B_INT32_TYPE, 0, &entry.flags,
			sizeof(int32)) != sizeof(int32))
			continue;

		node.GetNodeRef(&entry.nodeRef);

		fMailEntryMap[entry.uid] = entry;
	}

	release_sem(fLoadDatabaseLock);
	return B_OK;
}


status_t
IMAPStorage::_WriteFlags(int32 flags, BNode& node)
{
	if ((flags & kSeen) != 0)
		write_read_attr(node, B_READ);
	else
		write_read_attr(node, B_UNREAD);

	ssize_t writen = node.WriteAttr("MAIL:server_flags", B_INT32_TYPE, 0,
		&flags, sizeof(int32));
	if (writen != sizeof(int32))
		return writen;

	return B_OK;
}


status_t
IMAPStorage::ReadUniqueID(BNode& node, int32& uid)
{
	const uint32 kMaxUniqueLength = 32;
	char uidString[kMaxUniqueLength];
	memset(uidString, 0, kMaxUniqueLength);
	if (node.ReadAttr("MAIL:unique_id", B_STRING_TYPE, 0, uidString,
		kMaxUniqueLength) < 0)
		return B_ERROR;
	uid = atoi(uidString);
	return B_OK;
}


status_t
IMAPStorage::_WriteUniqueID(BNode& node, int32 uid)
{
	BString uidString;
	uidString << uid;
	ssize_t written = node.WriteAttr("MAIL:unique_id", B_STRING_TYPE, 0,
		uidString.String(), uidString.Length());
	if (written < 0)
		return written;
	return B_OK;
}
