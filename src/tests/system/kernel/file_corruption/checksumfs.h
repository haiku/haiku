/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CHECK_SUM_FS_H
#define CHECK_SUM_FS_H


#include <OS.h>


#define CHECK_SUM_FS_PRETTY_NAME	"CheckSum File System"


static const uint64 kCheckSumFSSuperBlockOffset = 16 * B_PAGE_SIZE;
static const uint64 kCheckSumFSMinSize
	= kCheckSumFSSuperBlockOffset + 16 * B_PAGE_SIZE;


static const uint32 kCheckSumFSNameLength		= 256;

static const uint32 kCheckSumFSSignatureLength	= 16;
#define CHECK_SUM_FS_SIGNATURE_1	"_1!cHEcKsUmfS!1_"
#define CHECK_SUM_FS_SIGNATURE_2	"-2@ChECkSumFs@2-"

static const uint32 kCheckSumFSVersion = 1;

struct checksumfs_super_block {
	char	signature1[kCheckSumFSSignatureLength];
	uint32	version;
	uint32	pad1;
	uint64	totalBlocks;
	uint64	freeBlocks;
	uint64	rootDir;
	uint64	blockBitmap;
	char	name[kCheckSumFSNameLength];
	char	signature2[kCheckSumFSSignatureLength];
} _PACKED;


struct checksumfs_node {
	uint32	mode;				// node type + permissions
	uint32	attributeType;		// attribute type (attributes only)
	uint32	uid;				// owning user ID
	uint32	gid;				// owning group ID
	uint64	creationTime;		// in ns since the epoche
	uint64	modificationTime;	//
	uint64	changeTime;			//
	uint64	hardLinks;			// number of references to the node
	uint64	size;				// content size in bytes
	uint64	parentDirectory;	// block index of the parent directory
								// (directories and attributes only)
	uint64	attributeDirectory;	// block index of the attribute directory (0 if
								// empty)
} _PACKED;


static const uint32 kCheckSumFSMaxDirEntryTreeDepth		= 24;

struct checksumfs_dir_entry_tree {
	uint16	depth;
} _PACKED;


struct checksumfs_dir_entry_block {
	uint16	entryCount;
	uint16	nameEnds[0];		// end offsets of the names (relative to the
								// start of the first name),
								// e.g. nameEnds[0] == length of first name
	// char	names[];			// string of all (unterminated) names,
								// directly follows the nameEnds array
	// ...
	// uint64	nodes[];
		// node array ends at the end of the block
};


#endif	// CHECK_SUM_FS_H
