/*
 * Copyright 2013-2016, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


/*!	This class offers a high level look at an IMAP mailbox.
*/


#include "IMAPMailbox.h"


IMAPMailbox::IMAPMailbox(IMAP::Protocol& protocol, const BString& mailboxName)
	:
	fProtocol(protocol),
	fMailboxName(mailboxName)
{
}


IMAPMailbox::~IMAPMailbox()
{
}


void
IMAPMailbox::AddMessageEntry(uint32 index, uint32 uid, uint32 flags,
	uint32 size)
{
	fMessageEntries.insert(
		std::make_pair(uid, MessageFlagsAndSize(flags, size)));

	fUIDs.reserve(index + 1);
	fUIDs[index] = uid;
}


void
IMAPMailbox::RemoveMessageEntry(uint32 index)
{
	if (index >= fUIDs.size())
		return;

	uint32 uid = fUIDs[index];
	fMessageEntries.erase(uid);

	fUIDs.erase(fUIDs.begin() + index);
}


uint32
IMAPMailbox::UIDForIndex(uint32 index) const
{
	if (index >= fUIDs.size())
		return 0;

	return fUIDs[index];
}


uint32
IMAPMailbox::MessageFlags(uint32 uid) const
{
	MessageEntryMap::const_iterator found = fMessageEntries.find(uid);
	if (found == fMessageEntries.end())
		return 0;

	return found->second.flags;
}


uint32
IMAPMailbox::MessageSize(uint32 uid) const
{
	MessageEntryMap::const_iterator found = fMessageEntries.find(uid);
	if (found == fMessageEntries.end())
		return 0;

	return found->second.size;
}


uint32
IMAPMailbox::MessageAdded(const MessageToken& fromToken, const entry_ref& ref)
{
	printf("IMAP: message added %s, uid %" B_PRIu32 "\n", ref.name,
		fromToken.uid);
	return fromToken.uid;
}


void
IMAPMailbox::MessageDeleted(const MessageToken& token)
{
	printf("IMAP: message deleted, uid %" B_PRIu32 "\n", token.uid);
}


void
IMAPMailbox::MessageFlagsChanged(const MessageToken& token,
	const entry_ref& ref, uint32 oldFlags, uint32 newFlags)
{
	printf("IMAP: flags changed %s, uid %" B_PRIu32 ", from %" B_PRIx32 " to %"
		B_PRIx32 "\n", ref.name, token.uid, oldFlags, newFlags);
}
