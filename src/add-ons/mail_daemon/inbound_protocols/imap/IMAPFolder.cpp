/*
 * Copyright 2012-2016, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IMAPFolder.h"

#include <set>

#include <ByteOrder.h>
#include <Debug.h>
#include <Directory.h>
#include <File.h>
#include <fs_attr.h>
#include <Messenger.h>
#include <Node.h>
#include <Path.h>

#include <NodeMessage.h>

#include "IMAPProtocol.h"


static const char* kMailboxNameAttribute = "IMAP:mailbox";
static const char* kUIDValidityAttribute = "IMAP:uidvalidity";
static const char* kLastUIDAttribute = "IMAP:lastuid";
static const char* kStateAttribute = "IMAP:state";
static const char* kFlagsAttribute = "IMAP:flags";
static const char* kUIDAttribute = "MAIL:unique_id";


class TemporaryFile : public BFile {
public:
	TemporaryFile(BFile& file)
		:
		fFile(file),
		fDeleteFile(false)
	{
	}

	~TemporaryFile()
	{
		if (fDeleteFile) {
			fFile.Unset();
			BEntry(fPath.Path()).Remove();
		}
	}

	status_t Init(const BPath& path, entry_ref& ref)
	{
		int32 tries = 53;
		while (true) {
			BString name("temp-mail-");
			name << system_time();

			fPath = path;
			fPath.Append(name.String());

			status_t status = fFile.SetTo(fPath.Path(),
				B_CREATE_FILE | B_FAIL_IF_EXISTS | B_READ_WRITE);
			if (status == B_FILE_EXISTS && tries-- > 0)
				continue;
			if (status != B_OK)
				return status;

			fDeleteFile = true;
			return get_ref_for_path(fPath.Path(), &ref);
		}
	}

	void KeepFile()
	{
		fDeleteFile = false;
	}

private:
			BFile&				fFile;
			BPath				fPath;
			bool				fDeleteFile;
};


// #pragma mark -


IMAPFolder::IMAPFolder(IMAPProtocol& protocol, const BString& mailboxName,
	const entry_ref& ref)
	:
	BHandler(mailboxName.String()),
	fProtocol(protocol),
	fRef(ref),
	fMailboxName(mailboxName),
	fUIDValidity(UINT32_MAX),
	fLastUID(0),
	fListener(NULL),
	fFolderStateInitialized(false),
	fQuitFolderState(false)
{
	mutex_init(&fLock, "imap folder lock");
	mutex_init(&fFolderStateLock, "imap folder state lock");
}


IMAPFolder::~IMAPFolder()
{
	MutexLocker locker(fLock);
	if (!fFolderStateInitialized && fListener != NULL) {
		fQuitFolderState = true;
		locker.Unlock();
		wait_for_thread(fReadFolderStateThread, NULL);
	}
}


status_t
IMAPFolder::Init()
{
	// Initialize from folder attributes
	BNode node(&fRef);
	status_t status = node.InitCheck();
	if (status != B_OK)
		return status;

	node_ref nodeRef;
	status = node.GetNodeRef(&nodeRef);
	if (status != B_OK)
		return status;

	fNodeID = nodeRef.node;

	BString originalMailboxName;
	if (node.ReadAttrString(kMailboxNameAttribute, &originalMailboxName) == B_OK
		&& originalMailboxName != fMailboxName) {
		// TODO: mailbox name has changed
	}

	fUIDValidity = _ReadUInt32(node, kUIDValidityAttribute);
	fLastUID = _ReadUInt32(node, kLastUIDAttribute);
	printf("IMAP: %s, last UID %" B_PRIu32 "\n", fMailboxName.String(),
		fLastUID);

	attr_info info;
	status = node.GetAttrInfo(kStateAttribute, &info);
	if (status == B_OK) {
		struct entry {
			uint32	uid;
			uint32	flags;
		} _PACKED;
		struct entry* entries = (struct entry*)malloc(info.size);
		if (entries == NULL)
			return B_NO_MEMORY;

		ssize_t bytesRead = node.ReadAttr(kStateAttribute, B_RAW_TYPE, 0,
			entries, info.size);
		if (bytesRead != info.size)
			return B_BAD_DATA;

		for (size_t i = 0; i < info.size / sizeof(entry); i++) {
			uint32 uid = B_BENDIAN_TO_HOST_INT32(entries[i].uid);
			uint32 flags = B_BENDIAN_TO_HOST_INT32(entries[i].flags);

			fFlagsMap.insert(std::make_pair(uid, flags));
		}
	}

	return B_OK;
}


void
IMAPFolder::SetListener(FolderListener* listener)
{
	ASSERT(fListener == NULL);

	fListener = listener;

	// Initialize current state from actual folder
	// TODO: this should be done in another thread
	_InitializeFolderState();
}


void
IMAPFolder::SetUIDValidity(uint32 uidValidity)
{
	if (fUIDValidity == uidValidity)
		return;

	// TODO: delete all mails that have the same UID validity value we had
	fUIDValidity = uidValidity;

	BNode node(&fRef);
	_WriteUInt32(node, kUIDValidityAttribute, uidValidity);
}


status_t
IMAPFolder::GetMessageEntryRef(uint32 uid, entry_ref& ref)
{
	MutexLocker locker(fLock);
	return _GetMessageEntryRef(uid, ref);
}


status_t
IMAPFolder::GetMessageUID(const entry_ref& ref, uint32& uid) const
{
	BNode node(&ref);
	status_t status = node.InitCheck();
	if (status != B_OK)
		return status;

	uid = _ReadUniqueID(node);
	if (uid == 0)
		return B_ENTRY_NOT_FOUND;

	return B_OK;
}


uint32
IMAPFolder::MessageFlags(uint32 uid)
{
	MutexLocker locker(fLock);
	UIDToFlagsMap::const_iterator found = fFlagsMap.find(uid);
	if (found == fFlagsMap.end())
		return 0;

	return found->second;
}


/*!	Synchronizes the message flags/state from the server with the local
	one.
*/
void
IMAPFolder::SyncMessageFlags(uint32 uid, uint32 mailboxFlags)
{
	if (uid > LastUID())
		return;

	entry_ref ref;
	BNode node;

	while (true) {
		status_t status = GetMessageEntryRef(uid, ref);
		if (status == B_ENTRY_NOT_FOUND) {
			// The message does not exist anymore locally, delete it on the
			// server
			// TODO: copy it to the trash directory first!
			if (fProtocol.Settings()->DeleteRemoteWhenLocal())
				fProtocol.UpdateMessageFlags(*this, uid, IMAP::kDeleted);
			return;
		}
		if (status == B_OK)
			status = node.SetTo(&ref);
		if (status == B_TIMED_OUT) {
			// We don't know the message state yet
			fPendingFlagsMap.insert(std::make_pair(uid, mailboxFlags));
		}
		if (status != B_OK)
			return;

		break;
	}
	fSynchronizedUIDsSet.insert(uid);

	uint32 previousFlags = MessageFlags(uid);
	uint32 currentFlags = previousFlags;
	if (_MailToIMAPFlags(node, currentFlags) != B_OK)
		return;

	// Compare flags to previous/current flags, and update either the
	// message on the server, or the message locally (or even both)

	uint32 nextFlags = mailboxFlags;
	_TestMessageFlags(previousFlags, mailboxFlags, currentFlags,
		IMAP::kSeen, nextFlags);
	_TestMessageFlags(previousFlags, mailboxFlags, currentFlags,
		IMAP::kAnswered, nextFlags);

	if (nextFlags != previousFlags)
		_WriteFlags(node, nextFlags);
	if (currentFlags != nextFlags) {
		// Update mail message attributes
		BMessage attributes;
		_IMAPToMailFlags(nextFlags, attributes);
		node << attributes;

		fFlagsMap[uid] = nextFlags;
	}
	if (mailboxFlags != nextFlags) {
		// Update server flags
		fProtocol.UpdateMessageFlags(*this, uid, nextFlags);
	}
}


