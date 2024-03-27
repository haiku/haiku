/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Inode.h"

#include "BPlusTree.h"
#include "VerifyHeader.h"


void
Inode::SwapEndian()
{
	fNode->di_magic = B_BENDIAN_TO_HOST_INT16(fNode->di_magic);
	fNode->di_mode = B_BENDIAN_TO_HOST_INT16(fNode->di_mode);
	fNode->di_onlink = B_BENDIAN_TO_HOST_INT16(fNode->di_onlink);
	fNode->di_uid = B_BENDIAN_TO_HOST_INT32(fNode->di_uid);
	fNode->di_gid = B_BENDIAN_TO_HOST_INT32(fNode->di_gid);
	fNode->di_nlink = B_BENDIAN_TO_HOST_INT32(fNode->di_nlink);
	fNode->di_projid = B_BENDIAN_TO_HOST_INT16(fNode->di_projid);
	fNode->di_flushiter = B_BENDIAN_TO_HOST_INT16(fNode->di_flushiter);
	fNode->di_atime.t_sec = B_BENDIAN_TO_HOST_INT32(fNode->di_atime.t_sec);
	fNode->di_atime.t_nsec = B_BENDIAN_TO_HOST_INT32(fNode->di_atime.t_nsec);
	fNode->di_mtime.t_sec = B_BENDIAN_TO_HOST_INT32(fNode->di_mtime.t_sec);
	fNode->di_mtime.t_nsec = B_BENDIAN_TO_HOST_INT32(fNode->di_mtime.t_nsec);
	fNode->di_ctime.t_sec = B_BENDIAN_TO_HOST_INT32(fNode->di_ctime.t_sec);
	fNode->di_ctime.t_nsec = B_BENDIAN_TO_HOST_INT32(fNode->di_ctime.t_nsec);
	fNode->di_size = B_BENDIAN_TO_HOST_INT64(fNode->di_size);
	fNode->di_nblocks = B_BENDIAN_TO_HOST_INT64(fNode->di_nblocks);
	fNode->di_extsize = B_BENDIAN_TO_HOST_INT32(fNode->di_extsize);
	fNode->di_nextents = B_BENDIAN_TO_HOST_INT32(fNode->di_nextents);
	fNode->di_naextents = B_BENDIAN_TO_HOST_INT16(fNode->di_naextents);
	fNode->di_dmevmask = B_BENDIAN_TO_HOST_INT32(fNode->di_dmevmask);
	fNode->di_dmstate = B_BENDIAN_TO_HOST_INT16(fNode->di_dmstate);
	fNode->di_flags = B_BENDIAN_TO_HOST_INT16(fNode->di_flags);
	fNode->di_gen = B_BENDIAN_TO_HOST_INT32(fNode->di_gen);
	fNode->di_next_unlinked = B_BENDIAN_TO_HOST_INT32(fNode->di_next_unlinked);
	fNode->di_changecount = B_BENDIAN_TO_HOST_INT64(fNode->di_changecount);
	fNode->di_lsn = B_BENDIAN_TO_HOST_INT64(fNode->di_lsn);
	fNode->di_flags2 = B_BENDIAN_TO_HOST_INT64(fNode->di_flags2);
	fNode->di_cowextsize = B_BENDIAN_TO_HOST_INT64(fNode->di_cowextsize);
	fNode->di_crtime.t_sec = B_BENDIAN_TO_HOST_INT32(fNode->di_crtime.t_sec);
	fNode->di_crtime.t_nsec = B_BENDIAN_TO_HOST_INT32(fNode->di_crtime.t_nsec);
	fNode->di_ino = B_BENDIAN_TO_HOST_INT64(fNode->di_ino);
}


void
Inode::GetModificationTime(struct timespec& stamp) const
{
	stamp.tv_sec = fNode->di_mtime.t_sec;
	stamp.tv_nsec = fNode->di_mtime.t_nsec;
}


void
Inode::GetAccessTime(struct timespec& stamp) const
{
	stamp.tv_sec = fNode->di_atime.t_sec;
	stamp.tv_nsec = fNode->di_atime.t_nsec;
}


void
Inode::GetChangeTime(struct timespec& stamp) const
{
	stamp.tv_sec = fNode->di_ctime.t_sec;
	stamp.tv_nsec = fNode->di_ctime.t_nsec;
}


