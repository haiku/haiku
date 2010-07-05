/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Node.h"

#include <string.h>
#include <sys/time.h>

#include "Block.h"


Node::Node(Volume* volume, uint64 blockIndex, const checksumfs_node& nodeData)
	:
	fVolume(volume),
	fBlockIndex(blockIndex),
	fNode(nodeData),
	fNodeDataDirty(false)
{
	_Init();
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

	// set the times
	timeval time;
	gettimeofday(&time, NULL);

	fNode.creationTime = (uint64)time.tv_sec * 1000000000
		+ (uint64)time.tv_usec * 1000;
	fNode.modificationTime = fNode.creationTime;
	fNode.changeTime = fNode.creationTime;
}


Node::~Node()
{
	rw_lock_destroy(&fLock);
}


status_t
Node::Flush()
{
	if (!fNodeDataDirty)
		return B_OK;

	Block block;
	if (!block.GetWritable(fVolume, fBlockIndex))
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
