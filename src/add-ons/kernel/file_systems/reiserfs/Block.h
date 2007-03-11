// Block.h
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

#ifndef BLOCK_H
#define BLOCK_H

#include <SupportDefs.h>

#include "BlockCache.h"
#include "endianess.h"
#include "reiserfs.h"

class DiskChild;
class InternalNode;
class Item;
class ItemHeader;
class Key;
class VKey;
class LeafNode;
class Node;

// Block
class Block {
public:
	Block();
	~Block();

	BlockCache *GetCache() const;
	uint32 GetBlockSize() const;

	void Get();
	void Put();

	uint64 GetNumber() const;
	void *GetData() const;

	void SetKind(uint32 kind);
	uint32 GetKind() const			{ return (fFlags & KIND_MASK); }

	void SetChecked(bool checked);
	bool IsChecked() const			{ return (fFlags & CHECKED); }

	bool IsFormatted() const;
	bool IsLeaf() const;
	bool IsInternal() const;

	Node *ToNode();
	InternalNode *ToInternalNode();
	LeafNode *ToLeafNode();

public:
	enum {
		KIND_FORMATTED		= 0x00,
		KIND_UNFORMATTED	= 0x01,
		KIND_UNKNOWN		= 0x02,
		KIND_MASK			= 0x03,
		CHECKED				= 0x80,
	};

private:
	status_t _SetTo(BlockCache *cache, uint64 number);
	void _Unset();

	int32 _GetRefCount() const { return fRefCount; }
	void _Get();
	bool _Put();

private:
	friend class BlockCache;

protected:
	BlockCache		*fCache;
	uint64			fNumber;
	void			*fData;
	uint32			fFlags;
	int32			fRefCount;
};

// Node
class Node : public Block {
public:
	uint16 GetLevel() const;
	uint16 CountItems() const;
	uint16 GetFreeSpace() const;
	bool IsLeaf() const;
	bool IsInternal() const;

	status_t Check() const;

private:
	block_head *GetHeader() const;

private:
	Node();
};

// InternalNode
class InternalNode : public Node {
public:
	const Key *GetKeys() const;
	const Key *KeyAt(int32 index) const;
	const DiskChild *GetChilds() const;
	const DiskChild *ChildAt(int32 index) const;

	status_t Check() const;
};

// LeafNode
class LeafNode : public Node {
public:
	const ItemHeader *GetItemHeaders() const;
	const ItemHeader *ItemHeaderAt(int32 index) const;
	status_t GetLeftKey(VKey *k) const;
	status_t GetRightKey(VKey *k) const;

	status_t Check() const;

	uint32 GetItemSpaceOffset() const;
};

// DiskChild
class DiskChild : private disk_child {
public:
	DiskChild() {}

	uint32 GetBlockNumber() const { return le2h(dc_block_number); }
	uint16 GetSize() const { return le2h(dc_size); }
};

#endif	// BLOCK_H
