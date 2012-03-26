/*
 * Copyright (c) 2001-2008 pinc Software. All Rights Reserved.
 */

//!	Handles BFS super block, disk access etc.

#include "Disk.h"
#include "dump.h"

#include <Drivers.h>
#include <File.h>
#include <Entry.h>
#include <List.h>
#include <fs_info.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>


#define MIN_BLOCK_SIZE_INODES 50
#define MAX_BLOCK_SIZE_INODES 500


struct bfs_disk_info {
	off_t	offset;
	disk_super_block super_block;
};


class CacheableBlockRun : public BlockRunCache::Cacheable {
	public:
		CacheableBlockRun(block_run run,uint8 *data)
			:
			fRun(run),
			fData(data)
		{
		}

		virtual ~CacheableBlockRun()
		{
			free(fData);
		}

		virtual bool Equals(block_run run)
		{
			return run == fRun;
		}

		void SetData(uint8 *data)
		{
			fData = data;
		}

		uint8 *Data()
		{
			return fData;
		}

	protected:
		block_run	fRun;
		uint8		*fData;
};


BlockRunCache::BlockRunCache(Disk *disk)
	: Cache<block_run>(),
	fDisk(disk)
{
}


Cache<block_run>::Cacheable *BlockRunCache::NewCacheable(block_run run)
{
	ssize_t length = (int32)run.length << fDisk->BlockShift();

	void *buffer = malloc(length);
	if (buffer == NULL)
		return NULL;

	ssize_t read = fDisk->ReadAt(fDisk->ToOffset(run),buffer,length);
	if (read < length) {
		free(buffer);
		return NULL;
	}

	return new CacheableBlockRun(run,(uint8 *)buffer);
}


//	#pragma mark -


Disk::Disk(const char *deviceName, bool rawMode, off_t start, off_t stop)
	:
	fRawDiskOffset(0),
	fSize(0LL),
	fCache(this),
	fRawMode(rawMode)
{
	BEntry entry(deviceName);
	fPath.SetTo(deviceName);

	if (entry.IsDirectory()) {
		dev_t on = dev_for_path(deviceName);
		fs_info info;
		if (fs_stat_dev(on, &info) != B_OK)
			return;

		fPath.SetTo(info.device_name);
		deviceName = fPath.Path();
	}

	if (!rawMode && !strncmp(deviceName, "/dev/", 5)
		&& !strcmp(deviceName + strlen(deviceName) - 3, "raw"))
		fprintf(stderr, "Raw mode not selected, but raw device specified.\n");

	if (fFile.SetTo(deviceName, B_READ_WRITE) < B_OK) {
		//fprintf(stderr,"Could not open file: %s\n",strerror(fFile.InitCheck()));
		return;
	}

	int device = open(deviceName, O_RDONLY);
	if (device < B_OK) {
		//fprintf(stderr,"Could not open device\n");
		return;
	}

	partition_info partitionInfo;
	device_geometry geometry;
	if (ioctl(device, B_GET_PARTITION_INFO, &partitionInfo,
			sizeof(partition_info)) == 0) {
		//if (gDumpPartitionInfo)
		//	dump_partition_info(&partitionInfo);
		fSize = partitionInfo.size;
	} else if (ioctl(device, B_GET_GEOMETRY, &geometry, sizeof(device_geometry))
			== 0) {
		fSize = (off_t)geometry.cylinder_count * geometry.sectors_per_track
				* geometry.head_count * geometry.bytes_per_sector;
	} else {
		fprintf(stderr,"Disk: Could not get partition info.\n  Use file size as partition size\n");
		fFile.GetSize(&fSize);
	}
	close(device);

	if (fSize == 0LL)
		fprintf(stderr,"Disk: Invalid file size (%Ld bytes)!\n",fSize);

	if (rawMode && ScanForSuperBlock(start, stop) < B_OK) {
		fFile.Unset();
		return;
	}

	if (fFile.ReadAt(512 + fRawDiskOffset, &fSuperBlock,
			sizeof(disk_super_block)) < 1)
		fprintf(stderr,"Disk: Could not read super block\n");

	//dump_super_block(&fSuperBlock);
}


