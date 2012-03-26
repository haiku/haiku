/*
 * Copyright (c) 2002-2009 pinc Software. All Rights Reserved.
 */

//!	Finds out to which file(s) a particular block belongs


#include "Bitmap.h"
#include "Disk.h"
#include "Inode.h"
#include "Hashtable.h"
#include "BPlusTree.h"
#include "dump.h"

#include <fs_info.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>


void scanNodes(Disk& disk, Directory* directory, const char* name,
	const block_run& checkForRun);


char gEscape[3] = {0x1b, '[', 0};
off_t gCount = 1;


bool
checkForBlockRunIntersection(const block_run& check, const block_run& against)
{
	if (check.allocation_group == against.allocation_group
		&& check.start + check.length > against.start
		&& check.start < against.start + against.length)
		return true;

	return false;
}


bool
checkNode(Disk &disk, Inode *inode, block_run checkForRun)
{
	// check the inode space itself
	if (checkForBlockRunIntersection(inode->BlockRun(), checkForRun))
		return true;

	// the data stream of symlinks is no real data stream	
	if (inode->IsSymlink() && (inode->Flags() & INODE_LONG_SYMLINK) == 0)
		return false;

	// direct blocks

	const data_stream* data = &inode->InodeBuffer()->data;

	if (data->max_direct_range == 0)
		return false;

	for (int32 i = 0; i < NUM_DIRECT_BLOCKS; i++) {
		if (data->direct[i].IsZero())
			break;

		if (checkForBlockRunIntersection(data->direct[i], checkForRun))
			return true;
	}

	// indirect blocks

	if (data->max_indirect_range == 0 || data->indirect.IsZero())
		return false;

	if (checkForBlockRunIntersection(data->indirect, checkForRun))
		return true;

	DataStream *stream = dynamic_cast<DataStream *>(inode);
	if (stream == NULL)
		return false;

	// load indirect blocks
	int32 bytes = data->indirect.length << disk.BlockShift();
	block_run* indirect = (block_run*)malloc(bytes);
	if (indirect == NULL)
		return false;
	if (disk.ReadAt(disk.ToOffset(data->indirect), indirect, bytes) <= 0)
		return false;

	int32 runs = bytes / sizeof(block_run);
	for (int32 i = 0; i < runs; i++) {
		if (indirect[i].IsZero())
			break;

		if (checkForBlockRunIntersection(indirect[i], checkForRun))
			return true;
	}
	free(indirect);

	// double indirect blocks

	if (data->max_double_indirect_range == 0 || data->double_indirect.IsZero())
		return false;

	if (checkForBlockRunIntersection(data->double_indirect, checkForRun))
		return true;

	// TODO: to be implemented...

	return false;
}


void
scanNode(Disk& disk, Inode* inode, const char* name,
	const block_run& checkForRun)
{
	if (checkNode(disk, inode, checkForRun)) {
		printf("Inode at (%ld, %u, %u) \"%s\" intersects\n",
			inode->BlockRun().allocation_group, inode->BlockRun().start,
			inode->BlockRun().length, name);
	}

	if (!inode->Attributes().IsZero()) {
		Inode *attributeDirectory = Inode::Factory(&disk,
			inode->Attributes());
		if (attributeDirectory != NULL) {
			scanNodes(disk,
				dynamic_cast<Directory *>(attributeDirectory), "(attr-dir)",
				checkForRun);
		}

		delete attributeDirectory;
	}
}


void
scanNodes(Disk& disk, Directory* directory, const char* directoryName,
	const block_run& checkForRun)
{
	if (directory == NULL)
		return;

	scanNode(disk, directory, directoryName, checkForRun);

	directory->Rewind();
	char name[B_FILE_NAME_LENGTH];
	block_run run;
	while (directory->GetNextEntry(name, &run) == B_OK) {
		if (!strcmp(name, ".") || !strcmp(name, ".."))
			continue;

		if (++gCount % 50 == 0)
			printf("  %7Ld%s1A\n", gCount, gEscape);

		Inode *inode = Inode::Factory(&disk, run);
		if (inode != NULL) {

			if (inode->IsDirectory()) {
				scanNodes(disk, static_cast<Directory *>(inode), name,
					checkForRun);
			} else
				scanNode(disk, inode, name, checkForRun);

			delete inode;
		} else {
			printf("  Directory \"%s\" (%ld, %d) points to corrupt inode \"%s\" "
				"(%ld, %d)\n", directory->Name(),
				directory->BlockRun().allocation_group,directory
					->BlockRun().start,
				name, run.allocation_group, run.start);
		}
	}
}


