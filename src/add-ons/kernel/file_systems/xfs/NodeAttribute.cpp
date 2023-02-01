/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */


#include "NodeAttribute.h"

#include "VerifyHeader.h"


NodeAttribute::NodeAttribute(Inode* inode)
	:
	fInode(inode),
	fName(NULL),
	fLeafBuffer(NULL),
	fNodeBuffer(NULL)
{
	fLastEntryOffset	=	0;
	fLastNodeOffset		=	0;
	fNodeFlag			=	0;
}


NodeAttribute::~NodeAttribute()
{
	delete fMap;
	delete[] fLeafBuffer;
	delete[] fNodeBuffer;
}


status_t
NodeAttribute::Init()
{
	status_t status;

	fMap = new(std::nothrow) ExtentMapEntry;
	if (fMap == NULL)
		return B_NO_MEMORY;

	status = _FillMapEntry(0);

	if (status != B_OK)
		return status;

    // allocate memory for both Node and Leaf Buffer
	int len = fInode->DirBlockSize();
	fLeafBuffer = new(std::nothrow) char[len];
	if (fLeafBuffer == NULL)
		return B_NO_MEMORY;
	fNodeBuffer = new(std::nothrow) char[len];
	if (fNodeBuffer == NULL)
		return B_NO_MEMORY;

	// fill up Node buffer just one time
	status = _FillBuffer(fNodeBuffer, fMap->br_startblock);
	if (status != B_OK)
		return status;

	NodeHeader* header = NodeHeader::Create(fInode,fNodeBuffer);
	if (header == NULL)
		return B_NO_MEMORY;

	if (!VerifyHeader<NodeHeader>(header, fNodeBuffer, fInode, 0, fMap, ATTR_NODE)) {
		ERROR("Invalid data header");
		delete header;
		return B_BAD_VALUE;
	}
	delete header;

	return B_OK;
}


status_t
NodeAttribute::_FillMapEntry(xfs_extnum_t num)
{
	void* attributeFork = DIR_AFORK_PTR(fInode->Buffer(),
		fInode->CoreInodeSize(), fInode->ForkOffset());

	uint64* pointerToMap = (uint64*)(((char*)attributeFork + num * EXTENT_SIZE));
	uint64 firstHalf = pointerToMap[0];
	uint64 secondHalf = pointerToMap[1];
		//dividing the 128 bits into 2 parts.

	firstHalf = B_BENDIAN_TO_HOST_INT64(firstHalf);
	secondHalf = B_BENDIAN_TO_HOST_INT64(secondHalf);
	fMap->br_state = firstHalf >> 63;
	fMap->br_startoff = (firstHalf & MASK(63)) >> 9;
	fMap->br_startblock = ((firstHalf & MASK(9)) << 43) | (secondHalf >> 21);
	fMap->br_blockcount = secondHalf & MASK(21);

	return B_OK;
}


xfs_fsblock_t
NodeAttribute::_LogicalToFileSystemBlock(uint32 logicalBlock)
{
	xfs_extnum_t totalExtents = fInode->AttrExtentsCount();

	// If logicalBlock is lesser than or equal to ExtentsCount then
	// simply read and return extent block count.
	// else read last Map and add difference of logical block offset
	if (logicalBlock < (uint32)totalExtents) {
		_FillMapEntry(logicalBlock);
		return fMap->br_startblock;
	} else {
		_FillMapEntry(totalExtents - 1);
		return fMap->br_startblock + logicalBlock - fMap->br_startoff;
	}
}


status_t
NodeAttribute::_FillBuffer(char* buffer, xfs_fsblock_t block)
{
	int len = fInode->DirBlockSize();

	xfs_daddr_t readPos =
		fInode->FileSystemBlockToAddr(block);

	if (read_pos(fInode->GetVolume()->Device(), readPos, buffer, len) != len) {
		ERROR("Extent::FillBlockBuffer(): IO Error");
		return B_IO_ERROR;
	}

	return B_OK;
}


status_t
NodeAttribute::Open(const char* name, int openMode, attr_cookie** _cookie)
{
	TRACE("NodeAttribute::Open\n");

	size_t length = strlen(name);
	status_t status = Lookup(name, &length);
	if (status < B_OK)
		return status;

	attr_cookie* cookie = new(std::nothrow) attr_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	fName = name;

	// initialize the cookie
	strlcpy(cookie->name, fName, B_ATTR_NAME_LENGTH);
	cookie->open_mode = openMode;
	cookie->create = false;

	*_cookie = cookie;
	return B_OK;
}