Disk::~Disk()
{
}


status_t Disk::InitCheck()
{
	status_t status = fFile.InitCheck();
	if (status == B_OK)
		return fSize == 0LL ? B_ERROR : B_OK;
	
	return status;
}


block_run Disk::ToBlockRun(off_t start, int16 length) const
{
	block_run run;
	run.allocation_group = start >> fSuperBlock.ag_shift;
	run.start = start & ((1UL << fSuperBlock.ag_shift) - 1);
	run.length = length;

	return run;
}


off_t Disk::LogSize() const
{
	if (fSuperBlock.num_blocks >= 4096)
		return 2048;

	return 512;
}


uint8 *Disk::ReadBlockRun(block_run run)
{
	CacheableBlockRun *entry = (CacheableBlockRun *)fCache.Get(run);
	if (entry)
		return entry->Data();
	
	return NULL;
}


status_t
Disk::DumpBootBlockToFile()
{
	BFile file("/boot/home/bootblock.old", B_READ_WRITE | B_CREATE_FILE);
	if (file.InitCheck() < B_OK)
		return file.InitCheck();

	char buffer[BlockSize()];
	ssize_t bytes = ReadAt(0, buffer, BlockSize());
	if (bytes < (int32)BlockSize())
		return bytes < B_OK ? bytes : B_ERROR;

	file.Write(buffer, BlockSize());

	// initialize buffer
	memset(buffer, 0, BlockSize());
	memcpy(buffer + 512, &fSuperBlock, sizeof(disk_super_block));

	// write buffer to disk
	WriteAt(0, buffer, BlockSize());

	return B_OK;
}


//	#pragma mark - Superblock recovery methods


status_t 
Disk::ScanForSuperBlock(off_t start, off_t stop)
{
	printf("Disk size %Ld bytes, %.2f GB\n", fSize, 1.0 * fSize / (1024*1024*1024));

	uint32 blockSize = 4096;
	char buffer[blockSize + 1024];

	if (stop == -1)
		stop = fSize;

	char escape[3] = {0x1b, '[', 0};

	BList superBlocks;

	printf("Scanning Disk from %Ld to %Ld\n", start, stop);

	for (off_t offset = start; offset < stop; offset += blockSize)
	{
		if (((offset-start) % (blockSize * 100)) == 0)
			printf("  %12Ld, %.2f GB     %s1A\n",offset,1.0 * offset / (1024*1024*1024),escape);

		ssize_t bytes = fFile.ReadAt(offset, buffer, blockSize + 1024);
		if (bytes < B_OK)
		{
			fprintf(stderr,"Could not read from device: %s\n", strerror(bytes));
			return -1;
		}

		for (uint32 i = 0;i < blockSize - 2;i++)
		{
			disk_super_block *super = (disk_super_block *)&buffer[i];

			if (super->magic1 == (int32)SUPER_BLOCK_MAGIC1
				&& super->magic2 == (int32)SUPER_BLOCK_MAGIC2
				&& super->magic3 == (int32)SUPER_BLOCK_MAGIC3)
			{
				printf("\n(%ld) *** BFS superblock found at: %Ld\n",superBlocks.CountItems() + 1,offset);
				dump_super_block(super);

				// add a copy of the super block to the list
				bfs_disk_info *info = (bfs_disk_info *)malloc(sizeof(bfs_disk_info));
				if (info == NULL)
					return B_NO_MEMORY;

				memcpy(&info->super_block, super, sizeof(disk_super_block));
				info->offset = offset + i - 512;
					/* location off the BFS super block is 512 bytes after the partition start */
				superBlocks.AddItem(info);
			}
		}
	}

	if (superBlocks.CountItems() == 0) {
		puts("\nCouldn't find any BFS super blocks!");
		return B_ENTRY_NOT_FOUND;
	}

	// Let the user decide which block to use

	puts("\n\nThe following disks were found:");

	for (int32 i = 0; i < superBlocks.CountItems(); i++) {
		bfs_disk_info *info = (bfs_disk_info *)superBlocks.ItemAt(i);

		printf("%ld) %s, offset %Ld, size %g GB (%svalid)\n", i + 1,
			info->super_block.name, info->offset,
			1.0 * info->super_block.num_blocks * info->super_block.block_size / (1024*1024*1024),
			ValidateSuperBlock(info->super_block) < B_OK ? "in" : "");
	}

	char answer[16];
	printf("\nChoose one (by number): ");
	fflush(stdout);

	fgets(answer, 15, stdin);
	int32 index = atol(answer);

	if (index > superBlocks.CountItems() || index < 1) {
		puts("No disk there... exiting.");
		return B_BAD_VALUE;
	}

	bfs_disk_info *info = (bfs_disk_info *)superBlocks.ItemAt(index - 1);

	// ToDo: free the other disk infos

	fRawDiskOffset = info->offset;
	fFile.Seek(fRawDiskOffset, SEEK_SET);

	if (ValidateSuperBlock(info->super_block))
		fSize = info->super_block.block_size * info->super_block.block_size;
	else {
		// just make it open-end
		fSize -= fRawDiskOffset;
	}

	return B_OK;
}


