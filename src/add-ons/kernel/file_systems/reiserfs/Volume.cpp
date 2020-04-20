// Volume.cpp
//
// Copyright (c) 2003, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can alternatively use *this file* under the terms of the the MIT
// license included in this package.

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Volume.h"
#include "Block.h"
#include "BlockCache.h"
#include "Debug.h"
#include "DirItem.h"
#include "IndirectItem.h"
#include "Iterators.h"
#include "reiserfs.h"
#include "Settings.h"
#include "StatItem.h"
#include "SuperBlock.h"
#include "Tree.h"
#include "VNode.h"


extern fs_vnode_ops gReiserFSVnodeOps;


// min and max
// We don't want to include <algobase.h> otherwise we also get <iostream.h>
// and other undesired things.
template<typename C>
static inline C min(const C &a, const C &b) { return (a < b ? a : b); }
template<typename C>
static inline C max(const C &a, const C &b) { return (a > b ? a : b); }

/*!
	\class Volume
	\brief Represents a volume.

	The Volume class bundles all functionality related to a volume.
	It knows the superblock and has some basic functionality needed
	for handling VNodes. Actually it should be the layer that provides the
	abstraction from VNodes. The design is not strict in this respect
	(the whole thing evolved while I was in the process of understanding
	the structures ;-), since the kernel interface does a good part of that
	too (e.g. concerning dir iteration).
*/

// constructor
Volume::Volume()
	: fFSVolume(NULL),
	  fDevice(-1),
	  fBlockCache(NULL),
	  fTree(NULL),
	  fSuperBlock(NULL),
	  fHashFunction(NULL),
	  fRootVNode(NULL),
	  fDeviceName(NULL),
	  fSettings(NULL),
	  fNegativeEntries()
{
	fVolumeName[0] = '\0';
}

// destructor
Volume::~Volume()
{
	Unmount();
}


// Identify
status_t
Volume::Identify(int fd, partition_data *partition)
{
	// open disk
	fDevice = dup(fd);
	if (fDevice < 0)
		return B_ERROR;

	// read and analyze superblock
	return _ReadSuperBlock();
}

// Mount
status_t
Volume::Mount(fs_volume *fsVolume, const char *path)
{
	Unmount();
	status_t error = (path ? B_OK : B_BAD_VALUE);
	fFSVolume = fsVolume;
	// load the settings
	if (error == B_OK) {
		fSettings = new(nothrow) Settings;
		if (fSettings)
			error = fSettings->SetTo(path);
		else
			error = B_NO_MEMORY;
	}
	// copy the device name
	if (error == B_OK) {
		fDeviceName = new(nothrow) char[strlen(path) + 1];
		if (fDeviceName)
			strcpy(fDeviceName, path);
		else
			error = B_NO_MEMORY;
	}
	// open disk
	if (error == B_OK) {
		fDevice = open(path, O_RDONLY);
		if (fDevice < 0)
			SET_ERROR(error, errno);
	}
	// read and analyze superblock
	if (error == B_OK)
		error = _ReadSuperBlock();

	if (error == B_OK)
		UpdateName(fsVolume->partition);

	// create and init block cache
	if (error == B_OK) {
		fBlockCache = new(nothrow) BlockCache;
		if (fBlockCache)
			error = fBlockCache->Init(fDevice, CountBlocks(), GetBlockSize());
		else
			error = B_NO_MEMORY;
	}
	// create the tree
	if (error == B_OK) {
		fTree = new(nothrow) Tree;
		if (!fTree)
			error = B_NO_MEMORY;
	}
	// get the root node and init the tree
	if (error == B_OK) {
		Block *rootBlock = NULL;
		error = fBlockCache->GetBlock(fSuperBlock->GetRootBlock(), &rootBlock);
REPORT_ERROR(error);
		if (error == B_OK) {
			rootBlock->SetKind(Block::KIND_FORMATTED);
			error = fTree->Init(this, rootBlock->ToNode(),
								fSuperBlock->GetTreeHeight());
REPORT_ERROR(error);
			rootBlock->Put();
		}
	}
	// get the root VNode (i.e. the root dir)
	if (error == B_OK) {
		fRootVNode = new(nothrow) VNode;
		if (fRootVNode) {
			error = FindVNode(REISERFS_ROOT_PARENT_OBJECTID,
							  REISERFS_ROOT_OBJECTID, fRootVNode);
REPORT_ERROR(error);
			if (error == B_OK) {
				error = publish_vnode(fFSVolume, fRootVNode->GetID(),
					fRootVNode, &gReiserFSVnodeOps, S_IFDIR, 0);
			}
REPORT_ERROR(error);
		} else
			error = B_NO_MEMORY;
	}
	// init the hash function
	if (error == B_OK)
		_InitHashFunction();
	// init the negative entry list
	if (error == B_OK)
		_InitNegativeEntries();
	// cleanup on error
	if (error != B_OK)
		Unmount();
	RETURN_ERROR(error);
}