void
IMAPFolder::MessageEntriesFetched()
{
	_WaitForFolderState();

	// Synchronize all pending flags first
	UIDToFlagsMap::const_iterator pendingIterator = fPendingFlagsMap.begin();
	for (; pendingIterator != fPendingFlagsMap.end(); pendingIterator++)
		SyncMessageFlags(pendingIterator->first, pendingIterator->second);

	fPendingFlagsMap.clear();

	// Delete all local messages that are no longer found on the server

	MutexLocker locker(fLock);
	UIDSet deleteUIDs;
	UIDToRefMap::const_iterator iterator = fRefMap.begin();
	for (; iterator != fRefMap.end(); iterator++) {
		uint32 uid = iterator->first;
		if (fSynchronizedUIDsSet.find(uid) == fSynchronizedUIDsSet.end())
			deleteUIDs.insert(uid);
	}

	fSynchronizedUIDsSet.clear();
	locker.Unlock();

	UIDSet::const_iterator deleteIterator = deleteUIDs.begin();
	for (; deleteIterator != deleteUIDs.end(); deleteIterator++)
		_DeleteLocalMessage(*deleteIterator);
}


/*!	Stores the given \a stream into a temporary file using the provided
	BFile object. A new file will be created, and the \a ref object will
	point to it. The file will remain open when this method exits without
	an error.

	\a length will reflect how many bytes are left to read in case there
	was an error.
*/
status_t
IMAPFolder::StoreMessage(uint32 fetchFlags, BDataIO& stream,
	size_t& length, entry_ref& ref, BFile& file)
{
	BPath path;
	status_t status = path.SetTo(&fRef);
	if (status != B_OK)
		return status;

	TemporaryFile temporaryFile(file);
	status = temporaryFile.Init(path, ref);
	if (status != B_OK)
		return status;

	status = _WriteStream(file, stream, length);
	if (status == B_OK)
		temporaryFile.KeepFile();

	return status;
}