status_t
Disk::ValidateSuperBlock(disk_super_block &superBlock)
{
	if (superBlock.magic1 != (int32)SUPER_BLOCK_MAGIC1
		|| superBlock.magic2 != (int32)SUPER_BLOCK_MAGIC2
		|| superBlock.magic3 != (int32)SUPER_BLOCK_MAGIC3
		|| (int32)superBlock.block_size != superBlock.inode_size
		|| superBlock.fs_byte_order != SUPER_BLOCK_FS_LENDIAN
		|| (1UL << superBlock.block_shift) != superBlock.block_size
		|| superBlock.num_ags < 1
		|| superBlock.ag_shift < 1
		|| superBlock.blocks_per_ag < 1
		|| superBlock.num_blocks < 10
		|| superBlock.used_blocks > superBlock.num_blocks
		|| superBlock.num_ags != divide_roundup(superBlock.num_blocks,
				1L << superBlock.ag_shift))
		return B_ERROR;

	return B_OK;
}


status_t
Disk::ValidateSuperBlock()
{
	if (ValidateSuperBlock(fSuperBlock) < B_OK)
		return B_ERROR;

	fBitmap.SetTo(this);

	return B_OK;
}


status_t
Disk::RecreateSuperBlock()
{
	memset(&fSuperBlock,0,sizeof(disk_super_block));

	puts("\n*** Determine block size");

	status_t status = DetermineBlockSize();
	if (status < B_OK)
		return status;

	printf("\tblock size = %ld\n",BlockSize());

	strcpy(fSuperBlock.name,"recovered");	
	fSuperBlock.magic1 = SUPER_BLOCK_MAGIC1;
	fSuperBlock.fs_byte_order = SUPER_BLOCK_FS_LENDIAN;
	fSuperBlock.block_shift = get_shift(BlockSize());
	fSuperBlock.num_blocks = fSize / BlockSize();	// only full blocks
	fSuperBlock.inode_size = BlockSize();
	fSuperBlock.magic2 = SUPER_BLOCK_MAGIC2;

	fSuperBlock.flags = SUPER_BLOCK_CLEAN;

	// size of the block bitmap + root block
	fLogStart = 1 + divide_roundup(fSuperBlock.num_blocks,BlockSize() * 8);

	// set it anywhere in the log
	fSuperBlock.log_start = fLogStart + (LogSize() >> 1);
	fSuperBlock.log_end = fSuperBlock.log_start;

	//
	// scan for indices and root inode
	//

	puts("\n*** Scanning for indices and root node...");
	fValidOffset = 0LL;
	bfs_inode indexDir;
	bfs_inode rootDir;
	if (ScanForIndexAndRoot(&indexDir,&rootDir) < B_OK) {
		fprintf(stderr,"ERROR: Could not find root directory! Trying to recreate the super block will have no effect!\n\tSetting standard values for the root dir.\n");
		rootDir.inode_num.allocation_group = 8;
		rootDir.inode_num.start = 0;
		rootDir.inode_num.length = 1;
		//gErrors++;
	}
	if (fValidOffset == 0LL) {
		printf("No valid inode found so far - perform search.\n");

		off_t offset = 8LL * 65536 * BlockSize();
		char buffer[1024];
		GetNextSpecialInode(buffer,&offset,offset + 32LL * 65536 * BlockSize(),true);

		if (fValidOffset == 0LL)
		{			
			fprintf(stderr,"FATAL ERROR: Could not find valid inode!\n");
			return B_ERROR;
		}
	}

	// calculate allocation group size

	int32 allocationGroupSize = (fValidOffset - fValidBlockRun.start
			* BlockSize()
		+ BlockSize() - 1) / (BlockSize() * fValidBlockRun.allocation_group);

	fSuperBlock.blocks_per_ag = allocationGroupSize / (BlockSize() << 3);
	fSuperBlock.ag_shift = get_shift(allocationGroupSize);
	fSuperBlock.num_ags = divide_roundup(fSuperBlock.num_blocks,allocationGroupSize);

	// calculate rest of log area
	
	fSuperBlock.log_blocks.allocation_group = fLogStart / allocationGroupSize;
	fSuperBlock.log_blocks.start = fLogStart - fSuperBlock.log_blocks.allocation_group * allocationGroupSize;
	fSuperBlock.log_blocks.length = LogSize();	// assumed length of 2048 blocks

	if (fLogStart != ((indexDir.inode_num.allocation_group
			<< fSuperBlock.ag_shift) + indexDir.inode_num.start - LogSize())) {
		fprintf(stderr,"ERROR: start of logging area doesn't fit assumed value "
			"(%Ld blocks before indices)!\n", LogSize());
		//gErrors++;
	}

	fSuperBlock.magic3 = SUPER_BLOCK_MAGIC3;
	fSuperBlock.root_dir = rootDir.inode_num;
	fSuperBlock.indices = indexDir.inode_num;

	// calculate used blocks (from block bitmap)

	fBitmap.SetTo(this);
	if (fBitmap.UsedBlocks())
		fSuperBlock.used_blocks = fBitmap.UsedBlocks();
	else {
		fprintf(stderr,"ERROR: couldn't calculate number of used blocks, marking all blocks as used!\n");
		fSuperBlock.used_blocks = fSuperBlock.num_blocks;
		//gErrors++;
	}

	return B_OK;
}


