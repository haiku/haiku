/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Node.h"

#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "Block.h"


static inline uint64
current_time_nanos()
{
	timeval time;
	gettimeofday(&time, NULL);

	return (uint64)time.tv_sec * 1000000000 + (uint64)time.tv_usec * 1000;
}


Node::Node(Volume* volume, uint64 blockIndex, const checksumfs_node& nodeData)
	:
	fVolume(volume),
	fBlockIndex(blockIndex),
	fNode(nodeData),
	fNodeDataDirty(false)
{
	_Init();

	fAccessedTime = ModificationTime();
}


Node::Node(Volume* volume, uint64 blockIndex, mode_t mode)
	:
	fVolume(volume),
	fBlockIndex(blockIndex),
	fNodeDataDirty(true)
{
	_Init();

	memset(&fNode, 0, sizeof(fNode));

	fNode.mode = mode;

	// set user/group
	fNode.uid = geteuid();
	fNode.gid = getegid();

	// set the times
	timeval time;
	gettimeofday(&time, NULL);

	fAccessedTime = current_time_nanos();
	fNode.creationTime = fAccessedTime;
	fNode.modificationTime = fAccessedTime;
	fNode.changeTime = fAccessedTime;
}


Node::~Node()
{
	rw_lock_destroy(&fLock);
}


void
Node::SetParentDirectory(uint32 blockIndex)
{
	fNode.parentDirectory = blockIndex;
	fNodeDataDirty = true;
}


void
Node::SetHardLinks(uint32 value)
{
	fNode.hardLinks = value;
	fNodeDataDirty = true;
}


void
Node::SetSize(uint64 size)
{
	fNode.size = size;
}


void
Node::Touched(int32 mode)
{
	fAccessedTime = current_time_nanos();

	switch (mode) {
		default:
		case NODE_MODIFIED:
			fNode.modificationTime = fAccessedTime;
		case NODE_STAT_CHANGED:
			fNode.changeTime = fAccessedTime;
		case NODE_ACCESSED:
			break;
	}
}


void
Node::RevertNodeData(const checksumfs_node& nodeData)
{
	fNode = nodeData;
	fNodeDataDirty = false;
}


status_t
Node::Flush(Transaction& transaction)
{
	if (!fNodeDataDirty)
		return B_OK;

	Block block;
	if (!block.GetWritable(fVolume, fBlockIndex, transaction))
		return B_ERROR;

	memcpy(block.Data(), &fNode, sizeof(fNode));

	fNodeDataDirty = false;
	return B_OK;
}


void
Node::_Init()
{
	rw_lock_init(&fLock, "checkfs node");
}
