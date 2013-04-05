/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_MAILBOX_H
#define IMAP_MAILBOX_H


#include "IMAPFolder.h"


namespace IMAP {
	class Protocol;
};


class IMAPMailbox : public FolderListener {
public:
								IMAPMailbox(IMAP::Protocol& protocol,
									const BString& mailboxName);
	virtual						~IMAPMailbox();

			const BString&		MailboxName() const { return fMailboxName; }

	// FolderListener interface
	virtual	uint32				MessageAdded(const MessageToken& fromToken,
									const entry_ref& ref);
	virtual void				MessageDeleted(const MessageToken& token);

	virtual void				MessageFlagsChanged(const MessageToken& token,
									const entry_ref& ref, uint32 oldFlags,
									uint32 newFlags);

protected:
			IMAP::Protocol&		fProtocol;
			BString				fMailboxName;
};


#endif	// IMAP_MAILBOX_H