status_t
Disk::DetermineBlockSize()
{
	// scan for about 500 inodes to determine the disk's block size

	// min. block bitmap size (in bytes, rounded to a 1024 boundary)
	// root_block_size + (num_blocks / bits_per_block) * block_size
	off_t offset = 1024 + divide_roundup(fSize / 1024,8 * 1024) * 1024;

	off_t inodesFound = 0;
	char buffer[1024];
	bfs_inode *inode = (bfs_inode *)buffer;
	// valid block sizes from 1024 to 32768 bytes
	int32 blockSizeCounter[6] = {0};
	status_t status = B_OK;

	// read a quarter of the drive at maximum
	for (; offset < (fSize >> 2); offset += 1024) {
		if (fFile.ReadAt(offset, buffer, sizeof(buffer)) < B_OK) {
			fprintf(stderr, "could not read from device (offset = %Ld, "
				"size = %ld)!\n", offset, sizeof(buffer));
			status = B_IO_ERROR;
			break;
		}

		if (inode->magic1 != INODE_MAGIC1
			|| inode->inode_size != 1024
			&& inode->inode_size != 2048
			&& inode->inode_size != 4096
			&& inode->inode_size != 8192)
			continue;

		inodesFound++;

		// update block size counter
		for (int i = 0;i < 6;i++)
			if ((1 << (i + 10)) == inode->inode_size)
				blockSizeCounter[i]++;

		int32 count = 0;
		for (int32 i = 0;i < 6;i++)
			if (blockSizeCounter[i])
				count++;

		// do we have a clear winner at 100 inodes?
		// if not, break at 500 inodes
		if (inodesFound >= MAX_BLOCK_SIZE_INODES
			|| (inodesFound >= MIN_BLOCK_SIZE_INODES && count == 1))
			break;
	}

	// find the safest bet
	int32 maxCounter = -1;
	int32 maxIndex = 0;
	for (int32 i = 0; i < 6; i++) {
		if (maxCounter < blockSizeCounter[i]) {
			maxIndex = i;
			maxCounter = blockSizeCounter[i];
		}
	}
	fSuperBlock.block_size = (1 << (maxIndex + 10));

	return status;
}


