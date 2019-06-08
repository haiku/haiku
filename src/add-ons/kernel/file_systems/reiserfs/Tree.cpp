// Tree.cpp
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

#include <stdio.h>

#include "Tree.h"
#include "Block.h"
#include "BlockCache.h"
#include "Debug.h"
#include "DirItem.h"
#include "Iterators.h"
#include "StatItem.h"
#include "Volume.h"

const uint32 kMaxTreeHeight = 5;

/*!
	\class Tree
	\brief Represents the on-disk S+Tree.

	This class actually doesn't have that much functionality. GetBlock()
	and GetNode() are used to request a block/tree node from disk,
	FindDirEntry() searches an entry in a directory and FindStatItem()
	gets the stat data for an object. The mammoth part of the code is
	located in the iterator code (Iterators.{cpp,h}).
*/

// constructor
Tree::Tree()
	: fVolume(NULL),
	  fBlockCache(NULL),
	  fRootNode(NULL),
	  fTreeHeight(0)
{
}

// destructor
Tree::~Tree()
{
	if (fRootNode)
		fRootNode->Put();
}

// Init
status_t
Tree::Init(Volume *volume, Node *rootNode, uint32 treeHeight)
{
	status_t error = (volume && volume->GetBlockCache() && rootNode
					  ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (treeHeight > kMaxTreeHeight) {
			// we don't need to fail, as we can deal with that gracefully
			INFORM(("WARNING: tree height greater maximal height: %" B_PRIu32
				"\n", treeHeight));
		}
		fVolume = volume;
		fBlockCache = fVolume->GetBlockCache();
		fRootNode = rootNode;
		fRootNode->Get();
		fTreeHeight = treeHeight;
	}
	return error;
}

// InitCheck
status_t
Tree::InitCheck() const
{
	return (fBlockCache && fRootNode && fTreeHeight ? B_OK : B_NO_INIT);
}

// GetTreeHeight
uint32
Tree::GetTreeHeight() const
{
	return fTreeHeight;
}

// GetBlockSize
uint32
Tree::GetBlockSize() const
{
	if (fBlockCache)
		return fBlockCache->GetBlockSize();
	return 0;
}


// GetBlockCache
BlockCache *
Tree::GetBlockCache() const
{
	return fBlockCache;
}

// GetRootNode
Node *
Tree::GetRootNode() const
{
	return fRootNode;
}

// GetBlock
status_t
Tree::GetBlock(uint64 blockNumber, Block **block)
{
	status_t error = (block ? InitCheck() : B_BAD_VALUE);
	if (error == B_OK)
		error = fBlockCache->GetBlock(blockNumber, block);
	return error;
}

// GetNode
status_t
Tree::GetNode(uint64 blockNumber, Node **node)
{
	status_t error = (node ? InitCheck() : B_BAD_VALUE);
	if (error == B_OK) {
		Block *block;
		error = fBlockCache->GetBlock(blockNumber, &block);
		if (error == B_OK) {
			if (block->GetKind() == Block::KIND_UNKNOWN)
				block->SetKind(Block::KIND_FORMATTED);
			if (block->GetKind() == Block::KIND_FORMATTED) {
				*node = block->ToNode();
				// check the node
				if (!(*node)->IsChecked()) {
					if ((*node)->IsInternal())
						error = (*node)->ToInternalNode()->Check();
					else if ((*node)->IsLeaf())
						error = (*node)->ToLeafNode()->Check();
					else
						error = B_BAD_DATA;
					if (error == B_OK)
						(*node)->SetChecked(true);
				}
			} else {
				block->Put();
				error = B_BAD_DATA;
			}
		}
	}
	return error;
}

// FindDirEntry
status_t
Tree::FindDirEntry(uint32 dirID, uint32 objectID, const char *name,
				   DirItem *foundItem, int32 *entryIndex)
{
	status_t error = (name && foundItem && entryIndex ? InitCheck()
													  : B_BAD_VALUE);
	if (error == B_OK) {
		error = FindDirEntry(dirID, objectID, name, strlen(name), foundItem,
							 entryIndex);
	}
	return error;
}

// FindDirEntry
status_t
Tree::FindDirEntry(uint32 dirID, uint32 objectID, const char *name,
				   size_t nameLen, DirItem *foundItem, int32 *entryIndex)
{
	status_t error = (name && foundItem && entryIndex ? InitCheck()
													  : B_BAD_VALUE);
	if (error == B_OK) {
		uint64 offset = 0;
		if (fVolume->GetKeyOffsetForName(name, nameLen, &offset) == B_OK) {
//PRINT(("Tree::FindDirEntry(): hash function: offset: %Lu (`%.*s', %lu)\n",
//offset, (int)nameLen, name, nameLen));
			// offset computed -- we can directly iterate only over entries
			// with that offset
			DirEntryIterator iterator(this, dirID, objectID, offset, true);
			DirItem dirItem;
			int32 index = 0;
			error = B_ENTRY_NOT_FOUND;
			while (iterator.GetPrevious(&dirItem, &index) == B_OK) {
				size_t itemNameLen = 0;
				if (const char *itemName
					= dirItem.EntryNameAt(index, &itemNameLen)) {
					if (itemNameLen == nameLen
						&& !strncmp(name, itemName, nameLen)) {
						// item found
						*foundItem = dirItem;
						*entryIndex = index;
						error = B_OK;
						break;
					}
				}
			}
		} else {
//PRINT(("Tree::FindDirEntry(): no hash function\n"));
			// no offset (no hash function) -- do it the slow way
			ObjectItemIterator iterator(this, dirID, objectID);
			DirItem dirItem;
			error = B_ENTRY_NOT_FOUND;
			while (iterator.GetNext(&dirItem, TYPE_DIRENTRY) == B_OK) {
				if (dirItem.Check() != B_OK) // bad data: skip item
					continue;
				int32 index = dirItem.IndexOfName(name);
				if (index >= 0) {
					// item found
					*foundItem = dirItem;
					*entryIndex = index;
					error = B_OK;
//PRINT(("  offset: %lu\n", dirItem.EntryAt(index)->GetOffset()));
					break;
				}
			}
		}
	}
	return error;
}

// FindStatItem
status_t
Tree::FindStatItem(uint32 dirID, uint32 objectID, StatItem *item)
{
	status_t error = (item ? InitCheck() : B_BAD_VALUE);
	if (error == B_OK) {
		ItemIterator iterator(this);
		error = iterator.InitCheck();
		VKey k(dirID, objectID, SD_OFFSET, V1_SD_UNIQUENESS, KEY_FORMAT_3_5);
		if (error == B_OK)
			error = iterator.FindRightMost(&k, item);
	}
	return error;
}

