/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef _XFS_VHEADER_H
#define _XFS_VHEADER_H


#include "BPlusTree.h"
#include "Checksum.h"
#include "Extent.h"
#include "Inode.h"
#include "LeafDirectory.h"
#include "Node.h"
#include "xfs_types.h"
#include "system_dependencies.h"


// Common template function to Verify all forms of header
template<class T>
bool VerifyHeader(T* header, char* buffer, Inode* inode,
	int howManyBlocksFurther, ExtentMapEntry* map, int8 WhichDirectory)
{
	if (header->Magic() != T::ExpectedMagic(WhichDirectory, inode)) {
		ERROR("Bad magic number");
		return false;
	}

	if (inode->Version() == 1 || inode->Version() == 2)
		return true;

	if (!xfs_verify_cksum(buffer, inode->DirBlockSize(), T::CRCOffset())) {
		ERROR("CRC is invalid");
		return false;
	}

	uint64 actualBlockToRead = inode->FileSystemBlockToAddr(map->br_startblock
		+ howManyBlocksFurther) / XFS_MIN_BLOCKSIZE;

	if (actualBlockToRead != header->Blockno()) {
		ERROR("Wrong Block number");
		return false;
	}

	if (!inode->GetVolume()->UuidEquals(header->Uuid())) {
		ERROR("UUID is incorrect");
		return false;
	}

	if (inode->ID() != header->Owner()) {
		ERROR("Wrong data owner");
		return false;
	}
	// TODO : Can we use it for Block Header as well?

	return true;
}

#endif