status_t
Disk::GetNextSpecialInode(char *buffer, off_t *_offset, off_t end,
	bool skipAfterValidInode = false)
{
	off_t offset = *_offset;
	if (end == offset)
		end++;

	bfs_inode *inode = (bfs_inode *)buffer;

	for (; offset < end; offset += BlockSize()) {
		if (fFile.ReadAt(offset, buffer, 1024) < B_OK) {
			fprintf(stderr,"could not read from device (offset = %Ld, size = %d)!\n",offset,1024);
			*_offset = offset;
			return B_IO_ERROR;
		}

		if (inode->magic1 != INODE_MAGIC1
			|| inode->inode_size != fSuperBlock.inode_size)
			continue;

		// can compute allocation group only for inodes which are
		// a) not in the first allocation group
		// b) not in the log area

		if (inode->inode_num.allocation_group > 0
			&& offset >= (BlockSize() * (fLogStart + LogSize()))) {
			fValidBlockRun = inode->inode_num;
			fValidOffset = offset;
			
			if (skipAfterValidInode)
				return B_OK;
		}

		// is the inode a special root inode (parent == self)?

		if (!memcmp(&inode->parent, &inode->inode_num, sizeof(inode_addr))) {
			printf("\t  special inode found at %Ld\n", offset);

			*_offset = offset;
			return B_OK;
		}
	}
	*_offset = offset;
	return B_ENTRY_NOT_FOUND;
}


void
Disk::SaveInode(bfs_inode *inode, bool *indices, bfs_inode *indexDir,
	bool *root, bfs_inode *rootDir)
{
	if ((inode->mode & S_INDEX_DIR) == 0) {
		*root = true;
		printf("\troot node found!\n");
		if (inode->inode_num.allocation_group != 8
			|| inode->inode_num.start != 0
			|| inode->inode_num.length != 1)
			printf("WARNING: root node has unexpected position: (ag = %ld, "
				"start = %d, length = %d)\n", inode->inode_num.allocation_group,
				inode->inode_num.start, inode->inode_num.length);
		if (rootDir)
			memcpy(rootDir, inode, sizeof(bfs_inode));
	} else if (inode->mode & S_INDEX_DIR) {
		*indices = true;
		printf("\tindices node found!\n");
		if (indexDir)
			memcpy(indexDir, inode, sizeof(bfs_inode));
	}
//	if (gDumpIndexRootNode)
//		dump_inode(inode);
}


