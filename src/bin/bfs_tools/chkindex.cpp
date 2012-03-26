/*
 * Copyright (c) 2001-2008 pinc Software. All Rights Reserved.
 */

//!	sanity and completeness check for indices

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


char gEscape[3] = {0x1b, '[', 0};
off_t gCount = 1;
bool gDoNotCheckForFiles = false;
bool gDoNotCheckIndex = false;
bool gCheckAll = false;


// we just want to know which inodes exist, so don't remember anything
// along the position of the inode.

class BlockRunHashtable : public Hashtable
{
	public:
		BlockRunHashtable(int capacity)
			: Hashtable(capacity)
		{
			SetHashFunction((uint32 (*)(const void *))BlockRunHash);
			SetCompareFunction((bool (*)(const void *, const void *))BlockRunCompare);
		}

		bool Contains(block_run *run)
		{
			return ContainsKey((void *)run);
		}

		bool Put(block_run &run)
		{
			block_run *value = (block_run *)malloc(sizeof(block_run));
			if (!value)
				return false;

			memcpy(value,&run,sizeof(block_run));
			
			if (Hashtable::Put(value,value))
				return true;
			
			free(value);
			return false;
		}

		static uint32 BlockRunHash(const block_run *run)
		{
			return run->allocation_group << 16 | run->start;
		}

		static bool BlockRunCompare(const block_run *runA, const block_run *runB)
		{
			return *runA == *runB;
		}
};


BlockRunHashtable gHashtable(1000);


int
compareBlockRuns(const block_run *a, const block_run *b)
{
	int cmp = (int)a->allocation_group - (int)b->allocation_group;
	if (cmp == 0)
		cmp = (int)a->start - (int)b->start;
	return cmp;
}


void
collectFiles(Disk &disk,Directory *directory)
{
	if (directory == NULL)
		return;

	directory->Rewind();
	char name[B_FILE_NAME_LENGTH];
	block_run run;
	while (directory->GetNextEntry(name,&run) >= B_OK)
	{
		if (!strcmp(name,".") || !strcmp(name,".."))
			continue;

		gHashtable.Put(run);

		if (++gCount % 50 == 0)
			printf("  %7Ld%s1A\n",gCount,gEscape);

		Inode *inode = Inode::Factory(&disk,run);
		if (inode != NULL)
		{
			if (inode->IsDirectory())
				collectFiles(disk,static_cast<Directory *>(inode));

			delete inode;
		}
		else
			printf("  Directory \"%s\" (%ld, %d) points to corrupt inode \"%s\" (%ld, %d)\n",
				directory->Name(),directory->BlockRun().allocation_group,directory->BlockRun().start,
				name,run.allocation_group,run.start);
	}
}


void
collectFiles(Disk &disk)
{
	Directory *root = (Directory *)Inode::Factory(&disk,disk.Root());

	puts("Collecting files (this will take some time)...");

	if (root == NULL || root->InitCheck() < B_OK)
	{
		fprintf(stderr,"  Could not open root directory!\n");
		return;
	}
	collectFiles(disk,root);
	
	printf("  %7Ld files found.\n",gCount);

	delete root;
}


void
checkIndexForNonExistingFiles(Disk &disk,BPlusTree &tree)
{
	char name[B_FILE_NAME_LENGTH];
	uint16 length;
	off_t offset;

	while (tree.GetNextEntry(name,&length,B_FILE_NAME_LENGTH,&offset) == B_OK)
	{
		name[length] = 0;
		block_run run = disk.ToBlockRun(offset);
		if (!gHashtable.Contains(&run))
		{
			printf("  inode at (%ld, %d), offset %Ld, doesn't exist!",run.allocation_group,run.start,offset);
			switch (tree.Type())
			{
				case BPLUSTREE_STRING_TYPE:
					printf(" (string = \"%s\")",name);
					break;
				case BPLUSTREE_INT32_TYPE:
					printf(" (int32 = %ld)",*(int32 *)&name);
					break;
				case BPLUSTREE_UINT32_TYPE:
					printf(" (uint32 = %lu)",*(uint32 *)&name);
					break;
				case BPLUSTREE_INT64_TYPE:
					printf(" (int64 = %Ld)",*(int64 *)&name);
					break;
				case BPLUSTREE_UINT64_TYPE:
					printf(" (uint64 = %Lu)",*(uint64 *)&name);
					break;
				case BPLUSTREE_FLOAT_TYPE:
					printf(" (float = %g)",*(float *)&name);
					break;
				case BPLUSTREE_DOUBLE_TYPE:
					printf(" (double = %g)",*(double *)&name);
					break;
			}
			putchar('\n');
		}
	}
}