/*!	Writes UID, and flags to the message, and notifies the protocol that a
	message has been fetched. This method also closes the \a file passed in.
*/
void
IMAPFolder::MessageStored(entry_ref& ref, BFile& file, uint32 fetchFlags,
	uint32 uid, uint32 flags)
{
	_WriteUniqueIDValidity(file);
	_WriteUniqueID(file, uid);
	if ((fetchFlags & IMAP::kFetchFlags) != 0)
		_WriteFlags(file, flags);

	BMessage attributes;
	_IMAPToMailFlags(flags, attributes);

	fProtocol.MessageStored(*this, ref, file, fetchFlags, attributes);
	file.Unset();

	fRefMap.insert(std::make_pair(uid, ref));

	if (uid > fLastUID) {
		// Update last known UID
		fLastUID = uid;

		BNode directory(&fRef);
		status_t status = _WriteUInt32(directory, kLastUIDAttribute, uid);
		if (status != B_OK) {
			fprintf(stderr, "IMAP: Could not write last UID for mailbox "
				"%s: %s\n", fMailboxName.String(), strerror(status));
		}
	}
}


/*!	Pushes the refs for the pending bodies to the pending bodies list.
	This allows to prevent retrieving bodies more than once.
*/
void
IMAPFolder::RegisterPendingBodies(IMAP::MessageUIDList& uids,
	const BMessenger* replyTo)
{
	MutexLocker locker(fLock);

	MessengerList messengers;
	if (replyTo != NULL)
		messengers.push_back(*replyTo);

	IMAP::MessageUIDList::const_iterator iterator = uids.begin();
	for (; iterator != uids.end(); iterator++) {
		if (replyTo != NULL)
			fPendingBodies[*iterator].push_back(*replyTo);
		else
			fPendingBodies[*iterator].begin();
	}
}


/*!	Appends the given \a stream as body to the message file for the
	specified unique ID. The file will remain open when this method exits
	without an error.

	\a length will reflect how many bytes are left to read in case there
	were an error.
*/
status_t
IMAPFolder::StoreBody(uint32 uid, BDataIO& stream, size_t& length,
	entry_ref& ref, BFile& file)
{
	status_t status = GetMessageEntryRef(uid, ref);
	if (status != B_OK)
		return status;

	status = file.SetTo(&ref, B_OPEN_AT_END | B_WRITE_ONLY);
	if (status != B_OK)
		return status;

	BPath path(&ref);
	printf("IMAP: write body to %s\n", path.Path());

	return _WriteStream(file, stream, length);
}


/*!	Notifies the protocol that a body has been fetched.
	This method also closes the \a file passed in.
*/
void
IMAPFolder::BodyStored(entry_ref& ref, BFile& file, uint32 uid)
{
	BMessage attributes;
	fProtocol.MessageStored(*this, ref, file, IMAP::kFetchBody, attributes);
	file.Unset();

	_NotifyStoredBody(ref, uid, B_OK);
}


void
IMAPFolder::StoringBodyFailed(const entry_ref& ref, uint32 uid, status_t error)
{
	_NotifyStoredBody(ref, uid, error);
}


