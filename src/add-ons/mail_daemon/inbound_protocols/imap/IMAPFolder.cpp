/*
 * Copyright 2012-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "IMAPFolder.h"

#include <set>

#include <ByteOrder.h>
#include <Debug.h>
#include <Directory.h>
#include <File.h>
#include <fs_attr.h>
#include <Node.h>
#include <Path.h>

#include "Commands.h"

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
	fLastUID(0)
{
	// Initialize from folder attributes
	BNode node(&ref);
	if (node.InitCheck() != B_OK)
		return;

	BString originalMailboxName;
	if (node.ReadAttrString(kMailboxNameAttribute, &originalMailboxName) == B_OK
		&& originalMailboxName != mailboxName) {
		// TODO: mailbox name has changed
	}

	fUIDValidity = _ReadUInt32(node, kUIDValidityAttribute);
	fLastUID = _ReadUInt32(node, kLastUIDAttribute);
	printf("IMAP: %s, last UID %lu\n", fMailboxName.String(), fLastUID);

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

			fFlagsMap.insert(std::make_pair(uid, flags));
		}
	}

	// Initialize current state from actual folder
	// TODO: this should be done in another thread
	//_InitializeFolderState();
}


IMAPFolder::~IMAPFolder()
{
}


void
IMAPFolder::SetListener(FolderListener* listener)
{
	fListener = listener;
}


void
IMAPFolder::SetUIDValidity(uint32 uidValidity)
{
	if (fUIDValidity == uidValidity)
		return;

	fUIDValidity = uidValidity;

	BNode node(&fRef);
	_WriteUInt32(node, kUIDValidityAttribute, uidValidity);
}


status_t
IMAPFolder::GetMessageEntryRef(uint32 uid, entry_ref& ref) const
{
	UIDToRefMap::const_iterator found = fRefMap.find(uid);
	if (found == fRefMap.end())
		return B_ENTRY_NOT_FOUND;

	ref = found->second;
	return B_OK;
}


uint32
IMAPFolder::MessageFlags(uint32 uid) const
{
	UIDToFlagsMap::const_iterator found = fFlagsMap.find(uid);
	if (found == fFlagsMap.end())
		return 0;

	return found->second;
}


/*!	Stores the given \a stream into a temporary file using the provided
	BFile object. A new file will be created, and the \a ref object will
	point to it. The file will remain open when this method exits without
	an error.

	\a length will reflect how many bytes are left to read in case there
	were an error.
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
	_WriteUniqueID(file, uid);
	if ((fetchFlags & IMAP::kFetchFlags) != 0)
		_WriteFlags(file, flags);

	// TODO: add some utility function for this in libmail.so
	BMessage attributes;
	if ((flags & IMAP::kAnswered) != 0)
		attributes.AddString(B_MAIL_ATTR_STATUS, "Answered");
	else if ((flags & IMAP::kSeen) != 0)
		attributes.AddString(B_MAIL_ATTR_STATUS, "Read");

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
}


void
IMAPFolder::DeleteMessage(uint32 uid)
{
}


/*!	Called when the flags of a message changed on the server. This will update
	the flags for the local file.
*/
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
	UIDToFlagsMap::iterator iterator = fFlagsMap.begin();
	for (; iterator != fFlagsMap.end(); iterator++)
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

		// TODO: make sure a listener exists at this point!
		std::set<uint32>::iterator found = lastUIDs.find(uid);
		if (found != lastUIDs.end()) {
			// The message is still around
			lastUIDs.erase(found);

			uint32 flagsFound = MessageFlags(uid);
			if (flagsFound != flags) {
				// Its flags have changed locally, and need to be updated
				fListener->MessageFlagsChanged(_Token(uid), ref,
					flagsFound, flags);
			}
		} else {
			// This is a new message
			// TODO: the token must be the originating token!
			uid = fListener->MessageAdded(_Token(uid), ref);
			_WriteUniqueID(node, uid);
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


status_t
IMAPFolder::_WriteUniqueID(BNode& node, uint32 uid)
{
	BString string;
	string << uid;

	return node.WriteAttrString(kUIDAttribute, &string);
}


uint32
IMAPFolder::_ReadFlags(BNode& node)
{
	return _ReadUInt32(node, kFlagsAttribute);
}


status_t
IMAPFolder::_WriteFlags(BNode& node, uint32 flags)
{
	return _WriteUInt32(node, kFlagsAttribute, flags);
}


uint32
IMAPFolder::_ReadUInt32(BNode& node, const char* attribute)
{
	uint32 value;
	ssize_t bytesRead = node.ReadAttr(attribute, B_UINT32_TYPE, 0,
		&value, sizeof(uint32));
	if (bytesRead == (ssize_t)sizeof(uint32))
		return value;

	return 0;
}


status_t
IMAPFolder::_WriteUInt32(BNode& node, const char* attribute, uint32 value)
{
	ssize_t bytesWritten = node.WriteAttr(attribute, B_UINT32_TYPE, 0,
		&value, sizeof(uint32));
	if (bytesWritten == (ssize_t)sizeof(uint32))
		return B_OK;

	return bytesWritten < 0 ? bytesWritten : B_IO_ERROR;
}


status_t
IMAPFolder::_WriteStream(BFile& file, BDataIO& stream, size_t& length)
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
