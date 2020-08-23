/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Inode.h"
#include "BPlusTree.h"

void
xfs_inode_t::SwapEndian()
{
	di_magic = B_BENDIAN_TO_HOST_INT16(di_magic);
	di_mode = B_BENDIAN_TO_HOST_INT16(di_mode);
	di_onlink = B_BENDIAN_TO_HOST_INT16(di_onlink);
	di_uid = B_BENDIAN_TO_HOST_INT32(di_uid);
	di_gid = B_BENDIAN_TO_HOST_INT32(di_gid);
	di_nlink = B_BENDIAN_TO_HOST_INT32(di_nlink);
	di_projid = B_BENDIAN_TO_HOST_INT16(di_projid);
	di_flushiter = B_BENDIAN_TO_HOST_INT16(di_flushiter);
	di_atime.t_sec = B_BENDIAN_TO_HOST_INT32(di_atime.t_sec);
	di_atime.t_nsec = B_BENDIAN_TO_HOST_INT32(di_atime.t_nsec);
	di_mtime.t_sec = B_BENDIAN_TO_HOST_INT32(di_mtime.t_sec);
	di_mtime.t_nsec = B_BENDIAN_TO_HOST_INT32(di_mtime.t_nsec);
	di_ctime.t_sec = B_BENDIAN_TO_HOST_INT32(di_ctime.t_sec);
	di_ctime.t_nsec = B_BENDIAN_TO_HOST_INT32(di_ctime.t_nsec);
	di_size = B_BENDIAN_TO_HOST_INT64(di_size);
	di_nblocks =  B_BENDIAN_TO_HOST_INT64(di_nblocks);
	di_extsize = B_BENDIAN_TO_HOST_INT32(di_extsize);
	di_nextents = B_BENDIAN_TO_HOST_INT32(di_nextents);
	di_anextents = B_BENDIAN_TO_HOST_INT16(di_anextents);
	di_dmevmask = B_BENDIAN_TO_HOST_INT32(di_dmevmask);
	di_dmstate = B_BENDIAN_TO_HOST_INT16(di_dmstate);
	di_flags = B_BENDIAN_TO_HOST_INT16(di_flags);
	di_gen = B_BENDIAN_TO_HOST_INT32(di_gen);
	di_next_unlinked = B_BENDIAN_TO_HOST_INT32(di_next_unlinked);
}


uint8
xfs_inode_t::ForkOffset() const
{
	return di_forkoff;
}


xfs_rfsblock_t
xfs_inode_t::BlockCount() const
{
	return di_nblocks;
}


xfs_fsize_t
xfs_inode_t::Size() const
{
	return di_size;
}


mode_t
xfs_inode_t::Mode() const
{
	return di_mode;
}


uint32
xfs_inode_t::UserId() const
{
	return di_uid;
}


uint32
xfs_inode_t::GroupId() const
{
	return di_gid;
}


void
xfs_inode_t::GetModificationTime(struct timespec& stamp)
{
	stamp.tv_sec = di_mtime.t_sec;
	stamp.tv_nsec = di_mtime.t_nsec;
}


void
xfs_inode_t::GetAccessTime(struct timespec& stamp)
{
	stamp.tv_sec = di_atime.t_sec;
	stamp.tv_nsec = di_atime.t_nsec;
}


void
xfs_inode_t::GetChangeTime(struct timespec& stamp)
{
	stamp.tv_sec = di_ctime.t_sec;
	stamp.tv_nsec = di_ctime.t_nsec;
}


uint32
xfs_inode_t::NLink() const
{
	return di_nlink;
}


int8
xfs_inode_t::Version() const
{
	return di_version;
}


uint16
xfs_inode_t::Flags() const
{
	return di_flags;
}


int8
xfs_inode_t::Format() const
{
	return di_format;
}


xfs_extnum_t
xfs_inode_t::DataExtentsCount() const
{
	return di_nextents;
}


Inode::Inode(Volume* volume, xfs_ino_t id)
	:
	fVolume(volume),
	fId(id),
	fBuffer(NULL),
	fExtents(NULL)
{
}


