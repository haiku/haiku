/*
 * Copyright (c) 2001-2008 pinc Software. All Rights Reserved.
 */

//!	recovers corrupt BFS disks


#include "Disk.h"
#include "Inode.h"
#include "Hashtable.h"
#include "BPlusTree.h"
#include "dump.h"

#include <String.h>
#include <fs_info.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>


extern const char *__progname;
static const char *kProgramName = __progname;

bool gCreateIndices = false;
bool gDumpMissingInodes = false;
bool gRawMode = false;
bool gVerbose = false;


class InodeHashtable {
	public:
		InodeHashtable(int capacity)
			:
			fHashtable(capacity),
			fLastChecked(0)
		{
			fHashtable.SetHashFunction((uint32 (*)(const void *))BlockRunHash);
			fHashtable.SetCompareFunction((bool (*)(const void *, const void *))
				BlockRunCompare);
		}

		Inode* Acquire(Inode* inode)
		{
			if (inode == NULL)
				return NULL;

			status_t status = inode->AcquireBuffer();
			if (status != B_OK) {
				fprintf(stderr, "Could not retrieve buffer for inode %Ld: %s\n",
					inode->Offset(), strerror(status));
				return NULL;
			}
			return inode;
		}

		void Release(Inode* inode)
		{
			inode->ReleaseBuffer();
		}

		Inode* Get(block_run run)
		{
			return Acquire((Inode *)fHashtable.GetValue(&run));
		}

		bool Insert(Inode* inode)
		{
			bool success = fHashtable.Put(&inode->BlockRun(), inode);
			if (success)
				inode->ReleaseBuffer();

			return success;
		}

		bool Contains(block_run *key)
		{
			return fHashtable.ContainsKey(key);
		}

		Inode* Remove(block_run *key)
		{
			return Acquire((Inode*)fHashtable.Remove(key));
		}

		status_t GetNextEntry(Inode **_inode)
		{
			status_t status = fHashtable.GetNextEntry((void**)_inode);
			if (status == B_OK) {
				if (Acquire(*_inode) == NULL)
					return B_NO_MEMORY;
			}

			return status;
		}

		void Rewind()
		{
			fHashtable.Rewind();
		}

		bool IsEmpty() const
		{
			return fHashtable.IsEmpty();
		}

		void MakeEmpty()
		{
			fHashtable.MakeEmpty(HASH_EMPTY_NONE, HASH_EMPTY_DELETE);
		}

		static uint32 BlockRunHash(const block_run *run)
		{
			return run->allocation_group << 16 | run->start;
		}

		static bool BlockRunCompare(const block_run *runA, const block_run *runB)
		{
			return *runA == *runB;
		}

	private:
		Hashtable	fHashtable;
		bigtime_t	fLastChecked;
		uint32		fPercentUsed;
};

class InodeGetter {
	public:
		InodeGetter(InodeHashtable& hashtable, block_run run)
		{
			fInode = hashtable.Get(run);
		}

		~InodeGetter()
		{
			if (fInode != NULL)
				fInode->ReleaseBuffer();
		}

		Inode* Node() { return fInode; }

	private:
		Inode*	fInode;
};


InodeHashtable gHashtable(1000);
	// contains all inodes found on disk in the general data area
InodeHashtable gLogged(50);
	// contains all inodes found in the log area
InodeHashtable gMissing(50);
InodeHashtable gMissingEmpty(25);


class HashtableInodeSource : public Inode::Source {
	public:
		virtual Inode *InodeAt(block_run run)
		{
			Inode *inode;
			if ((inode = gHashtable.Get(run)) != NULL)
				return inode;

			if ((inode = gLogged.Get(run)) != NULL)
				return inode;

			if ((inode = gMissing.Get(run)) != NULL)
				return inode;

			return NULL;
		}
};


