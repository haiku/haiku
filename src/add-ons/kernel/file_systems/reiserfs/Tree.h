// Tree.h
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

#ifndef TREE_H
#define TREE_H

#include <SupportDefs.h>

class Block;
class BlockCache;
class DirItem;
class Node;
class StatItem;
class TreeIterator;
class TreePath;
class Volume;

// Tree
class Tree {
public:
	Tree();
	~Tree();

	status_t Init(Volume *volume, Node *rootNode, uint32 treeHeight);
	status_t InitCheck() const;

	Volume *GetVolume() const { return fVolume; }

	uint32 GetTreeHeight() const;
	uint32 GetBlockSize() const;

	BlockCache *GetBlockCache() const;
	Node *GetRootNode() const;

	status_t GetBlock(uint64 blockNumber, Block **block);
	status_t GetNode(uint64 blockNumber, Node **node);

	status_t FindDirEntry(uint32 dirID, uint32 objectID, const char *name,
						  DirItem *foundItem, int32 *entryIndex);
	status_t FindDirEntry(uint32 dirID, uint32 objectID, const char *name,
						  size_t nameLen, DirItem *foundItem,
						  int32 *entryIndex);

	status_t FindStatItem(uint32 dirID, uint32 objectID, StatItem *item);

private:
	Volume		*fVolume;
	BlockCache	*fBlockCache;
	Node		*fRootNode;
	uint32		fTreeHeight;
};

#endif	// TREE_H
