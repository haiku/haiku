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

			void				AddMessageEntry(uint32 uid, uint32 flags,
									uint32 size);
			uint32				MessageFlags(uint32 uid) const;
			uint32				MessageSize(uint32 uid) const;

	// FolderListener interface
	virtual	uint32				MessageAdded(const MessageToken& fromToken,
									const entry_ref& ref);
	virtual void				MessageDeleted(const MessageToken& token);

	virtual void				MessageFlagsChanged(const MessageToken& token,
									const entry_ref& ref, uint32 oldFlags,
									uint32 newFlags);

protected:
	struct MessageFlagsAndSize {
		MessageFlagsAndSize(uint32 _flags, uint32 _size)
			:
			flags(_flags),
			size(_size)
		{
		}

		uint32	flags;
		uint32	size;
	};
	typedef std::hash_map<uint32, MessageFlagsAndSize> MessageEntryMap;

			IMAP::Protocol&		fProtocol;
			BString				fMailboxName;
			MessageEntryMap		fMessageEntries;
};


#endif	// IMAP_MAILBOX_H