void
collectInodes(Disk &disk, InodeHashtable &hashtable, off_t start, off_t end)
{
	char buffer[8192];
	Inode inode(&disk, (bfs_inode *)buffer, false);
	off_t count = 0LL;
	off_t position = start;

	bigtime_t lastUpdate = system_time();

	for (off_t offset = start; offset < end; offset += sizeof(buffer)) {
		if (disk.ReadAt(offset, buffer, sizeof(buffer)) < B_OK) {
			fprintf(stderr, "could not read from device!\n");
			break;
		}

		//if ((offset % (disk.BlockSize() << disk.SuperBlock()->ag_shift)) == 0)
		//	printf("reading block %Ld, allocation group %Ld, %Ld inodes...\33[1A\n", offset / disk.BlockSize(),offset / (disk.BlockSize() << disk.SuperBlock()->ag_shift), count);

		for (uint32 i = 0; i < sizeof(buffer); i += disk.BlockSize()) {
			inode.SetTo((bfs_inode *)(buffer + i));
			if (inode.InitCheck() == B_OK) {
				if (inode.Flags() & INODE_DELETED)
					continue;

				Inode *node = Inode::Factory(&disk, &inode);
				if (node != NULL) {
					if (gVerbose)
						printf("  node: %Ld \"%s\"\n", position, node->Name());
					hashtable.Insert(node);
					count++;
				} else if (gVerbose) {
					printf("\nunrecognized inode:");
					dump_inode(&inode, inode.InodeBuffer());
				}
			}
			position += disk.BlockSize();
		}
		if (system_time() - lastUpdate > 500000) {
			printf("  block %Ld (%Ld%%), %Ld inodes\33[1A\n", offset, 100 * (offset - start) / (end - start), count);
			lastUpdate = system_time();
		}
	}
	printf("\n%Ld inodes found.\n", count);

	Inode *node;
	off_t directories = 0LL;
	off_t directorySize = 0LL;
	off_t files = 0LL;
	off_t fileSize = 0LL;
	off_t symlinks = 0LL;
	count = 0LL;

	hashtable.Rewind();
	while (hashtable.GetNextEntry(&node) == B_OK) {
		if (node->IsDirectory()) {
			directories++;
			directorySize += node->Size();
		} else if (node->IsFile()) {
			files++;
			fileSize += node->Size();
		} else if (node->IsSymlink()) {
			symlinks++;
		}
		count++;
		hashtable.Release(node);
	}

	printf("\n%20Ld directories found (total of %Ld bytes)\n"
	   "%20Ld files found (total of %Ld bytes)\n"
	   "%20Ld symlinks found\n"
	   "--------------------\n"
	   "%20Ld inodes total found in hashtable.\n",
	   directories, directorySize, files, fileSize, symlinks, count);
}


void
collectLogInodes(Disk &disk)
{
	// scan log area
	off_t offset = disk.ToOffset(disk.Log());
	off_t end = offset + (disk.Log().length << disk.BlockShift());

	printf("\nsearching from %Ld to %Ld (log area)\n",offset,end);
	
	collectInodes(disk, gLogged, offset, end);
}


void
collectRealInodes(Disk &disk)
{
	// first block after bootblock, bitmap, and log
	off_t offset = disk.ToOffset(disk.Log()) + (disk.Log().length
		<< disk.BlockShift());
	off_t end = /*(17LL << disk.SuperBlock()->ag_shift);
	if (end > disk.NumBlocks())
		end = */disk.NumBlocks();
	end *= disk.BlockSize();

	printf("\nsearching from %Ld to %Ld (main area)\n", offset, end);

	collectInodes(disk, gHashtable, offset, end);
}


Directory *
getNameIndex(Disk &disk)
{
	InodeGetter getter(gHashtable, disk.Indices());
	Directory *indices = dynamic_cast<Directory *>(getter.Node());

	block_run run;
	if (indices && indices->FindEntry("name", &run) == B_OK)
		return dynamic_cast<Directory *>(gHashtable.Get(run));

	// search name index

	Inode *node;

	gHashtable.Rewind();
	for (; gHashtable.GetNextEntry(&node) == B_OK; gHashtable.Release(node)) {
		if (!node->IsIndex() || node->Name() == NULL)
			continue;
		if (!strcmp(node->Name(), "name") && node->Mode() & S_STR_INDEX) {
			return dynamic_cast<Directory *>(node);
		}
	}
	return NULL;
}