void
IMAPFolder::DeleteMessage(uint32 uid)
{
	// TODO: move message to trash (server side)

	_DeleteLocalMessage(uid);
}


void
IMAPFolder::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BHandler::MessageReceived(message);
			break;
	}
}


void
IMAPFolder::_WaitForFolderState()
{
	while (true) {
		MutexLocker locker(fFolderStateLock);
		if (fFolderStateInitialized)
			return;
	}
}


void
IMAPFolder::_InitializeFolderState()
{
	mutex_lock(&fFolderStateLock);

	fReadFolderStateThread = spawn_thread(&IMAPFolder::_ReadFolderState,
		"IMAP folder state", B_NORMAL_PRIORITY, this);
	if (fReadFolderStateThread >= 0)
		resume_thread(fReadFolderStateThread);
	else
		mutex_unlock(&fFolderStateLock);
}


void
IMAPFolder::_ReadFolderState()
{
	BDirectory directory(&fRef);
	BEntry entry;
	while (directory.GetNextEntry(&entry) == B_OK) {
		entry_ref ref;
		BNode node;
		if (!entry.IsFile() || entry.GetRef(&ref) != B_OK
			|| node.SetTo(&entry) != B_OK)
			continue;

		uint32 uidValidity = _ReadUniqueIDValidity(node);
		if (uidValidity != fUIDValidity) {
			// TODO: add file to mailbox
			continue;
		}
		uint32 uid = _ReadUniqueID(node);
		uint32 flags = _ReadFlags(node);

		MutexLocker locker(fLock);
		if (fQuitFolderState)
			return;

		fRefMap.insert(std::make_pair(uid, ref));
		fFlagsMap.insert(std::make_pair(uid, flags));

//		// TODO: make sure a listener exists at this point!
//		std::set<uint32>::iterator found = lastUIDs.find(uid);
//		if (found != lastUIDs.end()) {
//			// The message is still around
//			lastUIDs.erase(found);
//
//			uint32 flagsFound = MessageFlags(uid);
//			if (flagsFound != flags) {
//				// Its flags have changed locally, and need to be updated
//				fListener->MessageFlagsChanged(_Token(uid), ref,
//					flagsFound, flags);
//			}
//		} else {
//			// This is a new message
//			// TODO: the token must be the originating token!
//			uid = fListener->MessageAdded(_Token(uid), ref);
//			_WriteUniqueID(node, uid);
//		}
//
	}

	fFolderStateInitialized = true;
	mutex_unlock(&fFolderStateLock);
}