status_t
Disk::ScanForIndexAndRoot(bfs_inode *indexDir,bfs_inode *rootDir)
{
	memset(rootDir,0,sizeof(bfs_inode));
	memset(indexDir,0,sizeof(bfs_inode));

	bool indices = false,root = false;
	char buffer[1024];
	bfs_inode *inode = (bfs_inode *)buffer;

	// The problem here is that we don't want to find copies of the
	// inodes in the log.
	// The first offset where a root node can be is therefore the
	// first allocation group with a block size of 1024, and 16384
	// blocks per ag; that should be relatively save.

	// search for the indices inode, start reading from log size + boot block size
	off_t offset = (fLogStart + LogSize()) * BlockSize();
	if (GetNextSpecialInode(buffer,&offset,offset + 65536LL * BlockSize()) == B_OK)
		SaveInode(inode,&indices,indexDir,&root,rootDir);

	if (!indices) {
		fputs("ERROR: no indices node found!\n",stderr);
		//gErrors++;
	}

	// search common places for root node, iterating from allocation group
	// size from 1024 to 65536
	for (off_t start = 8LL * 1024 * BlockSize();start <= 8LL * 65536 * BlockSize();start <<= 1) {
		if (start < fLogStart)
			continue;

		off_t commonOffset = start;
		if (GetNextSpecialInode(buffer, &commonOffset, commonOffset) == B_OK) {
			SaveInode(inode, &indices, indexDir, &root, rootDir);
			if (root)
				break;
		}
	}

	if (!root) {
		printf("WARNING: Could not find root node at common places!\n");
		printf("\tScanning log area for root node\n");
		
		off_t logOffset = ToOffset(fSuperBlock.log_blocks);
		if (GetNextSpecialInode(buffer,&logOffset,logOffset + LogSize() * BlockSize()) == B_OK)
		{
			SaveInode(inode,&indices,indexDir,&root,rootDir);
			
			printf("root node at: 0x%Lx (DiskProbe)\n",logOffset / 512);
			//fFile.ReadAt(logOffset + BlockSize(),buffer,1024);
			//if (*(uint32 *)buffer == BPLUSTREE_MAGIC)
			//{
			//	puts("\t\tnext block in log contains a bplustree!");
			//}
		}
	}
	
	/*if (!root)
	{
		char txt[64];
		printf("Should I perform a deeper search (that will take some time) (Y/N) [N]? ");
		gets(txt);
		
		if (!strcasecmp("y",txt))
		{
			// search not so common places for the root node (all places)
			
			if (indices)
				offset += BlockSize();	// the block after the indices inode
			else
				offset = (fLogStart + 1) * BlockSize();

			if (GetNextSpecialInode(buffer,&offset,65536LL * 16 * BlockSize()) == B_OK)
				SaveInode(inode,&indices,indexDir,&root,rootDir);
		}
	}*/
	if (root)
		return B_OK;

	return B_ERROR;
}


//	#pragma mark - BPositionIO methods


ssize_t
Disk::Read(void *buffer, size_t size)
{
	return fFile.Read(buffer, size);
}


ssize_t
Disk::Write(const void *buffer, size_t size)
{
	return fFile.Write(buffer, size);
}


ssize_t
Disk::ReadAt(off_t pos, void *buffer, size_t size)
{
	return fFile.ReadAt(pos + fRawDiskOffset, buffer, size);
}


ssize_t
Disk::WriteAt(off_t pos, const void *buffer, size_t size)
{
	return fFile.WriteAt(pos + fRawDiskOffset, buffer, size);
}


off_t
Disk::Seek(off_t position, uint32 seekMode)
{
	// ToDo: only correct for seekMode == SEEK_SET, right??
	if (seekMode != SEEK_SET)
		puts("OH NO, I AM BROKEN!");
	return fFile.Seek(position + fRawDiskOffset, seekMode);
}


off_t
Disk::Position() const
{
	return fFile.Position() - fRawDiskOffset;
}


status_t
Disk::SetSize(off_t /*size*/)
{
	// SetSize() is not supported
	return B_ERROR;
}