status_t
NodeAttribute::Stat(attr_cookie* cookie, struct stat& stat)
{
	TRACE("NodeAttribute::Stat\n");

	fName = cookie->name;

	size_t namelength = strlen(fName);

	status_t status;

	// check if this attribute exists
	status = Lookup(fName, &namelength);

	if(status != B_OK)
		return status;

	// We have valid attribute entry to stat
	if (fLocalEntry != NULL) {
		uint16 valuelen = B_BENDIAN_TO_HOST_INT16(fLocalEntry->valuelen);
		stat.st_type = B_XATTR_TYPE;
		stat.st_size = valuelen + fLocalEntry->namelen;
	} else {
		uint32 valuelen = B_BENDIAN_TO_HOST_INT32(fRemoteEntry->valuelen);
		stat.st_type = B_XATTR_TYPE;
		stat.st_size = valuelen + fRemoteEntry->namelen;
	}

	return B_OK;
}


status_t
NodeAttribute::Read(attr_cookie* cookie, off_t pos, uint8* buffer, size_t* length)
{
	TRACE("NodeAttribute::Read\n");

	if (pos < 0)
		return B_BAD_VALUE;

	fName = cookie->name;

	size_t namelength = strlen(fName);

	status_t status;

	status = Lookup(fName, &namelength);

	if (status != B_OK)
		return status;

	uint32 lengthToRead = 0;

	if (fLocalEntry != NULL) {
		uint16 valuelen = B_BENDIAN_TO_HOST_INT16(fLocalEntry->valuelen);
		if (pos + *length > valuelen)
			lengthToRead = valuelen - pos;
		else
			lengthToRead = *length;

		char* ptrToOffset = (char*) fLocalEntry + sizeof(uint16)
		+ sizeof(uint8) + fLocalEntry->namelen;

		memcpy(buffer, ptrToOffset, lengthToRead);

		*length = lengthToRead;

		return B_OK;
	} else {
		uint32 valuelen = B_BENDIAN_TO_HOST_INT32(fRemoteEntry->valuelen);
		if (pos + *length > valuelen)
			lengthToRead = valuelen - pos;
		else
			lengthToRead = *length;

		// For remote value blocks, value is stored in seperate file system block
		uint32 blkno = B_BENDIAN_TO_HOST_INT32(fRemoteEntry->valueblk);

		xfs_daddr_t readPos =
			fInode->FileSystemBlockToAddr(blkno);

		if (fInode->Version() == 3)
			pos += sizeof(AttrRemoteHeader);

		readPos += pos;

		// TODO : Implement remote header checks for V5 file system
		if (read_pos(fInode->GetVolume()->Device(), readPos, buffer, lengthToRead)
			!= lengthToRead) {
			ERROR("Extent::FillBlockBuffer(): IO Error");
			return B_IO_ERROR;
		}

		*length = lengthToRead;

		return B_OK;
	}
}


