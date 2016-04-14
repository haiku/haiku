/*
 * Copyright 2003-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "amiga_ffs.h"

#include <boot/partitions.h>
#include <boot/platform.h>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>


using namespace FFS;


class BCPLString {
	public:
		uint8 Length() { return fLength; }
		const char *String() { return (const char *)(&fLength + 1); }
		int32 CopyTo(char *name, size_t size);

	private:
		uint8	fLength;
};


int32
BCPLString::CopyTo(char *name, size_t size)
{
	int32 length = size - 1 > Length() ? Length() : size - 1;

	memcpy(name, String(), length);
	name[length] = '\0';

	return length;
}


//	#pragma mark -


status_t
BaseBlock::GetNameBackOffset(int32 offset, char *name, size_t size) const
{
	BCPLString *string = (BCPLString *)&fData[fSize - offset];
	string->CopyTo(name, size);

	return B_OK;
}


status_t
BaseBlock::ValidateCheckSum() const
{
	if (fData == NULL)
		return B_NO_INIT;

	int32 sum = 0;
	for (int32 index = 0; index < fSize; index++) {
		sum += Offset(index);
	}

	return sum == 0 ? B_OK : B_BAD_DATA;
}


//	#pragma mark -


char
DirectoryBlock::ToUpperChar(int32 type, char c) const
{
	// Taken from Ralph Babel's "The Amiga Guru Book" (1993), section 15.3.4.3

	if (type == DT_AMIGA_OFS || type == DT_AMIGA_FFS)
		return c >= 'a' && c <= 'z' ? c + ('A' - 'a') : c;

	return (c >= '\340' && c <= '\376' && c != '\367')
		|| (c >= 'a' && c <= 'z') ? c + ('A' - 'a') : c;
}


int32
DirectoryBlock::HashIndexFor(int32 type, const char *name) const
{
	int32 hash = strlen(name);

	while (name[0]) {
		hash = (hash * 13 + ToUpperChar(type, name[0])) & 0x7ff;
		name++;
	}

	return hash % HashSize();
}


int32
DirectoryBlock::HashValueAt(int32 index) const
{
	return index >= HashSize() ? -1 : (int32)B_BENDIAN_TO_HOST_INT32(HashTable()[index]);
}


int32
DirectoryBlock::FirstHashValue(int32 &index) const
{
	index = -1;
	return NextHashValue(index);
}


int32
DirectoryBlock::NextHashValue(int32 &index) const
{
	index++;

	int32 value;
	while ((value = HashValueAt(index)) == 0) {
		if (++index >= HashSize())
			return -1;
	}

	return value;
}


//	#pragma mark -


HashIterator::HashIterator(int32 device, DirectoryBlock &directory)
	:
	fDirectory(directory),
	fDevice(device),
	fCurrent(0),
	fBlock(-1)
{
	fData = (int32 *)malloc(directory.BlockSize());
	fNode.SetTo(directory.BlockData(), directory.BlockSize());
}


HashIterator::~HashIterator()
{
	free(fData);
}


status_t
HashIterator::InitCheck()
{
	return fData != NULL ? B_OK : B_NO_MEMORY;
}


void
HashIterator::Goto(int32 index)
{
	fCurrent = index;
	fBlock = fDirectory.HashValueAt(index);
}


NodeBlock *
HashIterator::GetNext(int32 &block)
{
	if (fBlock == -1) {
		// first entry
		fBlock = fDirectory.FirstHashValue(fCurrent);
	} else if (fBlock == 0) {
		fBlock = fDirectory.NextHashValue(fCurrent);
	}

	if (fBlock == -1)
		return NULL;

	block = fBlock;

	if (read_pos(fDevice, fBlock * fNode.BlockSize(), fData, fNode.BlockSize()) < B_OK)
		return NULL;

	fNode.SetTo(fData);
	if (fNode.ValidateCheckSum() != B_OK) {
		dprintf("block at %" B_PRId32 " bad checksum.\n", fBlock);
		return NULL;
	}

	fBlock = fNode.HashChain();

	return &fNode;
}


void
HashIterator::Rewind()
{
	fCurrent = 0;
	fBlock = -1;
}


//	#pragma mark -


status_t
FFS::get_root_block(int fDevice, char *buffer, int32 blockSize, off_t partitionSize)
{
	// calculate root block position (it depends on the block size)

	// ToDo: get the number of reserved blocks out of the disk_environment structure??
	//		(from the amiga_rdb module)
	int32 reservedBlocks = 2;
	off_t offset = (((partitionSize / blockSize) - 1 - reservedBlocks) / 2) + reservedBlocks;
		// ToDo: this calculation might be incorrect for certain cases.

	if (read_pos(fDevice, offset * blockSize, buffer, blockSize) < B_OK)
		return B_ERROR;

	RootBlock root(buffer, blockSize);
	if (root.ValidateCheckSum() < B_OK)
		return B_BAD_DATA;

	//printf("primary = %ld, secondary = %ld\n", root.PrimaryType(), root.SecondaryType());
	if (!root.IsRootBlock())
		return B_BAD_TYPE;

	return B_OK;
}

