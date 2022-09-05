/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */


#include "Symlink.h"

#include "VerifyHeader.h"


Symlink::Symlink(Inode* inode)
	:
	fInode(inode),
	fSymlinkBuffer(NULL)
{
}


Symlink::~Symlink()
{
	delete fSymlinkBuffer;
}


status_t
Symlink::_FillMapEntry()
{
	void* pointerToMap = DIR_DFORK_PTR(fInode->Buffer(), fInode->CoreInodeSize());

	uint64 firstHalf = *((uint64*)pointerToMap);
	uint64 secondHalf = *((uint64*)pointerToMap + 1);
		//dividing the 128 bits into 2 parts.
	firstHalf = B_BENDIAN_TO_HOST_INT64(firstHalf);
	secondHalf = B_BENDIAN_TO_HOST_INT64(secondHalf);
	fMap.br_state = firstHalf >> 63;
	fMap.br_startoff = (firstHalf & MASK(63)) >> 9;
	fMap.br_startblock = ((firstHalf & MASK(9)) << 43) | (secondHalf >> 21);
	fMap.br_blockcount = secondHalf & MASK(21);
	TRACE("Extent::Init: startoff:(%" B_PRIu64 "), startblock:(%" B_PRIu64 "),"
		"blockcount:(%" B_PRIu64 "),state:(%" B_PRIu8 ")\n", fMap.br_startoff, fMap.br_startblock,
		fMap.br_blockcount, fMap.br_state);

	return B_OK;
}


status_t
Symlink::_FillBuffer()
{
	if (fMap.br_state != 0)
		return B_BAD_VALUE;

	int len = fInode->DirBlockSize();
	fSymlinkBuffer = new(std::nothrow) char[len];
	if (fSymlinkBuffer == NULL)
		return B_NO_MEMORY;

	xfs_daddr_t readPos =
		fInode->FileSystemBlockToAddr(fMap.br_startblock);

	if (read_pos(fInode->GetVolume()->Device(), readPos, fSymlinkBuffer, len)
		!= len) {
		ERROR("Extent::FillBlockBuffer(): IO Error");
		return B_IO_ERROR;
	}

	return B_OK;
}


status_t
Symlink::_ReadLocalLink(off_t pos, char* buffer, size_t* _length)
{
	// All symlinks contents are inside inode itself

	size_t lengthToRead = fInode->Size();

	if (*_length < lengthToRead)
		lengthToRead = *_length;

	char* offset = (char*)(DIR_DFORK_PTR(fInode->Buffer(), fInode->CoreInodeSize()));

	memcpy(buffer, offset, lengthToRead);

	*_length = lengthToRead;

	return B_OK;

}


status_t
Symlink::_ReadExtentLink(off_t pos, char* buffer, size_t* _length)
{
	status_t status;
	// First fill up extent, then Symlink block buffer
	status = _FillMapEntry();
	if (status != B_OK)
		return status;

	status = _FillBuffer();
	if (status != B_OK)
		return status;

	uint32 offset = 0;
	// If it is Version 5 xfs then we have Symlink header
	if (fInode->Version() == 3) {
		SymlinkHeader* header = (SymlinkHeader*)fSymlinkBuffer;
		if (!VerifyHeader<SymlinkHeader>(header, fSymlinkBuffer, fInode, 0, &fMap, 0)) {
			ERROR("Invalid data header");
			return B_BAD_VALUE;
		}
		offset += sizeof(SymlinkHeader);
	}

	size_t lengthToRead = fInode->Size();

	if (*_length < lengthToRead)
		lengthToRead = *_length;

	memcpy(buffer, fSymlinkBuffer + offset, lengthToRead);

	*_length = lengthToRead;

	return B_OK;
}


status_t
Symlink::ReadLink(off_t pos, char* buffer, size_t* _length)
{
	switch (fInode->Format()) {
		case XFS_DINODE_FMT_LOCAL:
			return _ReadLocalLink(pos, buffer, _length);
		case XFS_DINODE_FMT_EXTENTS:
			return _ReadExtentLink(pos, buffer, _length);
		default:
			return B_BAD_VALUE;
	}
}