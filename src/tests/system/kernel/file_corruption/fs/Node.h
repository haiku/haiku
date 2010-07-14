/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NODE_H
#define NODE_H


#include <sys/types.h>

#include <AutoLocker.h>

#include <lock.h>

#include "checksumfs.h"


class Transaction;
class Volume;


enum {
	NODE_ACCESSED,
	NODE_STAT_CHANGED,
	NODE_MODIFIED
};


class Node {
public:
								Node(Volume* volume, uint64 blockIndex,
									const checksumfs_node& nodeData);
								Node(Volume* volume, uint64 blockIndex,
									mode_t mode);
	virtual						~Node();

	virtual	status_t			InitForVFS();
	virtual	status_t			DeletingNode(Transaction& transaction);

	virtual	status_t			Resize(uint64 newSize, bool fillWithZeroes,
									Transaction& transaction);
	virtual	status_t			Read(off_t pos, void* buffer, size_t size,
									size_t& _bytesRead);
	virtual	status_t			Write(off_t pos, const void* buffer,
									size_t size, size_t& _bytesWritten);

	inline	const checksumfs_node& NodeData() const	{ return fNode; }
	inline	Volume*				GetVolume() const	{ return fVolume; }
	inline	uint64				BlockIndex() const	{ return fBlockIndex; }
	inline	uint32				Mode() const		{ return fNode.mode; }
	inline	uint64				ParentDirectory() const;
	inline	uint32				HardLinks() const	{ return fNode.hardLinks; }
	inline	uint32				UID() const			{ return fNode.uid; }
	inline	uint32				GID() const			{ return fNode.gid; }
	inline	uint64				Size() const		{ return fNode.size; }
	inline	uint64				AccessedTime() const { return fAccessedTime; }
	inline	uint64				CreationTime() const;
	inline	uint64				ModificationTime() const;
	inline	uint64				ChangeTime() const;

			void				SetParentDirectory(uint32 blockIndex);
			void				SetHardLinks(uint32 value);
			void				SetUID(uint32 uid);
			void				SetGID(uint32 gid);
			void				SetSize(uint64 size);
			void				SetAccessedTime(uint64 time);
			void				SetCreationTime(uint64 time);
			void				SetModificationTime(uint64 time);
			void				SetChangeTime(uint64 time);

			void				Touched(int32 mode);

	inline	bool				ReadLock();
	inline	void				ReadUnlock();
	inline	bool				WriteLock();
	inline	void				WriteUnlock();

	virtual	void				RevertNodeData(const checksumfs_node& nodeData);

			status_t			Flush(Transaction& transaction);

private:
			void				_Init();

private:
			rw_lock				fLock;
			Volume*				fVolume;
			uint64				fBlockIndex;
			uint64				fAccessedTime;
			checksumfs_node		fNode;
			bool				fNodeDataDirty;
};


uint64
Node::ParentDirectory() const
{
	return fNode.parentDirectory;
}


uint64
Node::CreationTime() const
{
	return fNode.creationTime;
}


uint64
Node::ModificationTime() const
{
	return fNode.modificationTime;
}


uint64
Node::ChangeTime() const
{
	return fNode.changeTime;
}


bool
Node::ReadLock()
{
	return rw_lock_read_lock(&fLock) == B_OK;
}


void
Node::ReadUnlock()
{
	rw_lock_read_unlock(&fLock);
}


bool
Node::WriteLock()
{
	return rw_lock_write_lock(&fLock) == B_OK;
}


void
Node::WriteUnlock()
{
	rw_lock_write_unlock(&fLock);
}


typedef AutoLocker<Node, AutoLockerReadLocking<Node> > NodeReadLocker;
typedef AutoLocker<Node, AutoLockerWriteLocking<Node> > NodeWriteLocker;


#endif	// NODE_H