/*static*/ status_t
IMAPFolder::_ReadFolderState(void* self)
{
	((IMAPFolder*)self)->_ReadFolderState();
	return B_OK;
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


void
IMAPFolder::_NotifyStoredBody(const entry_ref& ref, uint32 uid, status_t status)
{
	MutexLocker locker(fLock);
	MessengerMap::iterator found = fPendingBodies.find(uid);
	if (found != fPendingBodies.end()) {
		MessengerList messengers = found->second;
		fPendingBodies.erase(found);
		locker.Unlock();

		MessengerList::iterator iterator = messengers.begin();
		for (; iterator != messengers.end(); iterator++)
			BInboundMailProtocol::ReplyBodyFetched(*iterator, ref, status);
	}
}


status_t
IMAPFolder::_GetMessageEntryRef(uint32 uid, entry_ref& ref) const
{
	UIDToRefMap::const_iterator found = fRefMap.find(uid);
	if (found == fRefMap.end())
		return !fFolderStateInitialized ? B_TIMED_OUT : B_ENTRY_NOT_FOUND;

	ref = found->second;
	return B_OK;
}


status_t
IMAPFolder::_DeleteLocalMessage(uint32 uid)
{
	entry_ref ref;
	status_t status = GetMessageEntryRef(uid, ref);
	if (status != B_OK)
		return status;

	fRefMap.erase(uid);
	fFlagsMap.erase(uid);

	BEntry entry(&ref);
	return entry.Remove();
}


void
IMAPFolder::_IMAPToMailFlags(uint32 flags, BMessage& attributes)
{
	// TODO: add some utility function for this in libmail.so
	if ((flags & IMAP::kAnswered) != 0)
		attributes.AddString(B_MAIL_ATTR_STATUS, "Answered");
	else if ((flags & IMAP::kFlagged) != 0)
		attributes.AddString(B_MAIL_ATTR_STATUS, "Starred");
	else if ((flags & IMAP::kSeen) != 0)
		attributes.AddString(B_MAIL_ATTR_STATUS, "Read");
}


status_t
IMAPFolder::_MailToIMAPFlags(BNode& node, uint32& flags)
{
	BString mailStatus;
	status_t status = node.ReadAttrString(B_MAIL_ATTR_STATUS, &mailStatus);
	if (status != B_OK)
		return status;

	flags &= ~(IMAP::kAnswered | IMAP::kSeen);

	// TODO: add some utility function for this in libmail.so
	if (mailStatus == "Answered")
		flags |= IMAP::kAnswered | IMAP::kSeen;
	else if (mailStatus == "Read")
		flags |= IMAP::kSeen;
	else if (mailStatus == "Starred")
		flags |= IMAP::kFlagged | IMAP::kSeen;

	return B_OK;
}


void
IMAPFolder::_TestMessageFlags(uint32 previousFlags, uint32 mailboxFlags,
	uint32 currentFlags, uint32 testFlag, uint32& nextFlags)
{
	if ((previousFlags & testFlag) != (mailboxFlags & testFlag)) {
		if ((previousFlags & testFlag) == (currentFlags & testFlag)) {
			// The flags on the mailbox changed, update local flags
			nextFlags &= ~testFlag;
			nextFlags |= mailboxFlags & testFlag;
		} else {
			// Both flags changed. Since we don't have the means to do
			// conflict resolution, we use a best effort mechanism
			nextFlags |= testFlag;
		}
		return;
	}

	// Previous message flags, and mailbox flags are identical, see
	// if the user changed the flag locally
	if ((currentFlags & testFlag) != (previousFlags & testFlag)) {
		// Flag changed, update mailbox
		nextFlags &= ~testFlag;
		nextFlags |= currentFlags & testFlag;
	}
}


uint32
IMAPFolder::_ReadUniqueID(BNode& node) const
{
	// For compatibility we must assume that the UID is stored as a string
	BString string;
	if (node.ReadAttrString(kUIDAttribute, &string) != B_OK)
		return 0;

	return strtoul(string.String(), NULL, 0);
}


status_t
IMAPFolder::_WriteUniqueID(BNode& node, uint32 uid) const
{
	// For compatibility we must assume that the UID is stored as a string
	BString string;
	string << uid;

	return node.WriteAttrString(kUIDAttribute, &string);
}


uint32
IMAPFolder::_ReadUniqueIDValidity(BNode& node) const
{

	return _ReadUInt32(node, kUIDValidityAttribute);
}


status_t
IMAPFolder::_WriteUniqueIDValidity(BNode& node) const
{
	return _WriteUInt32(node, kUIDValidityAttribute, fUIDValidity);
}


uint32
IMAPFolder::_ReadFlags(BNode& node) const
{
	return _ReadUInt32(node, kFlagsAttribute);
}


status_t
IMAPFolder::_WriteFlags(BNode& node, uint32 flags) const
{
	return _WriteUInt32(node, kFlagsAttribute, flags);
}


uint32
IMAPFolder::_ReadUInt32(BNode& node, const char* attribute) const
{
	uint32 value;
	ssize_t bytesRead = node.ReadAttr(attribute, B_UINT32_TYPE, 0,
		&value, sizeof(uint32));
	if (bytesRead == (ssize_t)sizeof(uint32))
		return value;

	return 0;
}


status_t
IMAPFolder::_WriteUInt32(BNode& node, const char* attribute, uint32 value) const
{
	ssize_t bytesWritten = node.WriteAttr(attribute, B_UINT32_TYPE, 0,
		&value, sizeof(uint32));
	if (bytesWritten == (ssize_t)sizeof(uint32))
		return B_OK;

	return bytesWritten < 0 ? bytesWritten : B_IO_ERROR;
}


status_t
IMAPFolder::_WriteStream(BFile& file, BDataIO& stream, size_t& length) const
{
	char buffer[65535];
	while (length > 0) {
		ssize_t bytesRead = stream.Read(buffer,
			std::min(sizeof(buffer), length));
		if (bytesRead < 0)
			return bytesRead;
		if (bytesRead <= 0)
			break;

		length -= bytesRead;

		ssize_t bytesWritten = file.Write(buffer, bytesRead);
		if (bytesWritten < 0)
			return bytesWritten;
		if (bytesWritten != bytesRead)
			return B_IO_ERROR;
	}

	return B_OK;
}
