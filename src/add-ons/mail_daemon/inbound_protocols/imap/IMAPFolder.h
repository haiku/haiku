/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_FOLDER_H
#define IMAP_FOLDER_H


#include <hash_map>
#include <sys/stat.h>

#include <Entry.h>
#include <Node.h>
#include <Handler.h>
#include <String.h>


struct MessageToken {
	BString	mailboxName;
	uint32	uidValidity;
	uint32	uid;
};


class FolderListener {
public:
	virtual	uint32				MessageAdded(const MessageToken& fromToken,
									const entry_ref& ref) = 0;
	virtual void				MessageDeleted(const MessageToken& token) = 0;

	virtual void				MessageFlagsChanged(const MessageToken& token,
									const entry_ref& ref, uint32 oldFlags,
									uint32 newFlags) = 0;
};


class IMAPFolder : public BHandler {
public:
								IMAPFolder(const BString& mailboxName,
									const entry_ref& ref);
	virtual						~IMAPFolder();

			const BString&		MailboxName() const { return fMailboxName; }

			void				SetListener(FolderListener* listener);
			void				SetUIDValidity(uint32 uidValidity);

			status_t			StoreTemporaryMessage(uint32 fetchFlags,
									BDataIO& stream, size_t length,
									entry_ref& ref);
			void				StoreMessage(uint32 fetchFlags, uint32 uid,
									uint32 flags, entry_ref& ref);
			status_t			StoreMessage(uint32 fetchFlags, uint32 uid,
									BDataIO& stream, size_t length);
			void				DeleteMessage(uint32 uid);
			void				SetMessageFlags(uint32 uid, uint32 flags);

	virtual void				MessageReceived(BMessage* message);

private:
			void				_InitializeFolderState();
			const MessageToken	_Token(uint32 uid) const;
			uint32				_ReadUniqueID(BNode& node);
			uint32				_ReadFlags(BNode& node);

private:
	typedef std::hash_map<uint32, uint32> UIDToFlagsMap;
	typedef std::hash_map<uint32, entry_ref> UIDToRefMap;

			const entry_ref		fRef;
			BString				fMailboxName;
			uint32				fUIDValidity;
			FolderListener*		fListener;
			UIDToRefMap			fRefMap;
			UIDToFlagsMap		fUIDMap;
};


#endif	// IMAP_FOLDER_H