// Unmount
status_t
Volume::Unmount()
{
	if (fRootVNode) {
		delete fRootVNode;
		fRootVNode = NULL;
	}
	if (fTree) {
		delete fTree;
		fTree = NULL;
	}
	if (fSuperBlock) {
		delete fSuperBlock;
		fSuperBlock = NULL;
	}
	if (fBlockCache) {
		delete fBlockCache;
		fBlockCache = NULL;
	}
	if (fDeviceName) {
		delete[] fDeviceName;
		fDeviceName = NULL;
	}
	if (fDevice >= 0) {
		close(fDevice);
		fDevice = -1;
	}
	if (fSettings) {
		delete fSettings;
		fSettings = NULL;
	}
	fNegativeEntries.MakeEmpty();
	fHashFunction = NULL;
	fFSVolume = NULL;
	return B_OK;
}

// GetBlockSize
off_t
Volume::GetBlockSize() const
{
	return fSuperBlock->GetBlockSize();
}

// CountBlocks
off_t
Volume::CountBlocks() const
{
	return fSuperBlock->CountBlocks();
}

// CountFreeBlocks
off_t
Volume::CountFreeBlocks() const
{
	return fSuperBlock->CountFreeBlocks();
}

// GetName
const char *
Volume::GetName() const
{
	return fVolumeName;
}


// UpdateName
void
Volume::UpdateName(partition_id partitionID)
{
	if (fSuperBlock->GetLabel(fVolumeName, sizeof(fVolumeName)) == B_OK)
		return;
	if (get_default_partition_content_name(partitionID, "ReiserFS",
			fVolumeName, sizeof(fVolumeName)) == B_OK)
		return;
	strlcpy(fVolumeName, "ReiserFS Volume", sizeof(fVolumeName));
}


// GetDeviceName
const char *
Volume::GetDeviceName() const
{
	return fDeviceName;
}

// GetKeyOffsetForName
status_t
Volume::GetKeyOffsetForName(const char *name, int len, uint64 *result)
{
	status_t error = (name && result ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (fHashFunction)
			*result = key_offset_for_name(fHashFunction, name, len);
		else
			error = B_ERROR;
	}
	return error;
}

// GetVNode
status_t
Volume::GetVNode(ino_t id, VNode **node)
{
	return get_vnode(GetFSVolume(), id, (void**)node);
}

// PutVNode
status_t
Volume::PutVNode(ino_t id)
{
	return put_vnode(GetFSVolume(), id);
}

// FindVNode
/*!	\brief Finds the node identified by a ino_t.

	\note The method does not initialize the parent ID for non-directory nodes.

	\param id ID of the node to be found.
	\param node pointer to a pre-allocated VNode to be initialized to
		   the found node.
	\return \c B_OK, if everything went fine.
*/
status_t
Volume::FindVNode(ino_t id, VNode *node)
{
	return FindVNode(VNode::GetDirIDFor(id), VNode::GetObjectIDFor(id), node);
}

