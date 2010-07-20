/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "SymLink.h"

#include <string.h>

#include "Block.h"
#include "DebugSupport.h"


static const size_t kSymLinkDataOffset	= sizeof(checksumfs_node);
static const size_t kMaxSymLinkSize		= B_PAGE_SIZE - kSymLinkDataOffset;


SymLink::SymLink(Volume* volume, uint64 blockIndex,
	const checksumfs_node& nodeData)
	:
	Node(volume, blockIndex, nodeData)
{
}


SymLink::SymLink(Volume* volume, mode_t mode)
	:
	Node(volume, mode)
{
}


SymLink::~SymLink()
{
}


status_t
SymLink::ReadSymLink(char* buffer, size_t toRead, size_t& _bytesRead)
{
	uint64 size = Size();
	if (size > kMaxSymLinkSize)
		RETURN_ERROR(B_BAD_DATA);

	if (toRead > size)
		toRead = size;

	if (toRead == 0) {
		_bytesRead = 0;
		return B_OK;
	}

	// get the block
	Block block;
	if (!block.GetReadable(GetVolume(), BlockIndex()))
		RETURN_ERROR(B_ERROR);

	const char* data = (char*)block.Data() + kSymLinkDataOffset;
	memcpy(buffer, data, toRead);

	_bytesRead = toRead;
	return B_OK;
}


status_t
SymLink::WriteSymLink(const char* buffer, size_t toWrite,
	Transaction& transaction)
{
	uint64 size = Size();
	if (size > kMaxSymLinkSize)
		RETURN_ERROR(B_BAD_DATA);

	if (toWrite > kMaxSymLinkSize)
		RETURN_ERROR(B_NAME_TOO_LONG);

	if (toWrite == 0) {
		SetSize(0);
		return B_OK;
	}

	Block block;
	if (!block.GetWritable(GetVolume(), BlockIndex(), transaction))
		return B_ERROR;

	char* data = (char*)block.Data() + kSymLinkDataOffset;
	memcpy(data, buffer, toWrite);
	SetSize(toWrite);

	return B_OK;
}
