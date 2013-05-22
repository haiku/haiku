/*
 * Copyright 2012-2013, Axel Dörfler, axeld@pinc-software.de.
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


class IMAPProtocol;


struct MessageToken {
	BString	mailboxName;
	uint32	uidValidity;
	uint32	uid;
};

// Additional local only message flags
enum FolderMessageFlags {
	kPartialMessage = 0x00010000,
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
								IMAPFolder(IMAPProtocol& protocol,
									const BString& mailboxName,
									const entry_ref& ref);
	virtual						~IMAPFolder();

			const BString&		MailboxName() const { return fMailboxName; }

			void				SetListener(FolderListener* listener);
			void				SetUIDValidity(uint32 uidValidity);

			uint32				LastUID() const { return fLastUID; }

			status_t			GetMessageEntryRef(uint32 uid,
									entry_ref& ref) const;
			uint32				MessageFlags(uint32 uid) const;

			status_t			StoreMessage(uint32 fetchFlags, BDataIO& stream,
									size_t& length, entry_ref& ref,
									BFile& file);
			void				MessageStored(entry_ref& ref, BFile& file,
									uint32 fetchFlags, uint32 uid,
									uint32 flags);

			status_t			StoreBody(uint32 uid, BDataIO& stream,
									size_t& length, entry_ref& ref,
									BFile& file);
			void				BodyStored(entry_ref& ref, BFile& file,
									uint32 uid);

			void				DeleteMessage(uint32 uid);
			void				SetMessageFlags(uint32 uid, uint32 flags);

	virtual void				MessageReceived(BMessage* message);

private:
			void				_InitializeFolderState();
			const MessageToken	_Token(uint32 uid) const;
			uint32				_ReadUniqueID(BNode& node);
			status_t			_WriteUniqueID(BNode& node, uint32 uid);
			uint32				_ReadFlags(BNode& node);
			status_t			_WriteFlags(BNode& node, uint32 flags);

			uint32				_ReadUInt32(BNode& node, const char* attribute);
			status_t			_WriteUInt32(BNode& node,
									const char* attribute, uint32 value);

			status_t			_WriteStream(BFile& file, BDataIO& stream,
									size_t& length);

private:
	typedef std::hash_map<uint32, uint32> UIDToFlagsMap;
	typedef std::hash_map<uint32, entry_ref> UIDToRefMap;

			IMAPProtocol&		fProtocol;
			const entry_ref		fRef;
			BString				fMailboxName;
			uint32				fUIDValidity;
			uint32				fLastUID;
			FolderListener*		fListener;
			UIDToRefMap			fRefMap;
			UIDToFlagsMap		fFlagsMap;
};


#endif	// IMAP_FOLDER_H
