/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Directory.h"
#include "Volume.h"


namespace BFS {

Directory::Directory(Volume &volume, block_run run)
	:
	fVolume(volume)
{
}


Directory::~Directory()
{
}


status_t 
Directory::InitCheck()
{
	return B_ERROR;
}


status_t 
Directory::Open(void **_cookie, int mode)
{
	return B_ERROR;
}


status_t 
Directory::Close(void *cookie)
{
	return B_ERROR;
}


Node *
Directory::Lookup(const char *name, bool traverseLinks)
{
	return NULL;
}


status_t 
Directory::GetNextNode(void *cookie, Node **_node)
{
	return B_ERROR;
}


status_t 
Directory::Rewind(void *cookie)
{
	return B_ERROR;
}


//	#pragma mark -


status_t 
bfs_inode::InitCheck(Volume *volume)
{
	if (Flags() & INODE_NOT_READY) {
		// the other fields may not yet contain valid values
		return B_BUSY;
	}

	if (Magic1() != INODE_MAGIC1
		|| !(Flags() & INODE_IN_USE)
		|| inode_num.Length() != 1
		// matches inode size?
		|| (uint32)InodeSize() != volume->InodeSize()
		// parent resides on disk?
		|| parent.AllocationGroup() > int32(volume->AllocationGroups())
		|| parent.AllocationGroup() < 0
		|| parent.Start() > (1L << volume->AllocationGroupShift())
		|| parent.Length() != 1
		// attributes, too?
		|| attributes.AllocationGroup() > int32(volume->AllocationGroups())
		|| attributes.AllocationGroup() < 0
		|| attributes.Start() > (1L << volume->AllocationGroupShift()))
		return B_BAD_DATA;

	// ToDo: Add some tests to check the integrity of the other stuff here,
	// especially for the data_stream!

	return B_OK;
}

}	// namespace BFS
