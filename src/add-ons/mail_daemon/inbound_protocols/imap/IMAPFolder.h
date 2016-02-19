/*
 * Copyright 2012-2016, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_FOLDER_H
#define IMAP_FOLDER_H


#include <hash_map>
#include <hash_set>

#include <sys/stat.h>

#include <Entry.h>
#include <Node.h>
#include <Handler.h>
#include <String.h>

#include <locks.h>

#include "Commands.h"


class BFile;
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

			status_t			Init();

			const BString&		MailboxName() const { return fMailboxName; }
			ino_t				NodeID() const { return fNodeID; }

			void				SetListener(FolderListener* listener);
			void				SetUIDValidity(uint32 uidValidity);

			uint32				LastUID() const { return fLastUID; }

			status_t			GetMessageEntryRef(uint32 uid, entry_ref& ref);
			status_t			GetMessageUID(const entry_ref& ref,
									uint32& uid) const;
			uint32				MessageFlags(uint32 uid);

			void				SyncMessageFlags(uint32 uid,
									uint32 mailboxFlags);
			void				MessageEntriesFetched();

			status_t			StoreMessage(uint32 fetchFlags, BDataIO& stream,
									size_t& length, entry_ref& ref,
									BFile& file);
			void				MessageStored(entry_ref& ref, BFile& file,
									uint32 fetchFlags, uint32 uid,
									uint32 flags);

			void				RegisterPendingBodies(
									IMAP::MessageUIDList& uids,
									const BMessenger* replyTo);
			status_t			StoreBody(uint32 uid, BDataIO& stream,
									size_t& length, entry_ref& ref,
									BFile& file);
			void				BodyStored(entry_ref& ref, BFile& file,
									uint32 uid);
			void				StoringBodyFailed(const entry_ref& ref,
									uint32 uid, status_t error);

			void				DeleteMessage(uint32 uid);

	virtual void				MessageReceived(BMessage* message);

private:
			void				_WaitForFolderState();
			void				_InitializeFolderState();
			void				_ReadFolderState();
	static	status_t			_ReadFolderState(void* self);

			const MessageToken	_Token(uint32 uid) const;
			void				_NotifyStoredBody(const entry_ref& ref,
									uint32 uid, status_t status);
			status_t			_GetMessageEntryRef(uint32 uid,
									entry_ref& ref) const;
			status_t			_DeleteLocalMessage(uint32 uid);

			void				_IMAPToMailFlags(uint32 flags,
									BMessage& attributes);
			status_t			_MailToIMAPFlags(BNode& node, uint32& flags);
			void				_TestMessageFlags(uint32 previousFlags,
									uint32 mailboxFlags, uint32 currentFlags,
									uint32 testFlag, uint32& nextFlags);

			uint32				_ReadUniqueID(BNode& node) const;
			status_t			_WriteUniqueID(BNode& node, uint32 uid) const;
			uint32				_ReadUniqueIDValidity(BNode& node) const;
			status_t			_WriteUniqueIDValidity(BNode& node) const;
			uint32				_ReadFlags(BNode& node) const;
			status_t			_WriteFlags(BNode& node, uint32 flags) const;

			uint32				_ReadUInt32(BNode& node,
									const char* attribute) const;
			status_t			_WriteUInt32(BNode& node,
									const char* attribute, uint32 value) const;

			status_t			_WriteStream(BFile& file, BDataIO& stream,
									size_t& length) const;

private:
	typedef std::vector<BMessenger> MessengerList;
#if __GNUC__ >= 4
	typedef __gnu_cxx::hash_map<uint32, uint32> UIDToFlagsMap;
	typedef __gnu_cxx::hash_map<uint32, entry_ref> UIDToRefMap;
	typedef __gnu_cxx::hash_map<uint32, MessengerList> MessengerMap;
	typedef __gnu_cxx::hash_set<uint32> UIDSet;
#else
	typedef std::hash_map<uint32, uint32> UIDToFlagsMap;
	typedef std::hash_map<uint32, entry_ref> UIDToRefMap;
	typedef std::hash_map<uint32, MessengerList> MessengerMap;
	typedef std::hash_set<uint32> UIDSet;
#endif

			IMAPProtocol&		fProtocol;
			const entry_ref		fRef;
			BString				fMailboxName;
			ino_t				fNodeID;
			uint32				fUIDValidity;
			uint32				fLastUID;
			FolderListener*		fListener;
			mutex				fLock;
			mutex				fFolderStateLock;
			thread_id			fReadFolderStateThread;
			bool				fFolderStateInitialized;
			bool				fQuitFolderState;
			UIDToRefMap			fRefMap;
			UIDToFlagsMap		fFlagsMap;
			UIDSet				fSynchronizedUIDsSet;
			UIDToFlagsMap		fPendingFlagsMap;
			MessengerMap		fPendingBodies;
};


#endif	// IMAP_FOLDER_H