// FindVNode
/*!	\brief Finds the node identified by a directory ID, object ID pair.

	\note The method does not initialize the parent ID for non-directory nodes.

	\param dirID Directory ID of the node to be found.
	\param objectID Object ID of the node to be found.
	\param node pointer to a pre-allocated VNode to be initialized to
		   the found node.
	\return \c B_OK, if everything went fine.
*/
status_t
Volume::FindVNode(uint32 dirID, uint32 objectID, VNode *node)
{
	// NOTE: The node's parent dir ID is not initialized!
	status_t error = (node ? B_OK : B_BAD_VALUE);
	// init the node
	if (error == B_OK)
		error = node->SetTo(dirID, objectID);
	// find the stat item
	StatItem item;
	if (error == B_OK) {
		error = fTree->FindStatItem(dirID, objectID, &item);
		if (error != B_OK) {
			FATAL(("Couldn't find stat item for node "
				"(%" B_PRIu32 ", %" B_PRIu32 ")\n",
				dirID, objectID));
		}
	}
	// get the stat data
	if (error == B_OK)
		SET_ERROR(error, item.GetStatData(node->GetStatData(), true));
	// for dirs get the ".." entry, since we need the parent dir ID
	if (error == B_OK && node->IsDir()) {
		DirItem dirItem;
		int32 index = 0;
		error = fTree->FindDirEntry(dirID, objectID, "..", &dirItem, &index);
		if (error == B_OK) {
			DirEntry *entry = dirItem.EntryAt(index);
			node->SetParentID(entry->GetDirID(), entry->GetObjectID());
		}
		else {
			FATAL(("failed to find `..' entry for dir node "
				"(%" B_PRIu32 ", %" B_PRIu32 ")\n",
				dirID, objectID));
		}
	}
	return error;
}