void
checkDirectoryContents(Disk& disk, Directory *dir)
{
	dir->Rewind();

	char name[B_FILE_NAME_LENGTH];
	block_run run;

	while (dir->GetNextEntry(name, &run) == B_OK) {
		if (run == dir->BlockRun() || run == dir->Parent()
			|| gHashtable.Contains(&run))
			continue;

		Inode *missing = gMissing.Get(run);
		if (missing != NULL) {
			if (missing->SetName(name) < B_OK) {
				fprintf(stderr, "setting name of missing node to "
					"\"%s\" failed!", name);
			}
			if (gVerbose) {
				printf("Set name of missing node (%ld, %d) to \"%s\" (%s)\n",
					run.allocation_group, run.start, missing->Name(), name);
			}

			missing->SetParent(dir->BlockRun());
		}
//		if (node->Mode() & S_INDEX_DIR)
//		{
//			if (node->Mode() & S_STR_INDEX)
//				printf("index directory (%ld, %d): \"%s\" is missing (%ld, %d, %d)\n",node->BlockRun().allocation_group,node->BlockRun().start,name,run.allocation_group,run.start,run.length);
//			else
//				printf("index directory (%ld, %d): key is missing (%ld, %d, %d)\n",node->BlockRun().allocation_group,node->BlockRun().start,run.allocation_group,run.start,run.length);
//		}
		else {
			// missing inode has not been found
			if (gVerbose) {
				printf("directory \"%s\" (%ld, %d): node \"%s\" is "
					"missing (%ld, %d, %d)\n", dir->Name(),
					dir->BlockRun().allocation_group,
					dir->BlockRun().start, name,
					run.allocation_group, run.start, run.length);
			}

			if ((missing = (Inode *)gLogged.Remove(&run)) != NULL) {
				// missing inode is in the log
				if (gVerbose)
					printf("found missing entry in log!\n");
				if (missing->InodeBuffer()->parent != dir->BlockRun()) {
					if (gVerbose)
						puts("\tparent directories differ (may be an old version of it), reseting parent.");
					missing->SetParent(dir->BlockRun());
				}
				if (!gMissing.Insert(missing))
					delete missing;
			} else if (!gMissingEmpty.Contains(&run)) {
				// not known at all yet
				Inode *empty = Inode::EmptyInode(&disk, name, 0);
				if (empty) {
					empty->SetBlockRun(run);
					empty->SetParent(dir->BlockRun());
					if (gVerbose)
						printf("\tname = %s\n", empty->Name());

					if (!gMissingEmpty.Insert(empty))
						delete empty;
				}
			}
		}
	}
}


