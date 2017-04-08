/*
 * Copyright 2001-2010 pinc Software. All Rights Reserved.
 */


//!	Dumps various information about BFS volumes.


#include "Disk.h"
#include "BPlusTree.h"
#include "Inode.h"
#include "dump.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


void
dump_bplustree(Disk &disk, BPositionIO *file, off_t size, bool hexDump)
{
	uint8 *buffer = (uint8 *)malloc(size);
	if (buffer == NULL) {
		puts("no buffer");
		return;
	}

	if (file->ReadAt(0, buffer, size) != size) {
		puts("couldn't read whole file");
		return;
	}

	bplustree_header *header = (bplustree_header *)buffer;
	int32 nodeSize = header->node_size;

	dump_bplustree_header(header);

	bplustree_node *node = (bplustree_node *)(buffer + nodeSize);
	while ((addr_t)node < (addr_t)buffer + size) {
		printf("\n\n-------------------\n"
			"** node at offset: %" B_PRIuADDR "\n** used: %" B_PRId32 " bytes"
			"\n", (addr_t)node - (addr_t)buffer, node->Used());
		dump_bplustree_node(node, header, &disk);

		if (hexDump) {
			putchar('\n');
			dump_block((char *)node, header->node_size, 0);
		}

		node = (bplustree_node *)((addr_t)node + nodeSize);
	}
}


void
dump_indirect_stream(Disk &disk, bfs_inode *node, bool showOffsets)
{
	if (node->data.max_indirect_range == 0)
		return;

	int32 bytes = node->data.indirect.length * disk.BlockSize();
	int32 count = bytes / sizeof(block_run);
	block_run runs[count];

	off_t offset = node->data.max_direct_range;

	ssize_t bytesRead = disk.ReadAt(disk.ToOffset(node->data.indirect),
		(uint8 *)runs, bytes);
	if (bytesRead < bytes) {
		fprintf(stderr, "couldn't read indirect runs: %s\n",
			strerror(bytesRead));
		return;
	}

	puts("indirect stream:");

	for (int32 i = 0; i < count; i++) {
		if (runs[i].IsZero())
			return;

		printf("  indirect[%04" B_PRId32 "]            = ", i);

		char buffer[256];
		if (showOffsets)
			snprintf(buffer, sizeof(buffer), " %16" B_PRIdOFF, offset);
		else
			buffer[0] = '\0';

		dump_block_run("", runs[i], buffer);

		offset += runs[i].length * disk.BlockSize();
	}
}


void
dump_double_indirect_stream(Disk& disk, bfs_inode* node, bool showOffsets)
{
	if (node->data.max_double_indirect_range == 0)
		return;

	int32 bytes = node->data.double_indirect.length * disk.BlockSize();
	int32 count = bytes / sizeof(block_run);
	block_run runs[count];

	off_t offset = node->data.max_indirect_range;

	ssize_t bytesRead = disk.ReadAt(disk.ToOffset(node->data.double_indirect),
		(uint8*)runs, bytes);
	if (bytesRead < bytes) {
		fprintf(stderr, "couldn't read double indirect runs: %s\n",
			strerror(bytesRead));
		return;
	}

	puts("double indirect stream:");

	for (int32 i = 0; i < count; i++) {
		if (runs[i].IsZero())
			return;

		printf("  double_indirect[%02" B_PRId32 "]       = ", i);

		dump_block_run("", runs[i], "");

		int32 indirectBytes = runs[i].length * disk.BlockSize();
		int32 indirectCount = indirectBytes / sizeof(block_run);
		block_run indirectRuns[indirectCount];

		bytesRead = disk.ReadAt(disk.ToOffset(runs[i]), (uint8*)indirectRuns,
			indirectBytes);
		if (bytesRead < indirectBytes) {
			fprintf(stderr, "couldn't read double indirect runs: %s\n",
				strerror(bytesRead));
			continue;
		}

		for (int32 j = 0; j < indirectCount; j++) {
			if (indirectRuns[j].IsZero())
				break;

			printf("                     [%04" B_PRId32 "] = ", j);

			char buffer[256];
			if (showOffsets)
				snprintf(buffer, sizeof(buffer), " %16" B_PRIdOFF, offset);
			else
				buffer[0] = '\0';

			dump_block_run("", indirectRuns[j], buffer);

			offset += indirectRuns[j].length * disk.BlockSize();
		}
	}
}


void
list_bplustree(Disk& disk, Directory* directory, off_t size)
{
	directory->Rewind();

	char name[B_FILE_NAME_LENGTH];
	char buffer[512];
	uint64 count = 0;
	block_run run;
	while (directory->GetNextEntry(name, &run) == B_OK) {
		snprintf(buffer, sizeof(buffer), " %s", name);
		dump_block_run("", run, buffer);
		count++;
	}

	printf("--\n%lld items.\n", count);
}