void
checkFiles(Disk &disk,BPlusTree &tree,char *attribute)
{
	block_run *runs = (block_run *)malloc(gCount * sizeof(block_run));
	if (runs == NULL)
	{
		fprintf(stderr,"  Not enough memory!\n");
		return;
	}
	// copy hashtable to array
	block_run *run = NULL;
	int32 index = 0;
	gHashtable.Rewind();
	while (gHashtable.GetNextEntry((void **)&run) == B_OK)
	{
		runs[index++] = *run;
	}
	
	// sort array to speed up disk access
	qsort(runs,index,sizeof(block_run),(int (*)(const void *,const void *))compareBlockRuns);

	bool sizeIndex = !strcmp(attribute,"size");
	bool nameIndex = !strcmp(attribute,"name");
	bool modifiedIndex = !strcmp(attribute,"last_modified");

	char key[B_FILE_NAME_LENGTH];
	uint16 keyLength = 0;

	Inode *inode = NULL;

	for (int32 i = 0;i < index;i++)
	{
		if (i % 50 == 0)
			printf("  %7ld%s1A\n",i,gEscape);

		delete inode;
		inode = Inode::Factory(&disk,runs[i]);
		if (inode == NULL || inode->InitCheck() < B_OK)
		{
			fprintf(stderr,"  inode at (%ld, %d) is corrupt!\n",runs[i].allocation_group,runs[i].start);
			delete inode;
			continue;
		}
		
		// check indices not based on standard attributes
		if (sizeIndex)
		{
			if (inode->IsDirectory())
				continue;

			memcpy(key,&inode->InodeBuffer()->data.size,sizeof(off_t));
			keyLength = sizeof(off_t);
		}
		else if (nameIndex)
		{
			strcpy(key,inode->Name());
			keyLength = strlen(key);
		}
		else if (modifiedIndex)
		{
			if (inode->IsDirectory())
				continue;

			memcpy(key,&inode->InodeBuffer()->last_modified_time,sizeof(off_t));
			keyLength = sizeof(off_t);
		}
		else	// iterate through all attributes to find the right one (damn slow, sorry...)
		{
			inode->RewindAttributes();
			char name[B_FILE_NAME_LENGTH];
			uint32 type;
			void *data;
			size_t length;
			bool found = false;
			while (inode->GetNextAttribute(name,&type,&data,&length) == B_OK)
			{
				if (!strcmp(name,attribute))
				{
					strncpy(key,(char *)data,B_FILE_NAME_LENGTH - 1);
					key[B_FILE_NAME_LENGTH - 1] = '\0';
					keyLength = length > B_FILE_NAME_LENGTH ? B_FILE_NAME_LENGTH : length;
					found = true;
					break;
				}
			}
			if (!found)
				continue;
		}

		off_t value;
		if (tree.Find((uint8 *)key,keyLength,&value) < B_OK)
		{
			if (*inode->Name())
				fprintf(stderr,"  inode at (%ld, %d) name \"%s\" is not in index!\n",runs[i].allocation_group,runs[i].start,inode->Name());
			else
			{
				// inode is obviously deleted!
				block_run parent = inode->Parent();
				Directory *directory = (Directory *)Inode::Factory(&disk,parent);
				if (directory != NULL && directory->InitCheck() == B_OK)
				{
					BPlusTree *parentTree;
					if (directory->GetTree(&parentTree) == B_OK)
					{
						char name[B_FILE_NAME_LENGTH];
						uint16 length;
						off_t offset,searchOffset = disk.ToBlock(runs[i]);
						bool found = false;

						while (parentTree->GetNextEntry(name,&length,B_FILE_NAME_LENGTH,&offset) == B_OK)
						{
							if (offset == searchOffset)
							{
								fprintf(stderr,"  inode at (%ld, %d) name \"%s\" was obviously deleted, but the parent \"%s\" still contains it!\n",runs[i].allocation_group,runs[i].start,name,directory->Name());
								found = true;
								break;
							}
						}
						if (!found)
							fprintf(stderr,"  inode at (%ld, %d) was obviously deleted, and the parent \"%s\" obviously doesn't contain it anymore!\n",runs[i].allocation_group,runs[i].start,directory->Name());
					}
					else
						fprintf(stderr,"  inode at (%ld, %d) was obviously deleted, but the parent \"%s\" is invalid and still contains it!\n",runs[i].allocation_group,runs[i].start,directory->Name());
				}
				else
				{
					// not that this would be really possible... - but who knows
					fprintf(stderr,"  inode at (%ld, %d) is not in index and has invalid parent!\n",runs[i].allocation_group,runs[i].start);
				}
				delete directory;
			}
		}
		else
		{
			if (bplustree_node::LinkType(value) == BPLUSTREE_NODE)
			{
				if (disk.ToBlockRun(value) != runs[i])
					fprintf(stderr,"  offset in index and inode offset doesn't match for inode \"%s\" at (%ld, %d)\n",inode->Name(),runs[i].allocation_group,runs[i].start);
			}
			else
			{
				// search duplicates
				char name[B_FILE_NAME_LENGTH];
				uint16 length;
				off_t offset;
				bool found = false,duplicates = false;
//puts("++");
				tree.Rewind();
				while (tree.GetNextEntry(name,&length,B_FILE_NAME_LENGTH,&offset) == B_OK)
				{
					//printf("search for = %ld, key = %ld -> value = %Ld (%ld, %d)\n",*(int32 *)&key,*(int32 *)&name,offset,disk.ToBlockRun(offset).allocation_group,disk.ToBlockRun(offset).start);
					if (keyLength == length && !memcmp(key,name,keyLength))
					{
						duplicates = true;
						if (disk.ToBlockRun(offset) == runs[i])
						{
							found = true;
							break;
						}
					}
					//else if (duplicates)
					//	break;
				}
				if (!found)
				{
					printf("  inode \"%s\" at (%ld, %d) not found in duplicates!\n",inode->Name(),runs[i].allocation_group,runs[i].start);
//					return;
				}
			}
		}
	}
	delete inode;
	printf("  %7Ld files processed.\n",gCount);
}


