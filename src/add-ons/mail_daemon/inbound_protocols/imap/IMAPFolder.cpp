/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IMAPFolder.h"

#include <set>

#include <ByteOrder.h>
#include <Debug.h>
#include <Directory.h>
#include <fs_attr.h>
#include <Node.h>


static const char* kMailboxNameAttribute = "IMAP:mailbox";
static const char* kUIDValidityAttribute = "IMAP:uidvalidity";
static const char* kStateAttribute = "IMAP:state";
static const char* kUIDAttribute = "MAIL:unique_id";


IMAPFolder::IMAPFolder(const entry_ref& ref, FolderListener& listener)
	:
	fRef(ref),
	fListener(listener)
{
	// Initialize from folder attributes
	BNode node(&ref);
	if (node.InitCheck() != B_OK)
		return;

	if (node.ReadAttrString(kMailboxNameAttribute, &fMailboxName) != B_OK)
		return;

	uint32 uidValidity;
	if (node.ReadAttr(kUIDValidityAttribute, B_UINT32_TYPE, 0, &uidValidity,
			sizeof(uint32)) != sizeof(uint32))
		return;

	fUIDValidity = B_BENDIAN_TO_HOST_INT32(uidValidity);

	attr_info info;
	status_t status = node.GetAttrInfo(kStateAttribute, &info);
	if (status == B_OK) {
		struct entry {
			uint32	uid;
			uint32	flags;
		} _PACKED;
		struct entry* entries = (struct entry*)malloc(info.size);
		if (entries == NULL) {
			// TODO: indicate B_NO_MEMORY
			return;
		}

		ssize_t bytesRead = node.ReadAttr(kStateAttribute, B_RAW_TYPE, 0,
			entries, info.size);
		if (bytesRead != info.size) {
			// TODO: indicate read error resp. corrupted data
			return;
		}

		for (size_t i = 0; i < info.size / sizeof(entry); i++) {
			uint32 uid = B_BENDIAN_TO_HOST_INT32(entries[i].uid);
			uint32 flags = B_BENDIAN_TO_HOST_INT32(entries[i].flags);

			fUIDMap.insert(std::make_pair(uid, flags));
		}
	}

	// Initialize current state from actual folder
	// TODO: this should be done in another thread
	_InitializeFolderState();
}


IMAPFolder::~IMAPFolder()
{
}


void
IMAPFolder::SetFolderID(const char* mailboxName, uint32 id)
{
}


void
IMAPFolder::StoreMessage(uint32 uid, ...)
{
}


void
IMAPFolder::DeleteMessage(uint32 uid)
{
}


void
IMAPFolder::SetMessageFlags(uint32 uid, uint32 flags)
{
}


void
IMAPFolder::MessageReceived(BMessage* message)
{
}


void
IMAPFolder::_InitializeFolderState()
{
	// Create set of the last known UID state - if an entry is found, it
	// is being removed from the list. The remaining entries were deleted.
	std::set<uint32> lastUIDs;
	UIDToFlagsMap::iterator iterator = fUIDMap.begin();
	for (; iterator != fUIDMap.end(); iterator++)
		lastUIDs.insert(iterator->first);

	BDirectory directory(&fRef);
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		entry_ref ref;
		BNode node;
		if (!entry.IsFile() || entry.GetRef(&ref) != B_OK
			|| node.SetTo(&entry) != B_OK)
			continue;

		uint32 uid = _ReadUniqueID(node);
		uint32 flags = _ReadFlags(node);

		std::set<uint32>::iterator found = lastUIDs.find(uid);
		if (found != lastUIDs.end()) {
			// The message is still around
			lastUIDs.erase(found);

			UIDToFlagsMap::iterator flagsFound = fUIDMap.find(uid);
			ASSERT(flagsFound != fUIDMap.end());
			if (flagsFound->second != flags) {
				// Its flags have changed locally, and need to be updated
				fListener.MessageFlagsChanged(_Token(uid), ref,
					flagsFound->second, flags);
			}
		} else {
			// This is a new message
			// TODO: the token must be the originating token!
			// TODO: uid might be udpated from the call
			uid = fListener.MessageAdded(_Token(uid), ref);
		}

		fRefMap.insert(std::make_pair(uid, ref));
	}
}


const MessageToken
IMAPFolder::_Token(uint32 uid) const
{
	MessageToken token;
	token.mailboxName = fMailboxName;
	token.uidValidity = fUIDValidity;
	token.uid = uid;

	return token;
}


uint32
IMAPFolder::_ReadUniqueID(BNode& node)
{
	// For compatibility we must assume that the UID is stored as a string
	BString string;
	if (node.ReadAttrString(kUIDAttribute, &string) != B_OK)
		return 0;

	return strtoul(string.String(), NULL, 0);
}


uint32
IMAPFolder::_ReadFlags(BNode& node)
{
	// TODO!
	return 0;
}