// FindDirEntry
/*!	\brief Searches an entry in a directory.

	\note Must not be called with \a entryName "." or ".."!

	\param dir The directory.
	\param entryName Name of the entry.
	\param foundNode pointer to a pre-allocated VNode to be initialized to
		   the found entry.
	\param failIfHidden The method shall fail, if the entry is hidden.
	\return \c B_OK, if everything went fine.
*/
status_t
Volume::FindDirEntry(VNode *dir, const char *entryName, VNode *foundNode,
					 bool failIfHidden)
{
	status_t error = (dir && foundNode ? B_OK : B_BAD_VALUE);
	// find the DirEntry
	DirItem item;
	int32 entryIndex = 0;
	if (error == B_OK) {
		error = fTree->FindDirEntry(dir->GetDirID(), dir->GetObjectID(),
									entryName, &item, &entryIndex);
	}
	// find the child node
	if (error == B_OK) {
		DirEntry *entry = item.EntryAt(entryIndex);
		error = FindVNode(entry->GetDirID(), entry->GetObjectID(), foundNode);
		if (error == B_OK && failIfHidden && entry->IsHidden())
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}


// ReadLink
/*!	\brief Reads a symlink.

	\note It is not check whether the object is a symlink or not! The caller
		  is responsible for that.

	\param node The symlink to be read from.
	\param buffer The buffer into which the data shall be written.
	\param bufferSize The number of bytes to be read.
	\param linkLength Pointer to a size_t to be set to the length of the link.
	\return \c B_OK, if everything went fine.
*/
status_t
Volume::ReadLink(VNode *node, char *buffer, size_t bufferSize,
				 size_t *linkLength)
{
	if (node == NULL || linkLength == NULL)
		return B_BAD_VALUE;

	if (!node->IsSymlink())
		return B_BAD_VALUE;

	StreamReader reader(fTree, node->GetDirID(), node->GetObjectID());
	status_t result = reader.InitCheck();
	if (result != B_OK)
		return result;

	size_t bytesCopied = bufferSize;
	result = reader.ReadAt(0, buffer, bufferSize, &bytesCopied);

	*linkLength = reader.StreamSize();

	return result;
}

// FindEntry
status_t
Volume::FindEntry(const VNode *rootDir, const char *path, VNode *foundNode)
{
// Note: does not resolve links.
PRINT(("Volume::FindEntry(`%s')\n", path));
	status_t error = (rootDir && path && foundNode ? B_OK : B_BAD_VALUE);
	if (error == B_OK && (path[0] == '\0' || path[0] == '/'))
		error = B_ENTRY_NOT_FOUND;
	// start at the given root dir
	if (error == B_OK)
		*foundNode = *rootDir;
	// resolve until we hit the end of the string
	while (error == B_OK && path[0] != '\0') {
PRINT(("  remaining path: `%s'\n", path));
		int32 len = strlen(path);
		// find the first `/'
		const char *componentNameEnd = strchr(path, '/');
		if (!componentNameEnd)
			componentNameEnd = path + len;
		// get the name of the first component
		int32 componentLen = componentNameEnd - path;
		if (componentLen >= B_FILE_NAME_LENGTH)
			return B_NAME_TOO_LONG;
		char component[B_FILE_NAME_LENGTH];
		strncpy(component, path, componentLen);
		component[componentLen] = '\0';
		// get the component
PRINT(("  looking for dir entry: `%s'\n", component));
		error = FindDirEntry(foundNode, component, foundNode);
		// skip trailing `/'s
		if (error == B_OK) {
			path = componentNameEnd;
			while (*path == '/')
				path++;
		}
	}
PRINT(("Volume::FindEntry(`%s') done: %s\n", path, strerror(error)));
	return error;
}

// IsNegativeEntry
bool
Volume::IsNegativeEntry(ino_t id)
{
	for (int32 i = fNegativeEntries.CountItems() - 1; i >= 0; i--) {
		if (fNegativeEntries.ItemAt(i) == id)
			return true;
	}
	return false;
}

// IsNegativeEntry
bool
Volume::IsNegativeEntry(uint32 dirID, uint32 objectID)
{
	return IsNegativeEntry(VNode::GetIDFor(dirID, objectID));
}

// GetHideEsoteric
bool
Volume::GetHideEsoteric() const
{
	return fSettings->GetHideEsoteric();
}

// _ReadSuperBlock
status_t
Volume::_ReadSuperBlock()
{
	status_t error = B_OK;
	fSuperBlock = new(nothrow) SuperBlock;
	if (fSuperBlock)
		error = fSuperBlock->Init(fDevice);
	else
		error = B_NO_MEMORY;
	// check FS state
	if (error == B_OK && fSuperBlock->GetState() != REISERFS_VALID_FS) {
		FATAL(("The superblock indicates a non-valid FS! Bailing out."));
		error = B_ERROR;
	}
	// check FS version
	if (error == B_OK && fSuperBlock->GetVersion() > REISERFS_VERSION_2) {
		FATAL(("The superblock indicates a version greater than 2 (%u)! "
			   "Bailing out.", fSuperBlock->GetVersion()));
		error = B_ERROR;
	}
	RETURN_ERROR(error);
}

// hash_function_for_code
static
hash_function_t
hash_function_for_code(uint32 code)
{
	hash_function_t function = NULL;
	// find the hash function
	switch (code) {
		case TEA_HASH:
			function = keyed_hash;
			break;
		case YURA_HASH:
			function = yura_hash;
			break;
		case R5_HASH:
			function = r5_hash;
			break;
		case UNSET_HASH:
		default:
			break;
	}
	return function;
}

// _InitHashFunction
void
Volume::_InitHashFunction()
{
	// get the hash function
	fHashFunction = hash_function_for_code(fSuperBlock->GetHashFunctionCode());
	// try to detect it, if it is not set or is not the right one
	if (!fHashFunction || !_VerifyHashFunction(fHashFunction)) {
		INFORM(("No or wrong directory hash function. Try to detect...\n"));
		uint32 code = _DetectHashFunction();
		fHashFunction = hash_function_for_code(code);
		// verify it
		if (fHashFunction) {
			if (_VerifyHashFunction(fHashFunction)) {
				INFORM(("Directory hash function successfully detected: "
						"%" B_PRIu32 "\n", code));
			} else {
				fHashFunction = NULL;
				INFORM(("Detected directory hash function is not the right "
						"one.\n"));
			}
		} else
			INFORM(("Failed to detect the directory hash function.\n"));
	}
}

// _DetectHashFunction
uint32
Volume::_DetectHashFunction()
{
	// iterate over the entries in the root dir until we find an entry, that
	// let us draw an unambiguous conclusion
	DirEntryIterator iterator(fTree, fRootVNode->GetDirID(),
							  fRootVNode->GetObjectID(), DOT_DOT_OFFSET + 1);
	uint32 foundCode = UNSET_HASH;
	DirItem item;
	int32 index = 0;
	while (foundCode == UNSET_HASH
		   && iterator.GetNext(&item, &index) == B_OK) {
		DirEntry *entry = item.EntryAt(index);
		uint64 offset = entry->GetOffset();
		uint32 hashCodes[] = { TEA_HASH, YURA_HASH, R5_HASH };
		int32 hashCodeCount = sizeof(hashCodes) / sizeof(uint32);
		size_t nameLen = 0;
		const char *name = item.EntryNameAt(index, &nameLen);
		if (!name)	// bad data!
			continue;
		// try each hash function -- if there's a single winner, we're done,
		// otherwise the next entry must help
		for (int32 i = 0; i < hashCodeCount; i++) {
			hash_function_t function = hash_function_for_code(hashCodes[i]);
			uint64 testOffset = key_offset_for_name(function, name, nameLen);
			if (offset_hash_value(offset) == offset_hash_value(testOffset)) {
				if (foundCode != UNSET_HASH) {
					// ambiguous
					foundCode = UNSET_HASH;
					break;
				} else
					foundCode = hashCodes[i];
			}
		}
	}
	return foundCode;
}

// _VerifyHashFunction
bool
Volume::_VerifyHashFunction(hash_function_t function)
{
	bool result = true;
	// iterate over the entries in the root dir until we find an entry, that
	// doesn't falsify the hash function
	DirEntryIterator iterator(fTree, fRootVNode->GetDirID(),
							  fRootVNode->GetObjectID(), DOT_DOT_OFFSET + 1);
	DirItem item;
	int32 index = 0;
	while (iterator.GetNext(&item, &index) == B_OK) {
		DirEntry *entry = item.EntryAt(index);
		uint64 offset = entry->GetOffset();
		// try the hash function
		size_t nameLen = 0;
		if (const char *name = item.EntryNameAt(index, &nameLen)) {
			uint64 testOffset = key_offset_for_name(function, name, nameLen);
			if (offset_hash_value(offset) != offset_hash_value(testOffset)) {
				result = false;
				break;
			}
		} // else: bad data
	}
	return result;
}

// _InitNegativeEntries
void
Volume::_InitNegativeEntries()
{
	// build a list of vnode IDs
	for (int32 i = 0; const char *entry = fSettings->HiddenEntryAt(i); i++) {
		if (entry && strlen(entry) > 0 && entry[0] != '/') {
			VNode node;
			if (FindEntry(fRootVNode, entry, &node) == B_OK
				&& node.GetID() != fRootVNode->GetID()) {
				fNegativeEntries.AddItem(node.GetID());
			} else
				INFORM(("WARNING: negative entry not found: `%s'\n", entry));
		}
	}
}