int
checkIndex(Disk &disk,char *attribute,block_run &run,bool collect)
{
	Directory *index = (Directory *)Inode::Factory(&disk,run);
	status_t status;
	if (index == NULL || (status = index->InitCheck()) < B_OK)
	{
		fprintf(stderr,"  Could not get index directory for \"%s\": %s!\n",attribute,index ? strerror(status) : "not found/corrupted");
		return -1;
	}

	printf("\nCheck \"%s\" index's on-disk structure...\n",attribute);
	//dump_inode(index->InodeBuffer());

	BPlusTree *tree;
	if (index->GetTree(&tree) < B_OK || tree->Validate(true) < B_OK)
	{
		fprintf(stderr,"  B+Tree of index \"%s\" seems to be corrupt!\n",attribute);
		//return -1;
	}

	if (collect && (!gDoNotCheckIndex || !gDoNotCheckForFiles))
		collectFiles(disk);

	if (!gDoNotCheckIndex)
	{
		printf("Check for non-existing files in index \"%s\"...\n",attribute);
		checkIndexForNonExistingFiles(disk,*tree);
	}

	if (!gDoNotCheckForFiles)
	{
		printf("Check for files not in index \"%s\" (this may take even more time)...\n",attribute);
		checkFiles(disk,*tree,attribute);
	}
	return 0;
}


void
printUsage(char *tool)
{
	char *filename = strrchr(tool,'/');
	fprintf(stderr,"usage: %s [-ifa] index-name\n"
			"\t-i\tdo not check for non-existing files in the index\n"
			"\t-f\tdo not check if all the files are in the index\n"
			"\t-a\tcheck all indices (could take some weeks...)\n"
			,filename ? filename + 1 : tool);
}


int
main(int argc,char **argv)
{
	puts("Copyright (c) 2001-2008 pinc Software.");

	char *toolName = argv[0];
	if (argc < 2 || !strcmp(argv[1],"--help"))
	{
		printUsage(toolName);
		return -1;
	}
	
	while (*++argv)
	{
		char *arg = *argv;
		if (*arg == '-')
		{
			while (*++arg && isalpha(*arg))
			{
				switch (*arg)
				{
					case 'i':
						gDoNotCheckIndex = true;
						break;
					case 'f':
						gDoNotCheckForFiles = true;
						break;
					case 'a':
						gCheckAll = true;
						break;
				}
			}
		}
		else
			break;
	}
	
	char *attribute = argv[0];
	if (!gCheckAll && attribute == NULL)
	{
		printUsage(toolName);
		return -1;
	}

	dev_t device = dev_for_path(".");
	if (device < B_OK)
	{
		fprintf(stderr,"Could not find device for current location: %s\n",strerror(device));
		return -1;
	}

	fs_info info;
	if (fs_stat_dev(device,&info) < B_OK)
	{
		fprintf(stderr,"Could not get stats for device: %s\n",strerror(errno));
		return -1;
	}

	Disk disk(info.device_name);
	status_t status;
	if ((status = disk.InitCheck()) < B_OK)
	{
		fprintf(stderr,"Could not open device or file \"%s\": %s\n",info.device_name,strerror(status));
		return -1;
	}
	
	if (disk.ValidateSuperBlock() < B_OK)
	{
		fprintf(stderr,"The disk's super block is corrupt!\n");
		return -1;
	}
	
	Directory *indices = (Directory *)Inode::Factory(&disk,disk.Indices());
	if (indices == NULL || (status = indices->InitCheck()) < B_OK)
	{
		fprintf(stderr,"  Could not get indices directory: %s!\n",indices ? strerror(status) : "not found/corrupted");
		delete indices;
		return -1;
	}
	BPlusTree *tree;
	if (indices->GetTree(&tree) < B_OK || tree->Validate() < B_OK)
	{
		fprintf(stderr,"  Indices B+Tree seems to be corrupt!\n");
		delete indices;
		return -1;
	}
	
	block_run run;

	if (gCheckAll)
	{
		putchar('\n');
		collectFiles(disk);

		char name[B_FILE_NAME_LENGTH];
		while (indices->GetNextEntry(name,&run) >= B_OK)
			checkIndex(disk,name,run,false);
	}
	else if (indices->FindEntry(attribute,&run) == B_OK)
		checkIndex(disk,attribute,run,true);
	else
		fprintf(stderr,"  Could not find index directory for \"%s\"!\n",attribute);

	delete indices;
	
	gHashtable.MakeEmpty(HASH_EMPTY_NONE,HASH_EMPTY_FREE);

	return 0;
}

