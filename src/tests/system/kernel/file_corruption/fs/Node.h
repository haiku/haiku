/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NODE_H
#define NODE_H


#include <sys/types.h>

#include <lock.h>

#include "checksumfs.h"


class Volume;


class Node {
public:
								Node(Volume* volume, uint64 blockIndex,
									const checksumfs_node& nodeData);
								Node(Volume* volume, uint64 blockIndex,
									mode_t mode);
	virtual						~Node();

	inline	Volume*				GetVolume() const	{ return fVolume; }
	inline	uint64				BlockIndex() const	{ return fBlockIndex; }
	inline	uint32				Mode() const		{ return fNode.mode; }
	inline	uint64				ParentDirectory() const;
	inline	uint32				HardLinks() const	{ return fNode.hardLinks; }
	inline	uint32				UID() const			{ return fNode.uid; }
	inline	uint32				GID() const			{ return fNode.gid; }
	inline	uint64				Size() const		{ return fNode.size; }
	inline	uint64				CreationTime() const;
	inline	uint64				ModificationTime() const;
	inline	uint64				ChangeTime() const;

	inline	bool				ReadLock();
	inline	void				ReadUnlock();
	inline	bool				WriteLock();
	inline	void				WriteUnlock();

			status_t			Flush();

private:
			void				_Init();

private:
			rw_lock				fLock;
			Volume*				fVolume;
			uint64				fBlockIndex;
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


#endif	// NODE_H
