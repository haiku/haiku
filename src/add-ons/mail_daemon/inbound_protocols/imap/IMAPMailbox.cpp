/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
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


uint32
IMAPMailbox::MessageAdded(const MessageToken& fromToken, const entry_ref& ref)
{
	printf("IMAP: message added %s, uid %lu\n", ref.name, fromToken.uid);
	return 0;
}


void
IMAPMailbox::MessageDeleted(const MessageToken& token)
{
	printf("IMAP: message deleted, uid %lu\n", token.uid);
}


void
IMAPMailbox::MessageFlagsChanged(const MessageToken& token,
	const entry_ref& ref, uint32 oldFlags, uint32 newFlags)
{
	printf("IMAP: flags changed %s, uid %lu, from %lx to %lx\n", ref.name,
		token.uid, oldFlags, newFlags);
}