void
checkStructure(Disk &disk)
{
	Inode *node;

	off_t count = 0;
	gHashtable.Rewind();
	while (gHashtable.GetNextEntry(&node) == B_OK) {
		count++;
		if ((count % 50) == 0)
			fprintf(stderr, "%Ld inodes processed...\33[1A\n", count);

		if (node->IsDirectory() && !node->IsIndex()) {
			// check if all entries are in the hashtable
			checkDirectoryContents(disk, (Directory*)node);
		}

		// check for the parent directory

		block_run run = node->Parent();
		InodeGetter parentGetter(gHashtable, run);
		Inode *parentNode = parentGetter.Node();

		Directory *dir = dynamic_cast<Directory *>(parentNode);
		if (dir || parentNode && (node->Mode() & S_ATTR_DIR)) {
			// entry has a valid parent directory, so it's assumed to be a valid entry
			disk.BlockBitmap()->BackupSet(node, true);
		} else if (node->Mode() & S_ATTR) {
			if (gVerbose) {
				printf("attribute \"%s\" at %ld,%d misses its parent.\n",
					node->Name(), node->BlockRun().allocation_group,
					node->BlockRun().start);
				puts("\thandling not yet implemented...");
			}
		} else /*if ((node->Flags() & INODE_DELETED) == 0)*/ {
			Inode* missing = gMissing.Get(run);
			dir = dynamic_cast<Directory *>(missing);

			if (missing == NULL) {
				if (gVerbose) {
					printf("%s \"%s\" (%ld, %d, mode = %10lo): parent directory is "
						"missing (%ld, %d, %d), may be deleted!\n",
						node->IsFile() ? "file" : "node", node->Name(),
						node->BlockRun().allocation_group, node->BlockRun().start,
						node->Mode(),run.allocation_group, run.start, run.length);
				}

				if ((dir = dynamic_cast<Directory *>((Inode *)gLogged.Remove(
						&run))) != NULL) {
					if (gVerbose) {
						printf("found directory \"%s\" in log:\n", dir->Name());
						if (dir->Size() > 0)
							dump_inode(dir, dir->InodeBuffer());
						else
							puts("\tempty inode.");
					}
				} else {
					if (gVerbose)
						puts("\tcreate parent missing entry");

					Inode *nameNode = (Inode *)gMissingEmpty.Remove(&run);
					if (nameNode != NULL) {
						nameNode->SetMode(S_IFDIR);
						if (gVerbose)
							printf("found missing name!\n");
					} else {
						BString parentName;
						parentName << "__directory " << run.allocation_group
							<< ":" << (int32)run.start;

						nameNode = Inode::EmptyInode(&disk, parentName.String(),
							S_IFDIR);
						if (nameNode) {
							nameNode->SetBlockRun(run);
							nameNode->SetParent(disk.Root());
						}
					}

					if (nameNode) {
						dir = new Directory(*nameNode);
						if (dir->CopyBuffer() < B_OK)
							puts("could not copy buffer!");
						else
							delete nameNode;
					}
				}
				if (dir) {
					dir->AcquireBuffer();

					if (!gMissing.Insert(dir)) {
						printf("could not add dir!!\n");
						delete dir;
						dir = NULL;
					}
				}
			} else if (missing != NULL && dir == NULL && gVerbose) {
				printf("%s \"%s\" (%ld, %d, mode = %10lo): parent directory "
					"found in missing list (%ld, %d, %d), but it's not a dir!\n",
					node->IsFile() ? "file" : "node", node->Name(),
					node->BlockRun().allocation_group, node->BlockRun().start,
					node->Mode(),run.allocation_group, run.start, run.length);
			} else if (gVerbose) {
				printf("%s \"%s\" (%ld, %d, mode = %10lo): parent directory "
					"found in missing list (%ld, %d, %d)!\n",
					node->IsFile() ? "file" : "node", node->Name(),
					node->BlockRun().allocation_group, node->BlockRun().start,
					node->Mode(),run.allocation_group, run.start, run.length);
			}

			if (dir) {
				dir->AddEntry(node);
				dir->ReleaseBuffer();
			}
		}
//			else
//			{
//				printf("node %s\n",node->Name());
//				status_t status = dir->Contains(node);
//				if (status == B_ENTRY_NOT_FOUND)
//					printf("node \"%s\": parent directory \"%s\" contains no link to this node!\n",node->Name(),dir->Name());
//				else if (status != B_OK)
//					printf("node \"%s\": parent directory \"%s\" error: %s\n",node->Name(),dir->Name(),strerror(status));
//			}

		// check for attributes	

		run = node->Attributes();
		if (!run.IsZero()) {
			//printf("node \"%s\" (%ld, %d, mode = %010lo): has attribute dir!\n",node->Name(),node->BlockRun().allocation_group,node->BlockRun().start,node->Mode());

			if (!gHashtable.Contains(&run)) {
				if (gVerbose) {
					printf("node \"%s\": attributes are missing (%ld, %d, %d)\n",
						node->Name(), run.allocation_group, run.start, run.length);
				}

				if ((dir = (Directory *)gMissing.Get(run)) != NULL) {
					if (gVerbose)
						puts("\tfound in missing");
					dir->SetMode(dir->Mode() | S_ATTR_DIR);
					dir->SetParent(node->BlockRun());
				} else {
					if (gVerbose)
						puts("\tcreate new!");

					Inode *empty = Inode::EmptyInode(&disk, NULL,
						S_IFDIR | S_ATTR_DIR);
					if (empty) {
						empty->SetBlockRun(run);
						empty->SetParent(node->BlockRun());

						dir = new Directory(*empty);
						if (dir->CopyBuffer() < B_OK)
							puts("could not copy buffer!");
						else
							delete empty;

						if (!gMissing.Insert(dir)) {
							puts("\tcould not add attribute dir");
							delete dir;
						}
					}
				}
			}
		}
	}
	printf("%Ld inodes processed.\n", count);

	Directory *directory = getNameIndex(disk);
	if (directory != NULL) {
		puts("\n*** Search names of missing inodes in the name index");

		BPlusTree *tree;
		if (directory->GetTree(&tree) == B_OK && tree->Validate(gVerbose) == B_OK) {
			char name[B_FILE_NAME_LENGTH];
			block_run run;
			directory->Rewind();
			while (directory->GetNextEntry(name, &run) >= B_OK) {
				if ((node = gMissing.Get(run)) == NULL)
					continue;

				if (gVerbose) {
					printf("  Node found: %ld:%d\n", run.allocation_group,
						run.start);
				}
				if (node->Name() == NULL || strcmp(node->Name(), name)) {
					if (gVerbose) {
						printf("\tnames differ: %s -> %s (indices)\n",
							node->Name(), name);
					}
					node->SetName(name);
				}
			}
		} else
			printf("\tname index is corrupt!\n");

		directory->ReleaseBuffer();
	} else
		printf("*** Name index corrupt or not existent!\n");

	if (!gVerbose)
		return;

	if (!gMissing.IsEmpty())
		puts("\n*** Missing inodes:");

	gMissing.Rewind();
	while (gMissing.GetNextEntry(&node) == B_OK) {
		if (gDumpMissingInodes)
			dump_inode(node, node->InodeBuffer());

		Directory *dir = dynamic_cast<Directory *>(node);
		if (dir) {
			printf("\ndirectory \"%s\" (%ld, %d) contents:\n",
				node->Name(), node->BlockRun().allocation_group,
				node->BlockRun().start);

			dir->Rewind();

			char name[B_FILE_NAME_LENGTH];
			block_run run;
			while (dir->GetNextEntry(name, &run) == B_OK) {
				printf("\t\"%s\" (%ld, %d, %d)\n", name,
					run.allocation_group, run.start, run.length);	
			}

			BPlusTree *tree;
			if (dir->GetTree(&tree) < B_OK)
				continue;

			uint16 length;
			off_t offset;

			while (tree->GetNextEntry(name, &length, B_FILE_NAME_LENGTH,
					&offset) == B_OK) {
				name[length] = 0;

				run = disk.ToBlockRun(offset);
				printf("%s: block_run == (%5ld,%5d,%5d), \"%s\"\n", dir->Name(),
					run.allocation_group, run.start, run.length, name);
			}

			//tree->WriteTo(dir);
			//disk.WriteAt(dir->Offset(),dir->InodeBuffer(),disk.BlockSize());
		}

		gMissing.Release(node);
	}
}