void
Inode::GetCreationTime(struct timespec& stamp) const
{
	stamp.tv_sec = fNode->di_crtime.t_sec;
	stamp.tv_nsec = fNode->di_crtime.t_nsec;
}


Inode::Inode(Volume* volume, xfs_ino_t id)
	:
	fId(id),
	fVolume(volume),
	fBuffer(NULL),
	fExtents(NULL)
{
}


//Convert inode mode to directory entry filetype
unsigned char
Inode::XfsModeToFtype() const
{
	switch (Mode() & S_IFMT) {
	case S_IFREG:
		return XFS_DIR3_FT_REG_FILE;
	case S_IFDIR:
		return XFS_DIR3_FT_DIR;
	case S_IFCHR:
		return XFS_DIR3_FT_CHRDEV;
	case S_IFBLK:
		return XFS_DIR3_FT_BLKDEV;
	case S_IFIFO:
		return XFS_DIR3_FT_FIFO;
	case S_IFSOCK:
		return XFS_DIR3_FT_SOCK;
	case S_IFLNK:
		return XFS_DIR3_FT_SYMLINK;
	default:
		return XFS_DIR3_FT_UNKNOWN;
	}
}


bool
Inode::VerifyFork(int whichFork) const
{
	uint32 di_nextents = XFS_DFORK_NEXTENTS(fNode, whichFork);

	switch (XFS_DFORK_FORMAT(fNode, whichFork)) {
	case XFS_DINODE_FMT_LOCAL:
		if (whichFork == XFS_DATA_FORK) {
			if (S_ISREG(Mode()))
				return false;
			if (Size() > (xfs_fsize_t) DFORK_SIZE(fNode, fVolume, whichFork))
				return false;
		}
		if (di_nextents)
			return false;
		break;
	case XFS_DINODE_FMT_EXTENTS:
		if (di_nextents > DFORK_MAXEXT(fNode, fVolume, whichFork))
			return false;
		break;
	case XFS_DINODE_FMT_BTREE:
		if (whichFork == XFS_ATTR_FORK) {
			if (di_nextents > MAXAEXTNUM)
				return false;
		} else if (di_nextents > MAXEXTNUM) {
			return false;
		}
		break;
	default:
		return false;
	}

	return true;
}


bool
Inode::VerifyForkoff() const
{
	switch(Format()) {
		case XFS_DINODE_FMT_DEV:
			if (fNode->di_forkoff != (ROUNDUP(sizeof(uint32), 8) >> 3))
			return false;
		break;
		case XFS_DINODE_FMT_LOCAL:
		case XFS_DINODE_FMT_EXTENTS:
		case XFS_DINODE_FMT_BTREE:
			if (fNode->di_forkoff >= (LITINO(fVolume) >> 3))
				return false;
		break;
	default:
		return false;
	}

	return true;
}


bool
Inode::VerifyInode() const
{
	if(fNode->di_magic != INODE_MAGIC) {
		ERROR("Bad inode magic number");
		return false;
	}

	// check if inode version is valid
	if (Version() < 1 || Version() > 3) {
		ERROR("Bad inode version");
		return false;
	}

	// verify version 3 inodes first
	if (Version() == 3) {

		if(!HAS_V3INODES(fVolume)) {
			ERROR("xfs v4 doesn't have v3 inodes");
			return false;
		}

		if(!xfs_verify_cksum(fBuffer, fVolume->InodeSize(), INODE_CRC_OFF)) {
			ERROR("Inode is corrupted");
			return false;
		}

		if(fNode->di_ino != fId) {
			ERROR("Incorrect inode number");
			return false;
		}

		if(!fVolume->UuidEquals(fNode->di_uuid)) {
			ERROR("UUID is incorrect");
			return false;
		}
	}

	if(fNode->di_size & (1ULL << 63)) {
		ERROR("Invalid EOF of inode");
		return false;
	}

	if (Mode() && XfsModeToFtype() == XFS_DIR3_FT_UNKNOWN) {
		 ERROR("Entry points to an unknown inode type");
		 return false;
	}

	if(!VerifyForkoff()) {
		ERROR("Invalid inode fork offset");
		return false;
	}

	// Check for appropriate data fork formats for the mode
	switch (Mode() & S_IFMT) {
	case S_IFIFO:
	case S_IFCHR:
	case S_IFBLK:
	case S_IFSOCK:
		if (fNode->di_format != XFS_DINODE_FMT_DEV)
			return false;
		break;
	case S_IFREG:
	case S_IFLNK:
	case S_IFDIR:
		if (!VerifyFork(XFS_DATA_FORK)) {
			ERROR("Invalid data fork in inode");
			return false;
		}
		break;
	case 0:
		// Uninitialized inode is fine
		break;
	default:
		return false;
	}

	if (fNode->di_forkoff) {
		if (!VerifyFork(XFS_ATTR_FORK)) {
			ERROR("Invalid attribute fork in inode");
			return false;
		}
	} else {
		/*
			If there is no fork offset, this may be a freshly-made inode
			in a new disk cluster, in which case di_aformat is zeroed.
			Otherwise, such an inode must be in EXTENTS format; this goes
			for freed inodes as well.
		 */
		switch (fNode->di_aformat) {
		case 0:
		case XFS_DINODE_FMT_EXTENTS:
			break;
		default:
			return false;
		}

		if (fNode->di_naextents)
			return false;
	}

	// TODO : Add reflink and big-timestamps check using di_flags2

	return true;
}


