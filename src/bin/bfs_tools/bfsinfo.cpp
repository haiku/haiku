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
	while ((int32)node < (int32)buffer + size) {
		printf("\n\n-------------------\n"
			"** node at offset: %ld\n** used: %ld bytes\n",
			int32(node) - int32(buffer), node->Used());
		dump_bplustree_node(node, header, &disk);

		if (hexDump) {
			putchar('\n');
			dump_block((char *)node, header->node_size, 0);
		}

		node = (bplustree_node *)(int32(node) + nodeSize);
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

		printf("  indirect[%02ld]              = ", i);

		char buffer[256];
		if (showOffsets)
			snprintf(buffer, sizeof(buffer), " %16lld", offset);
		else
			buffer[0] = '\0';

		dump_block_run("", runs[i], buffer);

		offset += runs[i].length * disk.BlockSize();
	}
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
				"\t-s\tdump super block\n"
				"\t-r\tdump root node\n"
				"       the following options need the allocation_group/start "
					"parameters:\n"
				"\t-i\tdump inode\n"
				"\t-b\tdump b+tree\n"
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
		&& !dumpHex) {
		printf("  Name:\t\t\t\"%s\"\n", disk.SuperBlock()->name);
		printf("    (disk is %s)\n\n",
			disk.ValidateSuperBlock() == B_OK ? "valid" : "invalid!!");
		printf("  Block Size:\t\t%ld bytes\n", disk.BlockSize());
		printf("  Number of Blocks:\t%12Lu\t%10g MB\n", disk.NumBlocks(),
			disk.NumBlocks() * disk.BlockSize() / (1024.0*1024));
		if (disk.BlockBitmap() != NULL) {
			printf("  Used Blocks:\t\t%12Lu\t%10g MB\n",
				disk.BlockBitmap()->UsedBlocks(),
				disk.BlockBitmap()->UsedBlocks() * disk.BlockSize()
					/ (1024.0*1024));
			printf("  Free Blocks:\t\t%12Lu\t%10g MB\n",
				disk.BlockBitmap()->FreeBlocks(),
				disk.BlockBitmap()->FreeBlocks() * disk.BlockSize()
					/ (1024.0*1024));
		}
		int32 size
			= (disk.AllocationGroups() * disk.SuperBlock()->blocks_per_ag);
		printf("  Bitmap Size:\t\t%ld bytes (%ld blocks, %ld per allocation "
			"group)\n", disk.BlockSize() * size, size,
			disk.SuperBlock()->blocks_per_ag);
		printf("  Allocation Groups:\t%lu\n\n", disk.AllocationGroups());
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
		fprintf(stderr, "The disk's super block is corrupt (or it's not a BFS "
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
	block_run run;
	Inode *inode = NULL;

	if (dumpInode || dumpBTree || dumpHex || validateBTree) {
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

		inode = Inode::Factory(&disk, (bfs_inode *)buffer, false);
		if (inode == NULL || inode->InitCheck() < B_OK) {
			fprintf(stderr,"Not a valid inode!\n");
			delete inode;
			inode = NULL;
		}
	}

	if (dumpInode) {
		printf("Inode at block %Ld:\n-----------------------------------------"
			"\n", disk.ToBlock(run));
		dump_inode(inode, (bfs_inode *)buffer, showOffsets);
		dump_indirect_stream(disk, (bfs_inode *)buffer, showOffsets);
		dump_small_data(inode);
		putchar('\n');
	}

	if (dumpBTree && inode) {
		printf("B+Tree at block %Ld:\n----------------------------------------"
			"-\n", disk.ToBlock(run));
		if (inode->IsDirectory() || inode->IsAttributeDirectory()) {
			dump_bplustree(disk, (Directory *)inode, inode->Size(), dumpHex);
			putchar('\n');
		} else
			fprintf(stderr, "Inode is not a directory!\n");
	}

	if (validateBTree && inode) {
		printf("Validating B+Tree at block %Ld:\n-----------------------------"
			"------------\n", disk.ToBlock(run));
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
		printf("Hexdump from inode at block %Ld:\n-----------------------------"
			"------------\n", disk.ToBlock(run));
		dump_block(buffer, disk.BlockSize());
		putchar('\n');
	}

	delete inode;

	return 0;
}