void
copyInodes(const char *copyTo)
{
	if (!copyTo)
		return;

	Inode::Source *source = new HashtableInodeSource;
	Inode *node;

	int32 count = 0;

	gHashtable.Rewind();
	while (gHashtable.GetNextEntry(&node) == B_OK) {
		if (!node->IsIndex() && !node->IsAttributeDirectory())
			node->CopyTo(copyTo, source);

		if ((++count % 500) == 0)
			fprintf(stderr, "copied %ld files...\n", count);

		gHashtable.Release(node);
	}

	gMissing.Rewind();
	while (gMissing.GetNextEntry(&node) == B_OK) {
		if (!node->IsIndex() && !node->IsAttributeDirectory())
			node->CopyTo(copyTo, source);

		gHashtable.Release(node);
	}
}


void
usage(char *name)
{
	fprintf(stderr,"usage: %s [-idv] [-r [start-offset] [end-offset]] <device> [recover-to-path]\n"
		"\t-i\trecreate indices on target disk\n"
		"\t-d\tdump missing and recreated i-nodes\n"
		"\t-r\tdisk access in raw mode (use only if the partition table is invalid)\n"
		"\t-s\trecreate super block and exit (for experts only, don't use this\n"
		"\t\tif you don't know what you're doing)\n"
		"\t-v\tverbose output\n", name);
	exit(-1);
}


