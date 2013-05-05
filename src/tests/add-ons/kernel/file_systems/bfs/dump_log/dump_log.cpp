/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "bfs.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>


extern const char *__progname;
static const char *sProgramName = __progname;

static bool sDumpBlocks = false;


bool
disk_super_block::IsValid()
{
	if (Magic1() != (int32)SUPER_BLOCK_MAGIC1
		|| Magic2() != (int32)SUPER_BLOCK_MAGIC2
		|| Magic3() != (int32)SUPER_BLOCK_MAGIC3
		|| (int32)block_size != inode_size
		|| ByteOrder() != SUPER_BLOCK_FS_LENDIAN
		|| (1UL << BlockShift()) != BlockSize()
		|| AllocationGroups() < 1
		|| AllocationGroupShift() < 1
		|| BlocksPerAllocationGroup() < 1
		|| NumBlocks() < 10
		|| AllocationGroups() != divide_roundup(NumBlocks(),
			1L << AllocationGroupShift()))
		return false;

	return true;
}


off_t
toBlock(disk_super_block &superBlock, block_run run)
{
	return ((((off_t)run.AllocationGroup()) << superBlock.AllocationGroupShift()) | (off_t)run.Start());
}


off_t
toOffset(disk_super_block &superBlock, block_run run)
{
	return toBlock(superBlock, run) << superBlock.BlockShift();
}


void
dumpBlock(const char *buffer, int size)
{
	const int blockSize = 16;

	printf("\t");

	for (int i = 0; i < size;) {
		int start = i;

		for (; i < start + blockSize; i++) {
			if (!(i % 4))
				printf(" ");

			if (i >= size)
				printf("  ");
			else
				printf("%02x", *(unsigned char *)(buffer+i));
		}
		printf("  ");

		for (i = start; i < start + blockSize; i++) {
			if (i < size) {
				char c = *(buffer+i);

				if (c < 30)
					printf(".");
				else
					printf("%c", c);
			}
			else
				break;
		}

		if (i < size)
			printf("\n\t");
		else
			printf("\n");
	}
}


void
dumpLogEntry(int device, disk_super_block &superBlock, int32 &start, uint8 *block)
{
	int32 blockSize = superBlock.BlockSize();
	int32 entry = 0;
	off_t count = 1;
	int32 arrayBlocks = 1;
	off_t blockStart = start;
	bool first = true;
	int32 indexOffset = -1;
	int32 index = 0;
	off_t dataStart = blockStart;
	off_t bitmapSize = superBlock.AllocationGroups() * superBlock.BlocksPerAllocationGroup();

	uint8 *data = NULL;
	if (sDumpBlocks)
		data = (uint8 *)malloc(blockSize);

	for (int32 i = 0; i < arrayBlocks; i++) {
		off_t blockNumber = toBlock(superBlock, superBlock.log_blocks)
			+ blockStart++ % superBlock.log_blocks.Length();
		if (read_pos(device, blockNumber << superBlock.BlockShift(),
				block, blockSize) != blockSize) {
			fprintf(stderr, "%s: could not read block %Ld.\n", sProgramName, blockNumber);
			exit(-1);
		}

		off_t *array = (off_t *)block;
		int32 arraySize = blockSize / sizeof(off_t);

		if (first) {
			count = array[0];
			arrayBlocks = ((count + 1) * sizeof(off_t) + blockSize - 1) / blockSize;
			arraySize--;
			dataStart += arrayBlocks;

			start += arrayBlocks + count;

			printf("Entry %ld contains %Ld blocks (entry starts at block %Ld):\n", entry, count, blockNumber);
			first = false;
		} else
			indexOffset += arraySize;

		for (; index < count; index++) {
			int32 arrayIndex = index - indexOffset;
			if (arrayIndex >= arraySize)
				break;

			off_t value = array[arrayIndex];
			printf("%7ld: %Ld%s\n", index, value, value == 0 ? " (superblock)" :
				value < bitmapSize + 1 ? " (bitmap block)" : "");

			if (data != NULL) {
				off_t blockNumber = toBlock(superBlock, superBlock.log_blocks)
					+ ((dataStart + index) % superBlock.log_blocks.Length());
				if (read_pos(device, blockNumber << superBlock.BlockShift(),
						data, blockSize) != blockSize) {
					fprintf(stderr, "%s: could not read block %Ld.\n", sProgramName, blockNumber);
				} else
					dumpBlock((char *)data, blockSize);
			}
		}
	}

	free(data);
}


void
usage(void)
{
	fprintf(stderr, "usage: %s [-d] bfs-volume\n"
		"    -d\tdump blocks in log\n", sProgramName);
	exit(0);
}


int
main(int argc, char **argv)
{
	if (argc < 2 || !strcmp(argv[1], "--help"))
		usage();

	while (*++argv) {
		char *arg = *argv;
		if (*arg == '-') {
			while (*++arg && isalpha(*arg)) {
				switch (*arg) {
					case 'd':
						sDumpBlocks = true;
						break;
					default:
						usage();
				}
			}
		}
		else
			break;
	}

	char *devicePath = argv[0];
	int device = open(devicePath, O_RDONLY);
	if (device < 0) {
		fprintf(stderr, "%s: could not open device %s: %s\n",
			sProgramName, devicePath, strerror(errno));
		return -1;
	}

	disk_super_block superBlock;
	if (read_pos(device, 512, &superBlock, sizeof(disk_super_block)) < (ssize_t)sizeof(disk_super_block)) {
		fprintf(stderr, "%s: could not read superblock.\n", sProgramName);
		return -1;
	}

	if (!superBlock.IsValid()) {
		fprintf(stderr, "%s: invalid superblock!\n", sProgramName);
		return -1;
	}

	if (superBlock.Flags() == SUPER_BLOCK_DISK_CLEAN) {
		printf("volume is clean; log is empty.\n");
		return 0;
	}

	off_t bitmapSize = superBlock.AllocationGroups() * superBlock.BlocksPerAllocationGroup();
	printf("Log area = (%Ld - %Ld), current start = %Ld, end = %Ld, %Ld bitmap blocks\n",
		toBlock(superBlock, superBlock.log_blocks),
		toBlock(superBlock, superBlock.log_blocks) + superBlock.log_blocks.Length(),
		superBlock.LogStart(), superBlock.LogEnd(), bitmapSize);

	uint8 *block = (uint8 *)malloc(superBlock.BlockSize());
	if (block == NULL) {
		fprintf(stderr, "%s: could not allocate block!\n", sProgramName);
		return -1;
	}

	int32 start = superBlock.LogStart();
	int32 lastStart = -1;

	while (true) {
		if (start == superBlock.LogEnd())
			break;
		if (start == lastStart)
			break;

		lastStart = start;	
		dumpLogEntry(device, superBlock, start, block);
	}

	close(device);
	return 0;
}

