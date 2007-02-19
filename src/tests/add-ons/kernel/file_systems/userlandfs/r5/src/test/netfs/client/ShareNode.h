// Node.h

#ifndef NET_FS_SHARE_NODE_H
#define NET_FS_SHARE_NODE_H

#include "DLList.h"
#include "Node.h"
#include "NodeInfo.h"
#include "Referencable.h"
#include "SLList.h"

class ShareAttrDir;
class ShareDir;
class ShareNode;

static const int32 kRemoteShareDirIteratorCapacity = 32;

// ShareDirEntry
class ShareDirEntry : public Referencable,
	public DLListLinkImpl<ShareDirEntry>, public SLListLinkImpl<ShareDirEntry> {
public:
								ShareDirEntry(ShareDir* directory,
									const char* name, ShareNode* node);
								~ShareDirEntry();

			status_t			InitCheck() const;

			ShareDir*			GetDirectory() const;
			const char*			GetName() const;
			ShareNode*			GetNode() const;

			void				SetRevision(int64 revision);
			int64				GetRevision() const;

			bool				IsActualEntry() const;

private:
			ShareDir*			fDirectory;
			String				fName;
			ShareNode*			fNode;
			int64				fRevision;
};

// ShareNode
class ShareNode : public Node {
public:
								ShareNode(Volume* volume, vnode_id id,
									const NodeInfo* nodeInfo);
	virtual						~ShareNode();

			const NodeInfo&		GetNodeInfo() const;

			NodeID				GetRemoteID() const;

			void				Update(const NodeInfo& nodeInfo);

			void				AddReferringEntry(ShareDirEntry* entry);
			void				RemoveReferringEntry(ShareDirEntry* entry);
			ShareDirEntry*		GetFirstReferringEntry() const;
			ShareDirEntry*		GetNextReferringEntry(
									ShareDirEntry* entry) const;
			ShareDirEntry*		GetActualReferringEntry() const;

			void				SetAttrDir(ShareAttrDir* attrDir);
			ShareAttrDir*		GetAttrDir() const;

private:
			NodeInfo			fInfo;
			SLList<ShareDirEntry> fReferringEntries;
			ShareAttrDir*		fAttrDir;
};

// ShareDirIterator
class ShareDirIterator {
public:
								ShareDirIterator();
	virtual						~ShareDirIterator();

	virtual	ShareDirEntry*		GetCurrentEntry() const = 0;
	virtual	void				NextEntry() = 0;
	virtual	void				Rewind() = 0;
	virtual	bool				IsDone() const = 0;
};

// LocalShareDirIterator
class LocalShareDirIterator : public ShareDirIterator,
	public DLListLinkImpl<LocalShareDirIterator> {
public:
								LocalShareDirIterator();
								~LocalShareDirIterator();

			void				SetDirectory(ShareDir* directory);

	virtual	ShareDirEntry*		GetCurrentEntry() const;
	virtual	void				NextEntry();
	virtual	void				Rewind();
	virtual	bool				IsDone() const;

private:
			ShareDir*			fDirectory;
			ShareDirEntry*		fCurrentEntry;
};

// RemoteShareDirIterator
class RemoteShareDirIterator : public ShareDirIterator {
public:
								RemoteShareDirIterator();
								~RemoteShareDirIterator();

	virtual	ShareDirEntry*		GetCurrentEntry() const;
	virtual	void				NextEntry();
	virtual	void				Rewind();
	virtual	bool				IsDone() const;

			int32				GetCapacity() const;

			void				SetCookie(int32 cookie);
			int32				GetCookie() const;

			void				Clear();
			bool				AddEntry(ShareDirEntry* entry);

			void				SetRevision(int64 revision);
			int64				GetRevision() const;

			void				SetDone(bool done);

			bool				GetRewind() const;

private:
			int32				fCookie;
			ShareDirEntry*		fEntries[kRemoteShareDirIteratorCapacity];
			int32				fCapacity;
			int32				fCount;
			int32				fIndex;
			int64				fRevision;
			bool				fDone;
			bool				fRewind;
};

// ShareDir
class ShareDir : public ShareNode {
public:
								ShareDir(Volume* volume, vnode_id id,
									const NodeInfo* nodeInfo);
	virtual						~ShareDir();

			void				UpdateEntryCreatedEventRevision(int64 revision);
			int64				GetEntryCreatedEventRevision() const;

			void				UpdateEntryRemovedEventRevision(int64 revision);
			int64				GetEntryRemovedEventRevision() const;

			void				SetComplete(bool complete);
			bool				IsComplete() const;

			void				AddEntry(ShareDirEntry* entry);
			void				RemoveEntry(ShareDirEntry* entry);

			ShareDirEntry*		GetFirstEntry() const;
			ShareDirEntry*		GetNextEntry(ShareDirEntry* entry) const;

			void				AddDirIterator(LocalShareDirIterator* iterator);
			void				RemoveDirIterator(
									LocalShareDirIterator* iterator);

private:
			DLList<ShareDirEntry> fEntries;
			DLList<LocalShareDirIterator> fIterators;
			int64				fEntryCreatedEventRevision;
									// The revision of the latest "created" or
									// "moved" (destination entry) event for
									// which the entry could not be created
									// (for whatever reason -- missing entry
									// info in the event request most likely).
									// To be compared with remote dir iterator
									// revisions to set fIsComplete.
			int64				fEntryRemovedEventRevision;
									// The revision of the latest "removed" or
									// "moved" (source entry) event. To be
									// compared with entry info revisions
									// returned by client request replies.
									// Such an info is to be considered invalid,
									// if its revision is less than this
									// revision, and must be reloaded from the
									// server.
			bool				fIsComplete;
};

#endif	// NET_FS_SHARE_NODE_H