void
scanNodes(Disk& disk, const block_run& checkForRun, bool scanAll)
{
	Directory* root = (Directory*)Inode::Factory(&disk, disk.Root());

	puts("Scanning nodes (this will take some time)...");

	if (root == NULL || root->InitCheck() != B_OK) {
		fprintf(stderr,"  Could not open root directory!\n");
		return;
	}

	scanNodes(disk, root, "(root)", checkForRun);

	delete root;

	if (scanAll) {
		Directory* indices = (Directory*)Inode::Factory(&disk, disk.Indices());

		puts("Scanning index nodes (this will take some time)...");

		scanNodes(disk, indices, "(indices)", checkForRun);

		delete indices;
	}

	printf("  %7Ld nodes found.\n", gCount);
}


void
testBitmap(Disk& disk, const block_run& run)
{
	Bitmap bitmap;
	status_t status = bitmap.SetTo(&disk);
	if (status != B_OK) {
		fprintf(stderr, "Bitmap initialization failed: %s\n", strerror(status));
		return;
	}

	printf("Block bitmap sees block %Ld as %s.\n", disk.ToBlock(run),
		bitmap.UsedAt(disk.ToBlock(run)) ? "used" : "free");
}


block_run
parseBlockRun(Disk& disk, char* first, char* last)
{
	char* comma;

	if (last != NULL)
		return block_run::Run(atol(first), atol(last), 1);

	if ((comma = strchr(first, ',')) != NULL) {
		*comma++ = '\0';
		return block_run::Run(atol(first), atol(comma));
	}

	return disk.ToBlockRun(atoll(first));
}


void
printUsage(char* tool)
{
	char* filename = strrchr(tool, '/');
	fprintf(stderr, "usage: %s <device> <allocation_group> <start>\n",
		filename ? filename + 1 : tool);
}


int
main(int argc, char** argv)
{
	puts("Copyright (c) 2001-2009 pinc Software.");

	char* toolName = argv[0];
	if (argc < 2 || !strcmp(argv[1], "--help")) {
		printUsage(toolName);
		return -1;
	}

	bool scanAll = false;

	while (*++argv) {
		char *arg = *argv;
		if (*arg == '-') {
			while (*++arg && isalpha(*arg)) {
				switch (*arg) {
					case 'a':
						scanAll = true;
						break;
					default:
						printUsage(toolName);
						return -1;
				}
			}
		} else
			break;
	}

	Disk disk(argv[0]);
	status_t status = disk.InitCheck();
	if (status < B_OK) {
		fprintf(stderr, "Could not open device or file \"%s\": %s\n",
			argv[0], strerror(status));
		return -1;
	}

	if (disk.ValidateSuperBlock() != B_OK) {
		fprintf(stderr, "The disk's super block is corrupt!\n");
		return -1;
	}

	// Set the block_run to the right value (as specified on the command line)
	if (!argv[1]) {
		fprintf(stderr, "Need the allocation group and starting offset (or the "
			"block number) of the block to search for!\n");
		return -1;
	}
	block_run run = parseBlockRun(disk, argv[1], argv[2]);
	off_t block = disk.ToBlock(run);

	putchar('\n');
	testBitmap(disk, run);

	if (checkForBlockRunIntersection(disk.Log(), run)) {
		printf("Log area (%lu.%u.%u) intersects\n",
			disk.Log().allocation_group, disk.Log().start,
			disk.Log().length);
	} else if (block < 1) {
		printf("Super Block intersects\n");
	} else if (block < 1 + disk.BitmapSize()) {
		printf("Block bitmap intersects (start %d, end %lu)\n", 1,
			disk.BitmapSize());
	} else
		scanNodes(disk, run, scanAll);

	return 0;
}