void
count_bplustree(Disk& disk, Directory* directory, off_t size)
{
	directory->Rewind();

	char name[B_FILE_NAME_LENGTH];
	uint64 count = 0;
	block_run run;
	while (directory->GetNextEntry(name, &run) == B_OK)
		count++;

	printf("%lld items.\n", count);
}


block_run
parseBlockRun(Disk &disk, char *first, char *last)
{
	char *comma;

	if (last) {
		return block_run::Run(atol(first), atol(last), 1);
	} else if ((comma = strchr(first, ',')) != NULL) {
		*comma++ = '\0';
		return block_run::Run(atol(first), atol(comma));
	}

	return disk.ToBlockRun(atoll(first));
}


int
main(int argc, char **argv)
{
	puts("Copyright (c) 2001-2010 pinc Software.");

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		char *filename = strrchr(argv[0],'/');
		fprintf(stderr,"usage: %s [-srib] <device> [allocation_group start]\n"
				"\t-s\tdump superblock\n"
				"\t-r\tdump root node\n"
				"       the following options need the allocation_group/start "
					"parameters:\n"
				"\t-i\tdump inode\n"
				"\t-b\tdump b+tree\n"
				"\t-c\tlist b+tree leaves\n"
				"\t-c\tcount b+tree leaves\n"
				"\t-v\tvalidate b+tree\n"
				"\t-h\thexdump\n"
				"\t-o\tshow disk offsets\n",
				filename ? filename + 1 : argv[0]);
		return -1;
	}

	bool dumpRootNode = false;
	bool dumpInode = false;
	bool dumpSuperBlock = false;
	bool dumpBTree = false;
	bool listBTree = false;
	bool countBTree = false;
	bool validateBTree = false;
	bool dumpHex = false;
	bool showOffsets = false;

	while (*++argv) {
		char *arg = *argv;
		if (*arg == '-') {
			while (*++arg && isalpha(*arg)) {
				switch (*arg) {
					case 's':
						dumpSuperBlock = true;
						break;
					case 'r':
						dumpRootNode = true;
						break;
					case 'i':
						dumpInode = true;
						break;
					case 'b':
						dumpBTree = true;
						break;
					case 'l':
						listBTree = true;
						break;
					case 'c':
						countBTree = true;
						break;
					case 'v':
						validateBTree = true;
						break;
					case 'h':
						dumpHex = true;
						break;
					case 'o':
						showOffsets = true;
						break;
				}
			}
		} else
			break;
	}

	Disk disk(argv[0]);
	if (disk.InitCheck() < B_OK)
	{
		fprintf(stderr, "Could not open device or file: %s\n", strerror(disk.InitCheck()));
		return -1;
	}
	putchar('\n');

	if (!dumpSuperBlock && !dumpRootNode && !dumpInode && !dumpBTree
		&& !dumpHex && !listBTree && !countBTree) {
		printf("  Name:\t\t\t\"%s\"\n", disk.SuperBlock()->name);
		printf("    (disk is %s)\n\n",
			disk.ValidateSuperBlock() == B_OK ? "valid" : "invalid!!");
		printf("  Block Size:\t\t%" B_PRIu32 " bytes\n", disk.BlockSize());
		printf("  Number of Blocks:\t%12" B_PRIdOFF "\t%10g MB\n",
			disk.NumBlocks(), disk.NumBlocks() * disk.BlockSize()
				/ (1024.0*1024));
		if (disk.BlockBitmap() != NULL) {
			printf("  Used Blocks:\t\t%12" B_PRIdOFF "\t%10g MB\n",
				disk.BlockBitmap()->UsedBlocks(),
				disk.BlockBitmap()->UsedBlocks() * disk.BlockSize()
					/ (1024.0*1024));
			printf("  Free Blocks:\t\t%12" B_PRIdOFF "\t%10g MB\n",
				disk.BlockBitmap()->FreeBlocks(),
				disk.BlockBitmap()->FreeBlocks() * disk.BlockSize()
					/ (1024.0*1024));
		}
		int32 size
			= (disk.AllocationGroups() * disk.SuperBlock()->blocks_per_ag);
		printf("  Bitmap Size:\t\t%" B_PRIu32 " bytes (%" B_PRId32 " blocks, %"
			B_PRId32 " per allocation group)\n", disk.BlockSize() * size, size,
			disk.SuperBlock()->blocks_per_ag);
		printf("  Allocation Groups:\t%" B_PRIu32 "\n\n",
			disk.AllocationGroups());
		dump_block_run("  Log:\t\t\t", disk.Log());
		printf("    (was %s)\n\n", disk.SuperBlock()->flags == SUPER_BLOCK_CLEAN
			? "cleanly unmounted" : "not unmounted cleanly!");
		dump_block_run("  Root Directory:\t", disk.Root());
		putchar('\n');
	} else if (dumpSuperBlock) {
		dump_super_block(disk.SuperBlock());
		putchar('\n');
	}

	if (disk.ValidateSuperBlock() < B_OK) {
		fprintf(stderr, "The disk's superblock is corrupt (or it's not a BFS "
			"device)!\n");
		return 0;
	}

	if (dumpRootNode) {
		bfs_inode inode;
		if (disk.ReadAt(disk.ToOffset(disk.Root()), (void *)&inode,
				sizeof(bfs_inode)) < B_OK) {
			fprintf(stderr,"Could not read root node from disk!\n");
		} else {
			puts("Root node:\n-----------------------------------------");
			dump_inode(NULL, &inode, showOffsets);
			dump_indirect_stream(disk, &inode, showOffsets);
			putchar('\n');
		}
	}

	char buffer[disk.BlockSize()];
	bfs_inode* bfsInode = (bfs_inode*)buffer;
	block_run run;
	Inode *inode = NULL;

	if (dumpInode || dumpBTree || dumpHex || validateBTree || listBTree
		|| countBTree) {
		// Set the block_run to the right value (as specified on the command
		// line)
		if (!argv[1]) {
			fprintf(stderr, "The -i/b/f options need the allocation group and "
				"starting offset (or the block number) of the node to dump!\n");
			return -1;
		}
		run = parseBlockRun(disk, argv[1], argv[2]);

		if (disk.ReadAt(disk.ToOffset(run), buffer, disk.BlockSize()) <= 0) {
			fprintf(stderr,"Could not read node from disk!\n");
			return -1;
		}

		inode = Inode::Factory(&disk, bfsInode, false);
		if (inode == NULL || inode->InitCheck() < B_OK) {
			fprintf(stderr,"Not a valid inode!\n");
			delete inode;
			inode = NULL;
		}
	}

	if (dumpInode) {
		printf("Inode at block %" B_PRIdOFF ":\n------------------------------"
			"-----------\n", disk.ToBlock(run));
		dump_inode(inode, bfsInode, showOffsets);
		dump_indirect_stream(disk, bfsInode, showOffsets);
		dump_double_indirect_stream(disk, bfsInode, showOffsets);
		dump_small_data(inode);
		putchar('\n');
	}

	if (dumpBTree && inode != NULL) {
		printf("B+Tree at block %" B_PRIdOFF ":\n-----------------------------"
			"------------\n", disk.ToBlock(run));
		if (inode->IsDirectory() || inode->IsAttributeDirectory()) {
			dump_bplustree(disk, (Directory*)inode, inode->Size(), dumpHex);
			putchar('\n');
		} else
			fprintf(stderr, "Inode is not a directory!\n");
	}

	if (listBTree && inode != NULL) {
		printf("Directory contents: ------------------------------------------\n");
		if (inode->IsDirectory() || inode->IsAttributeDirectory()) {
			list_bplustree(disk, (Directory*)inode, inode->Size());
			putchar('\n');
		} else
			fprintf(stderr, "Inode is not a directory!\n");
	}

	if (countBTree && inode != NULL) {
		printf("Count contents: ------------------------------------------\n");
		if (inode->IsDirectory() || inode->IsAttributeDirectory()) {
			count_bplustree(disk, (Directory*)inode, inode->Size());
			putchar('\n');
		} else
			fprintf(stderr, "Inode is not a directory!\n");
	}

	if (validateBTree && inode != NULL) {
		printf("Validating B+Tree at block %" B_PRIdOFF ":\n------------------"
			"-----------------------\n", disk.ToBlock(run));
		if (inode->IsDirectory() || inode->IsAttributeDirectory()) {
			BPlusTree *tree;
			if (((Directory *)inode)->GetTree(&tree) == B_OK) {
				if (tree->Validate(true) < B_OK)
					puts("B+Tree is corrupt!");
				else
					puts("B+Tree seems to be okay.");
			}
		} else
			fprintf(stderr, "Inode is not a directory!\n");
	}

	if (dumpHex) {
		printf("Hexdump from inode at block %" B_PRIdOFF ":\n-----------------"
			"------------------------\n", disk.ToBlock(run));
		dump_block(buffer, disk.BlockSize());
		putchar('\n');
	}

	delete inode;

	return 0;
}