status_t
NodeAttribute::GetNext(char* name, size_t* nameLength)
{
	TRACE("NodeAttribute::GetNext\n");

	NodeHeader* node = NodeHeader::Create(fInode, fNodeBuffer);
	NodeEntry* firstNodeEntry = (NodeEntry*)(fNodeBuffer + NodeHeader::Size(fInode));

	int totalNodeEntries = node->Count();

	delete node;

	for (int i = fLastNodeOffset; i < totalNodeEntries; i++) {

		NodeEntry* nodeEntry = (NodeEntry*)((char*)firstNodeEntry + i * sizeof(NodeEntry));
		fLastNodeOffset = i;

		// if we are at next node entry fill up leaf buffer
		if (fNodeFlag == 0) {
			// First see the leaf block from NodeEntry and logical block offset
			uint32 logicalBlock = B_BENDIAN_TO_HOST_INT32(nodeEntry->before);
			TRACE("Logical block : %d", logicalBlock);
			// Now calculate File system Block of This logical block
			xfs_fsblock_t block = _LogicalToFileSystemBlock(logicalBlock);
			_FillBuffer(fLeafBuffer, block);
			fNodeFlag = 1;
			fLastEntryOffset = 0;
		}

		AttrLeafHeader* header  = AttrLeafHeader::Create(fInode, fLeafBuffer);
		AttrLeafEntry* firstLeafEntry =
			(AttrLeafEntry*)(fLeafBuffer + AttrLeafHeader::Size(fInode));

		int totalEntries = header->Count();

		delete header;

		for (int j = fLastEntryOffset; j < totalEntries; j++) {

			AttrLeafEntry* entry = (AttrLeafEntry*)(
				(char*)firstLeafEntry + j * sizeof(AttrLeafEntry));

			uint32 offset = B_BENDIAN_TO_HOST_INT16(entry->nameidx);
			TRACE("offset:(%" B_PRIu16 ")\n", offset);
			fLastEntryOffset = j + 1;

			// First check if its local or remote value
			if (entry->flags & XFS_ATTR_LOCAL) {
				AttrLeafNameLocal* local  = (AttrLeafNameLocal*)(fLeafBuffer + offset);
				memcpy(name, local->nameval, local->namelen);
				name[local->namelen] = '\0';
				*nameLength = local->namelen + 1;
				TRACE("Entry found name : %s, namelength : %ld", name, *nameLength);
				return B_OK;
			} else {
				AttrLeafNameRemote* remote  = (AttrLeafNameRemote*)(fLeafBuffer + offset);
				memcpy(name, remote->name, remote->namelen);
				name[remote->namelen] = '\0';
				*nameLength = remote->namelen + 1;
				TRACE("Entry found name : %s, namelength : %ld", name, *nameLength);
				return B_OK;
			}
		}

		// we are now at next nodeEntry initialize it as 0
		fNodeFlag = 0;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
NodeAttribute::Lookup(const char* name, size_t* nameLength)
{
	TRACE("NodeAttribute::Lookup\n");
	uint32 hashValueOfRequest = hashfunction(name, *nameLength);
	TRACE("Hashval:(%" B_PRIu32 ")\n", hashValueOfRequest);

	// first we need to find leaf block which might contain our entry
	NodeHeader* node = NodeHeader::Create(fInode, fNodeBuffer);
	NodeEntry* nodeEntry = (NodeEntry*)(fNodeBuffer + NodeHeader::Size(fInode));

	int TotalNodeEntries = node->Count();
	int left = 0;
	int right = TotalNodeEntries - 1;

	delete node;

	hashLowerBound<NodeEntry>(nodeEntry, left, right, hashValueOfRequest);

	// We found our potential leaf block, now read leaf buffer
	// First see the leaf block from NodeEntry and logical block offset
	uint32 logicalBlock = B_BENDIAN_TO_HOST_INT32(nodeEntry[left].before);
	// Now calculate File system Block of This logical block
	xfs_fsblock_t block = _LogicalToFileSystemBlock(logicalBlock);
	_FillBuffer(fLeafBuffer, block);

	AttrLeafHeader* header  = AttrLeafHeader::Create(fInode,fLeafBuffer);
	AttrLeafEntry* entry = (AttrLeafEntry*)(fLeafBuffer + AttrLeafHeader::Size(fInode));

	int numberOfLeafEntries = header->Count();
	left = 0;
	right = numberOfLeafEntries - 1;

	delete header;

	hashLowerBound<AttrLeafEntry>(entry, left, right, hashValueOfRequest);

	while (B_BENDIAN_TO_HOST_INT32(entry[left].hashval)
			== hashValueOfRequest) {

		uint32 offset = B_BENDIAN_TO_HOST_INT16(entry[left].nameidx);
		TRACE("offset:(%" B_PRIu16 ")\n", offset);
		int status;

		// First check if its local or remote value
		if (entry[left].flags & XFS_ATTR_LOCAL) {
			AttrLeafNameLocal* local  = (AttrLeafNameLocal*)(fLeafBuffer + offset);
			char* PtrToOffset = (char*)local + sizeof(uint8) + sizeof(uint16);
			status = strncmp(name, PtrToOffset, *nameLength);
			if (status == 0) {
				fLocalEntry = local;
				fRemoteEntry = NULL;
				return B_OK;
			}
		} else {
			AttrLeafNameRemote* remote  = (AttrLeafNameRemote*)(fLeafBuffer + offset);
			char* PtrToOffset = (char*)remote + sizeof(uint8) + 2 * sizeof(uint32);
			status = strncmp(name, PtrToOffset, *nameLength);
			if (status == 0) {
				fRemoteEntry = remote;
				fLocalEntry = NULL;
				return B_OK;
			}
		}
		left++;
	}

	return B_ENTRY_NOT_FOUND;
}