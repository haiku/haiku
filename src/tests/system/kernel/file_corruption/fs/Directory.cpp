/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Directory.h"


Directory::Directory(Volume* volume, uint64 blockIndex,
	const checksumfs_node& nodeData)
	:
	Node(volume, blockIndex, nodeData)
{
}


Directory::Directory(Volume* volume, uint64 blockIndex, mode_t mode)
	:
	Node(volume, blockIndex, mode)
{
}


Directory::~Directory()
{
}