status_t
Inode::Init()
{
	fNode = new(std::nothrow) xfs_inode_t;
	if (fNode == NULL)
		return B_NO_MEMORY;

	uint16 inodeSize = fVolume->InodeSize();
	fBuffer = new(std::nothrow) char[inodeSize];
	if (fBuffer == NULL)
		return B_NO_MEMORY;

	status_t status = GetFromDisk();
	if (status == B_OK) {
		if (fNode->di_magic == INODE_MAGIC) {
			TRACE("Init(): Inode successfully read.\n");
			status = B_OK;
		} else {
			TRACE("Init(): Inode wasn't read successfully!\n");
			status = B_BAD_VALUE;
		}
	}

	return status;
}


Inode::~Inode()
{
	delete fNode;
	delete[] fBuffer;
	delete[] fExtents;
}


bool
Inode::HasFileTypeField() const
{
	return fVolume->SuperBlockFeatures2() & XFS_SB_VERSION2_FTYPE;
}


status_t
Inode::CheckPermissions(int accessMode) const
{
	// you never have write access to a read-only volume
	if ((accessMode & W_OK) != 0 && fVolume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	return check_access_permissions(accessMode, Mode(),
		(uint32)fNode->GroupId(), (uint32)fNode->UserId());
}


void
Inode::UnWrapExtentFromWrappedEntry(uint64 wrappedExtent[2],
	ExtentMapEntry* entry)
{
		uint64 first = B_BENDIAN_TO_HOST_INT64(wrappedExtent[0]);
		uint64 second = B_BENDIAN_TO_HOST_INT64(wrappedExtent[1]);
		entry->br_state = first >> 63;
		entry->br_startoff = (first & MASK(63)) >> 9;
		entry->br_startblock = ((first & MASK(9)) << 43) | (second >> 21);
		entry->br_blockcount = second & MASK(21);
}


status_t
Inode::ReadExtentsFromExtentBasedInode()
{
	fExtents = new(std::nothrow) ExtentMapEntry[DataExtentsCount()];
	char* dataStart = (char*) DIR_DFORK_PTR(Buffer());
	uint64 wrappedExtent[2];
	for (int i = 0; i < DataExtentsCount(); i++) {
		wrappedExtent[0] = *(uint64*)(dataStart);
		wrappedExtent[1] = *(uint64*)(dataStart + sizeof(uint64));
		dataStart += 2 * sizeof(uint64);
		UnWrapExtentFromWrappedEntry(wrappedExtent, &fExtents[i]);
	}
	return B_OK;
}


size_t
Inode::MaxRecordsPossibleInTreeRoot()
{
	size_t lengthOfDataFork;
	if (ForkOffset() != 0)
		lengthOfDataFork = ForkOffset() << 3;
	else if(ForkOffset() == 0) {
		lengthOfDataFork = GetVolume()->InodeSize()
			- INODE_CORE_UNLINKED_SIZE;
	}

	lengthOfDataFork -= sizeof(BlockInDataFork);
	return lengthOfDataFork / (XFS_KEY_SIZE + XFS_PTR_SIZE);
}


size_t
Inode::GetPtrOffsetIntoRoot(int pos)
{
	size_t maxRecords = MaxRecordsPossibleInTreeRoot();
	return (sizeof(BlockInDataFork)
		+ maxRecords * XFS_KEY_SIZE + (pos - 1) * XFS_PTR_SIZE);
}


TreePointer*
Inode::GetPtrFromRoot(int pos)
{
	return (TreePointer*)
		((char*)DIR_DFORK_PTR(Buffer()) + GetPtrOffsetIntoRoot(pos));
}


size_t
Inode::MaxRecordsPossibleNode()
{
	size_t availableSpace = GetVolume()->BlockSize() - XFS_BTREE_LBLOCK_SIZE;
	return availableSpace / (XFS_KEY_SIZE + XFS_PTR_SIZE);
}


size_t
Inode::GetPtrOffsetIntoNode(int pos)
{
	size_t maxRecords = MaxRecordsPossibleNode();
	return XFS_BTREE_LBLOCK_SIZE + maxRecords * XFS_KEY_SIZE
		+ (pos - 1) * XFS_PTR_SIZE;
}


TreePointer*
Inode::GetPtrFromNode(int pos, void* buffer)
{
	size_t offsetIntoNode = GetPtrOffsetIntoNode(pos);
	return (TreePointer*)((char*)buffer + offsetIntoNode);
}


status_t
Inode::GetNodefromTree(uint16& levelsInTree, Volume* volume,
	size_t& len, size_t DirBlockSize, char* block) {

	char* node = new(std::nothrow) char[len];
	if (node == NULL)
		return B_NO_MEMORY;
		// This isn't for a directory block but for one of the tree nodes

	ArrayDeleter<char> nodeDeleter(node);

	TRACE("levels:(%d)\n", levelsInTree);
	TRACE("Numrecs:(%d)\n", fRoot->NumRecords());

	TreePointer* ptrToNode = GetPtrFromRoot(1);
	uint64 fileSystemBlockNo = B_BENDIAN_TO_HOST_INT64(*ptrToNode);
	uint64 readPos = FileSystemBlockToAddr(fileSystemBlockNo);
		// Go down the tree by taking the leftmost pointer to the first leaf
	while (levelsInTree != 1) {
		fileSystemBlockNo = B_BENDIAN_TO_HOST_INT64(*ptrToNode);
			// The fs block that contains node at next lower level. Now read.
		readPos = FileSystemBlockToAddr(fileSystemBlockNo);
		if (read_pos(volume->Device(), readPos, node, len) != len) {
			ERROR("Extent::FillBlockBuffer(): IO Error");
			return B_IO_ERROR;
		}
		LongBlock* curLongBlock = (LongBlock*)node;
		ASSERT(curLongBlock->Magic() == XFS_BMAP_MAGIC);
		ptrToNode = GetPtrFromNode(1, (void*)curLongBlock);
			// Get's the first pointer. This points to next node.
		levelsInTree--;
	}
	// Next level wil contain leaf nodes. Now Read Directory Buffer
	len = DirBlockSize;
	if (read_pos(volume->Device(), readPos, block, len)
		!= len) {
		ERROR("Extent::FillBlockBuffer(): IO Error");
		return B_IO_ERROR;
	}
	levelsInTree--;
	if (levelsInTree != 0)
		return B_BAD_VALUE;
	return B_OK;
}


status_t
Inode::ReadExtentsFromTreeInode()
{
	fExtents = new(std::nothrow) ExtentMapEntry[DataExtentsCount()];
	BlockInDataFork* root = new(std::nothrow) BlockInDataFork;
	if (root == NULL)
		return B_NO_MEMORY;
	memcpy((void*)root,
		DIR_DFORK_PTR(Buffer()), sizeof(BlockInDataFork));

	size_t maxRecords = MaxRecordsPossibleInTreeRoot();
	TRACE("Maxrecords: (%d)\n", maxRecords);

	uint16 levelsInTree = root->Levels();
	delete root;

	Volume* volume = GetVolume();
	size_t len = volume->BlockSize();

	len = DirBlockSize();
	char* block = new(std::nothrow) char[len];
	if (block == NULL)
		return B_NO_MEMORY;

	ArrayDeleter<char> blockDeleter(block);

	GetNodefromTree(levelsInTree, volume, len, DirBlockSize(), block);

	uint64 fileSystemBlockNo;
	uint64 wrappedExtent[2];
	int indexIntoExtents = 0;
	// We should be at the left most leaf node.
	// This could be a multilevel node type directory
	while (1) {
		// Run till you have leaf blocks to checkout
		char* leafBuffer = block;
		ASSERT(((LongBlock*)leafBuffer)->Magic() == XFS_BMAP_MAGIC);
		uint32 offset = sizeof(LongBlock);
		int numRecs = ((LongBlock*)leafBuffer)->NumRecs();

		for (int i = 0; i < numRecs; i++) {
			wrappedExtent[0] = *(uint64*)(leafBuffer + offset);
			wrappedExtent[1]
				= *(uint64*)(leafBuffer + offset + sizeof(uint64));
			offset += sizeof(ExtentMapUnwrap);
			UnWrapExtentFromWrappedEntry(wrappedExtent,
				&fExtents[indexIntoExtents]);
			indexIntoExtents++;
		}

		fileSystemBlockNo = ((LongBlock*)leafBuffer)->Right();
		TRACE("Next leaf is at: (%d)\n", fileSystemBlockNo);
		if (fileSystemBlockNo == -1)
			break;
		uint64 readPos = FileSystemBlockToAddr(fileSystemBlockNo);
		if (read_pos(volume->Device(), readPos, block, len)
				!= len) {
				ERROR("Extent::FillBlockBuffer(): IO Error");
				return B_IO_ERROR;
		}
	}
	TRACE("Total covered: (%d)\n", indexIntoExtents);
	return B_OK;
}


status_t
Inode::ReadExtents()
{
	if (fExtents != NULL)
		return B_OK;
	if (Format() == XFS_DINODE_FMT_BTREE)
		return ReadExtentsFromTreeInode();
	if (Format() == XFS_DINODE_FMT_EXTENTS)
		return ReadExtentsFromExtentBasedInode();
	return B_BAD_VALUE;
}


int
Inode::SearchMapInAllExtent(int blockNo)
{
	for (int i = 0; i < DataExtentsCount(); i++) {
		if (fExtents[i].br_startoff <= blockNo
			&& (blockNo <= fExtents[i].br_startoff
				+ fExtents[i].br_blockcount - 1)) {
			// Map found
			return i;
		}
	}
	return -1;
}


status_t
Inode::ReadAt(off_t pos, uint8* buffer, size_t* length)
{
	TRACE("Inode::ReadAt: pos:(%ld), *length:(%ld)\n", pos, *length);
	status_t status;
	if (fExtents == NULL) {
		status = ReadExtents();
		if (status != B_OK)
			return status;
	}

	// set/check boundaries for pos/length
	if (pos < 0) {
		ERROR("inode %" B_PRIdINO ": ReadAt failed(pos %" B_PRIdOFF
			", length %lu)\n", ID(), pos, length);
		return B_BAD_VALUE;
	}

	if (pos >= Size() || length == 0) {
		TRACE("inode %" B_PRIdINO ": ReadAt 0 (pos %" B_PRIdOFF
			", length %lu)\n", ID(), pos, length);
		*length = 0;
		return B_NO_ERROR;
	}

	uint32 blockNo = BLOCKNO_FROM_POSITION(pos, GetVolume());
	uint32 offsetIntoBlock = BLOCKOFFSET_FROM_POSITION(pos, this);

	size_t lengthOfBlock = BlockSize();
	char* block = new(std::nothrow) char[lengthOfBlock];
	if (block == NULL)
		return B_NO_MEMORY;

	ArrayDeleter<char> blockDeleter(block);

	size_t lengthRead = 0;
	size_t lengthLeftInFile;
	size_t lengthLeftInBlock;
	size_t lengthToRead;
	TRACE("What is blockLen:(%d)\n", lengthOfBlock);
	while (*length > 0) {
		TRACE("Inode::ReadAt: pos:(%ld), *length:(%ld)\n", pos, *length);
		// As long as you can read full blocks, read.
		lengthLeftInFile = Size() - pos;
		lengthLeftInBlock = lengthOfBlock - offsetIntoBlock;

		// We could be almost at the end of the file
		if (lengthLeftInFile <= lengthLeftInBlock)
			lengthToRead = lengthLeftInFile;
		else lengthToRead = lengthLeftInBlock;

		// But we might not be able to read all of the
		// data because of less buffer length
		if (lengthToRead > *length)
			lengthToRead = *length;

		int indexForMap = SearchMapInAllExtent(blockNo);
		if (indexForMap == -1)
			return B_BAD_VALUE;

		xfs_daddr_t readPos
			= FileSystemBlockToAddr(fExtents[indexForMap].br_startblock
				+ blockNo - fExtents[indexForMap].br_startoff);

		if (read_pos(GetVolume()->Device(), readPos, block, lengthOfBlock)
			!= lengthOfBlock) {
			ERROR("TreeDirectory::FillBlockBuffer(): IO Error");
			return B_IO_ERROR;
		}


		memcpy((void*) (buffer + lengthRead),
			(void*)(block + offsetIntoBlock), lengthToRead);

		pos += lengthToRead;
		*length -= lengthToRead;
		lengthRead += lengthToRead;
		blockNo = BLOCKNO_FROM_POSITION(pos, GetVolume());
		offsetIntoBlock = BLOCKOFFSET_FROM_POSITION(pos, this);
	}

	TRACE("lengthRead:(%d)\n", lengthRead);
	*length = lengthRead;
	return B_OK;
}


status_t
Inode::GetFromDisk()
{
	xfs_agnumber_t agNo = INO_TO_AGNO(fId, fVolume);
		// Get the AG number from the inode
	uint32 agRelativeInodeNo = INO_TO_AGINO(fId, fVolume->AgInodeBits());
		// AG relative ino #
	xfs_agblock_t agBlock = INO_TO_AGBLOCK(agRelativeInodeNo, fVolume);
		// AG relative block number
	xfs_off_t offset = INO_TO_BLOCKOFFSET(fId, fVolume);
		// Offset into the block1
	uint32 len = fVolume->InodeSize();
		// Inode len to read (size of inode)

	TRACE("AgNumber: (%d), AgRelativeIno: (%d), AgRelativeBlockNum: (%d),"
		"Offset: (%d), len: (%d)\n", agNo,
		agRelativeInodeNo, agBlock, offset, len);

	if (agNo > fVolume->AgCount()) {
		ERROR("Inode::GetFromDisk : AG Number more than number of AGs");
		return B_ENTRY_NOT_FOUND;
	}

	xfs_agblock_t numberOfBlocksInAg = fVolume->AgBlocks();

	xfs_fsblock_t blockToRead = FSBLOCKS_TO_BASICBLOCKS(fVolume->BlockLog(),
		((uint64)(agNo * numberOfBlocksInAg) + agBlock));

	xfs_daddr_t readPos = blockToRead * BASICBLOCKSIZE + offset * len;

	if (read_pos(fVolume->Device(), readPos, fBuffer, len) != len) {
		ERROR("Inode::Inode(): IO Error");
		return B_IO_ERROR;
	}

	memcpy(fNode, fBuffer, INODE_CORE_UNLINKED_SIZE);
	fNode->SwapEndian();

	return B_OK;
}


uint64
Inode::FileSystemBlockToAddr(uint64 block)
{
	xfs_agblock_t numberOfBlocksInAg = fVolume->AgBlocks();

	uint64 agNo = FSBLOCKS_TO_AGNO(block, fVolume);
	uint64 agBlockNo = FSBLOCKS_TO_AGBLOCKNO(block, fVolume);

	xfs_fsblock_t actualBlockToRead =
		FSBLOCKS_TO_BASICBLOCKS(fVolume->BlockLog(),
			((uint64)(agNo * numberOfBlocksInAg) + agBlockNo));
	TRACE("blockToRead:(%d)\n", actualBlockToRead);

	uint64 readPos = actualBlockToRead * (BASICBLOCKSIZE);
	return readPos;
}


/*
 * Basically take 4 characters at a time as long as you can, and xor with
 * previous hashVal after rotating 4 bits of hashVal. Likewise, continue
 * xor and rotating. This is quite a generic hash function.
*/
uint32
hashfunction(const char* name, int length)
{
	uint32 hashVal = 0;
	int lengthCovered = 0;
	int index = 0;
	if (length >= 4) {
		for (; index < length && (length - index) >= 4; index += 4)
		{
			lengthCovered += 4;
			hashVal = (name[index] << 21) ^ (name[index + 1] << 14)
				^ (name[index + 2] << 7) ^ (name[index + 3] << 0)
				^ ((hashVal << 28) | (hashVal >> 4));
		}
	}

	int leftToCover = length - lengthCovered;
	if (leftToCover == 3) {
		hashVal = (name[index] << 14) ^ (name[index + 1] << 7)
			^ (name[index + 2] << 0) ^ ((hashVal << 21) | (hashVal >> 11));
	}
	if (leftToCover == 2) {
		hashVal = (name[index] << 7) ^ (name[index + 1] << 0)
			^ ((hashVal << 14) | (hashVal >> (32 - 14)));
	}
	if (leftToCover == 1) {
		hashVal = (name[index] << 0)
			^ ((hashVal << 7) | (hashVal >> (32 - 7)));
	}

	return hashVal;
}