uint32
Inode::CoreInodeSize() const
{
	if (Version() == 3)
		return sizeof(Dinode);

	return offsetof(Dinode, di_crc);
}


status_t
Inode::Init()
{
	fNode = new(std::nothrow) Dinode;
	if (fNode == NULL)
		return B_NO_MEMORY;

	uint16 inodeSize = fVolume->InodeSize();
	fBuffer = new(std::nothrow) char[inodeSize];
	if (fBuffer == NULL)
		return B_NO_MEMORY;

	status_t status = GetFromDisk();
	if (status == B_OK) {
		if (VerifyInode()) {
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
	if ((Version() == 3 && fVolume->XfsHasIncompatFeature())
		|| (fVolume->SuperBlockFeatures2() & XFS_SB_VERSION2_FTYPE)) {
		return true;
	}
	return false;
}


status_t
Inode::CheckPermissions(int accessMode) const
{
	// you never have write access to a read-only volume
	if ((accessMode & W_OK) != 0 && fVolume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	return check_access_permissions(accessMode, Mode(), (uint32)GroupId(), (uint32)UserId());
}


uint32
Inode::SizeOfLongBlock()
{
	if (Version() == 3)
		return sizeof(LongBlock);
	else
		return LongBlock::Offset_v5();
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
	char* dataStart = (char*) DIR_DFORK_PTR(Buffer(), CoreInodeSize());
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
			- CoreInodeSize();
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
		((char*)DIR_DFORK_PTR(Buffer(), CoreInodeSize()) + GetPtrOffsetIntoRoot(pos));
}


size_t
Inode::MaxRecordsPossibleNode()
{
	size_t availableSpace = GetVolume()->BlockSize();
	return availableSpace / (XFS_KEY_SIZE + XFS_PTR_SIZE);
}


size_t
Inode::GetPtrOffsetIntoNode(int pos)
{
	size_t maxRecords = MaxRecordsPossibleNode();

	return SizeOfLongBlock() + maxRecords * XFS_KEY_SIZE
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
	ssize_t& len, size_t DirBlockSize, char* block) {

	char* node = new(std::nothrow) char[len];
	if (node == NULL)
		return B_NO_MEMORY;
		// This isn't for a directory block but for one of the tree nodes

	ArrayDeleter<char> nodeDeleter(node);

	TRACE("levels:(%" B_PRIu16 ")\n", levelsInTree);

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
		if (!VerifyHeader<LongBlock>(curLongBlock, node, this,
				0, NULL, XFS_BTREE)) {
			TRACE("Invalid Long Block");
			return B_BAD_VALUE;
		}
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
		DIR_DFORK_PTR(Buffer(), CoreInodeSize()), sizeof(BlockInDataFork));

	uint16 levelsInTree = root->Levels();
	delete root;

	Volume* volume = GetVolume();
	ssize_t len = volume->BlockSize();

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
		if (!VerifyHeader<LongBlock>((LongBlock*)leafBuffer, leafBuffer, this,
				0, NULL, XFS_BTREE)) {
			TRACE("Invalid Long Block");
			return B_BAD_VALUE;
		}

		uint32 offset = SizeOfLongBlock();
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
		TRACE("Next leaf is at: (%" B_PRIu64 ")\n", fileSystemBlockNo);
		if (fileSystemBlockNo == (uint64) -1)
			break;
		uint64 readPos = FileSystemBlockToAddr(fileSystemBlockNo);
		if (read_pos(volume->Device(), readPos, block, len)
				!= len) {
				ERROR("Extent::FillBlockBuffer(): IO Error");
				return B_IO_ERROR;
		}
	}
	TRACE("Total covered: (%" B_PRId32 ")\n", indexIntoExtents);
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
Inode::SearchMapInAllExtent(uint64 blockNo)
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
	TRACE("Inode::ReadAt: pos:(%" B_PRIdOFF "), *length:(%" B_PRIuSIZE ")\n", pos, *length);
	status_t status;
	if (fExtents == NULL) {
		status = ReadExtents();
		if (status != B_OK)
			return status;
	}

	// set/check boundaries for pos/length
	if (pos < 0) {
		ERROR("inode %" B_PRIdINO ": ReadAt failed(pos %" B_PRIdOFF
			", length %lu)\n", ID(), pos, *length);
		return B_BAD_VALUE;
	}

	if (pos >= Size() || length == 0) {
		TRACE("inode %" B_PRIdINO ": ReadAt 0 (pos %" B_PRIdOFF
			", length %" B_PRIuSIZE ")\n", ID(), pos, *length);
		*length = 0;
		return B_NO_ERROR;
	}

	uint32 blockNo = BLOCKNO_FROM_POSITION(pos, GetVolume());
	uint32 offsetIntoBlock = BLOCKOFFSET_FROM_POSITION(pos, this);

	ssize_t lengthOfBlock = BlockSize();
	char* block = new(std::nothrow) char[lengthOfBlock];
	if (block == NULL)
		return B_NO_MEMORY;

	ArrayDeleter<char> blockDeleter(block);

	size_t lengthRead = 0;
	size_t lengthLeftInFile;
	size_t lengthLeftInBlock;
	size_t lengthToRead;
	TRACE("What is blockLen:(%" B_PRId64 ")\n", lengthOfBlock);
	while (*length > 0) {
		TRACE("Inode::ReadAt: pos:(%" B_PRIdOFF "), *length:(%" B_PRIuSIZE ")\n", pos, *length);
		// As long as you can read full blocks, read.
		lengthLeftInFile = Size() - pos;
		lengthLeftInBlock = lengthOfBlock - offsetIntoBlock;

		/*
			We will read file in blocks of size 4096 bytes, if that
			is not possible we will read file of remaining bytes.
			This meathod will change when we will add file cache for xfs.
		*/
		if(lengthLeftInFile >= 4096) {
			*length = 4096;
		} else {
			*length = lengthLeftInFile;
		}

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

	TRACE("lengthRead:(%" B_PRIuSIZE ")\n", lengthRead);
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

	TRACE("AgNumber: (%" B_PRIu32 "), AgRelativeIno: (%" B_PRIu32 "),"
		"AgRelativeBlockNum: (%" B_PRIu32 "),Offset: (%" B_PRId64 "),"
		"len: (%" B_PRIu32 ")\n", agNo,agRelativeInodeNo, agBlock, offset, len);

	if (agNo > fVolume->AgCount()) {
		ERROR("Inode::GetFromDisk : AG Number more than number of AGs");
		return B_ENTRY_NOT_FOUND;
	}

	xfs_agblock_t numberOfBlocksInAg = fVolume->AgBlocks();

	xfs_fsblock_t blockToRead = FSBLOCKS_TO_BASICBLOCKS(fVolume->BlockLog(),
		((uint64)(agNo * numberOfBlocksInAg) + agBlock));

	xfs_daddr_t readPos = blockToRead * XFS_MIN_BLOCKSIZE + offset * len;

	if (read_pos(fVolume->Device(), readPos, fBuffer, len) != len) {
		ERROR("Inode::Inode(): IO Error");
		return B_IO_ERROR;
	}

	if(fVolume->IsVersion5())
		memcpy(fNode, fBuffer, sizeof(Inode::Dinode));
	else
		memcpy(fNode, fBuffer, INODE_CRC_OFF);

	SwapEndian();

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
	TRACE("blockToRead:(%" B_PRIu64 ")\n", actualBlockToRead);

	uint64 readPos = actualBlockToRead * (XFS_MIN_BLOCKSIZE);
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