int
main(int argc, char **argv)
{
	char *fileName = strrchr(argv[0], '/');
	fileName = fileName ? fileName + 1 : argv[0];
	bool recreateSuperBlockOnly = false;

	off_t startOffset = 0, endOffset = -1;

	puts("Copyright (c) 2001-2008 pinc Software.");

	if (argc < 2 || !strcmp(argv[1], "--help"))
		usage(fileName);

	while (*++argv) {
		char *arg = *argv;
		if (*arg == '-') {
			while (*++arg && isalpha(*arg)) {
				switch (arg[0]) {
					case 'r':
					{
						gRawMode = true;

						if (argv[1] && isdigit((argv[1])[0])) {
							argv++;
							arg = *argv;

							startOffset = atoll(arg);
						}
						if (argv[1] && isdigit((argv[1])[0])) {
							argv++;
							arg = *argv;

							endOffset = atoll(arg);
						}
						if (endOffset != -1 && endOffset < startOffset)
							usage(fileName);
						break;
					}
					case 'v':
						gVerbose = true;
						break;
					case 'i':
						gCreateIndices = true;
						break;
					case 'd':
						gDumpMissingInodes = true;
						break;
					case 's':
						recreateSuperBlockOnly = true;
						break;
				}
			}
		} else
			break;
	}

	Disk disk(argv[0], gRawMode, startOffset, endOffset);
	if (disk.InitCheck() < B_OK) {
		fprintf(stderr,"Could not open device or file: %s\n",
			strerror(disk.InitCheck()));
		return -1;
	}

	if (argv[1] != NULL) {
		dev_t device = dev_for_path(argv[1]);
		fs_info info;
		if (fs_stat_dev(device, &info) == B_OK) {
			if (!strcmp(info.device_name, disk.Path().Path())) {
				fprintf(stderr,"The source and target device are identical, "
					"you currently need\n"
					"to have another disk to recover to, sorry!\n");
				return -1;
			}
			if ((info.flags & (B_FS_IS_PERSISTENT | B_FS_HAS_ATTR))
					!= (B_FS_IS_PERSISTENT | B_FS_HAS_ATTR)) {
				fprintf(stderr, "%s: The target file system (%s) does not have "
					"the required\n"
					"\tfunctionality in order to restore all information.\n",
					kProgramName, info.fsh_name);
				return -1;
			}
		}
	}

	bool recreatedSuperBlock = false;

	if (disk.ValidateSuperBlock() < B_OK) {
		fprintf(stderr, "The disk's super block is corrupt!\n");
		if (disk.RecreateSuperBlock() < B_OK) {
			fprintf(stderr, "Can't recreate the disk's super block, sorry!\n");
			return -1;
		}
		recreatedSuperBlock = true;
	}
	if (gVerbose) {
		puts("\n*** The super block:\n");
		dump_super_block(disk.SuperBlock());
	}

	if (recreateSuperBlockOnly) {
		if (!recreatedSuperBlock) {
			printf("Superblock was valid, no changes made.\n");
			return 0;
		}

		status_t status = disk.WriteAt(512, disk.SuperBlock(),
			sizeof(disk_super_block));
		if (status < B_OK) {
			fprintf(stderr, "Could not write super block: %s!\n",
				strerror(status));
			return 1;
		}
		return 0;
	}

	puts("\n*** Collecting inodes...");

	collectLogInodes(disk);
	collectRealInodes(disk);

	puts("\n*** Checking Disk Structure Integrity...");

	checkStructure(disk);

	if (argv[1])
		copyInodes(argv[1]);

	//disk.WriteBootBlock();
	//disk.BlockBitmap()->CompareWithBackup();

	gHashtable.MakeEmpty();
	gMissing.MakeEmpty();
	gLogged.MakeEmpty();

	return 0;
}

