/*
 * Copyright 2013-2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_MAILBOX_H
#define IMAP_MAILBOX_H


#include "Commands.h"
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

			void				AddMessageEntry(uint32 index, uint32 uid,
									uint32 flags, uint32 size);
			void				RemoveMessageEntry(uint32 index);

			uint32				UIDForIndex(uint32 index) const;
			uint32				MessageFlags(uint32 uid) const;
			uint32				MessageSize(uint32 uid) const;
			uint32				CountMessages() const { return fUIDs.size(); }

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
#if __GNUC__ >= 4
	typedef __gnu_cxx::hash_map<uint32, MessageFlagsAndSize> MessageEntryMap;
#else
	typedef std::hash_map<uint32, MessageFlagsAndSize> MessageEntryMap;
#endif

			IMAP::Protocol&		fProtocol;
			BString				fMailboxName;
			MessageEntryMap		fMessageEntries;
			IMAP::MessageUIDList fUIDs;
};


#endif	// IMAP_MAILBOX_H
