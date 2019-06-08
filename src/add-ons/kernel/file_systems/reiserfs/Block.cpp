// Block.cpp
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

#include "Block.h"

#include <stdlib.h>

//#include <fs_cache.h>

#include "BlockCache.h"
#include "Item.h"
#include "Key.h"

/*!
	\class Block
	\brief Represents a cached disk block.

	A block can either be formatted, i.e. a node in the S+Tree, or
	unformatted containing raw data. When formatted, it can be converted to
	a Node via the ToNode() method.
*/

// constructor
Block::Block()
	: fCache(NULL),
	  fNumber(0),
	  fData(NULL),
	  fFlags(KIND_UNKNOWN),
	  fRefCount(0)
{
}

// destructor
Block::~Block()
{
	_Unset();
}

// GetCache
BlockCache *
Block::GetCache() const
{
	return fCache;
}

// GetBlockSize
uint32
Block::GetBlockSize() const
{
	return fCache->GetBlockSize();
}

// Get
void
Block::Get()
{
	if (fCache)
		fCache->GetBlock(this);
}

// Put
void
Block::Put()
{
	if (fCache)
		fCache->PutBlock(this);
}

// GetNumber
uint64
Block::GetNumber() const
{
	return fNumber;
}

// GetData
void *
Block::GetData() const
{
	return fData;
}

// SetKind
void
Block::SetKind(uint32 kind)
{
	fFlags = (fFlags & ~(uint32)KIND_MASK) | (kind & KIND_MASK);
}

// SetChecked
void
Block::SetChecked(bool checked)
{
	if (checked)
		fFlags |= CHECKED;
	else
		fFlags &= ~(uint32)CHECKED;
}

// ToNode
Node *
Block::ToNode()
{
	Node *node = NULL;
	if (IsFormatted())
		node = static_cast<Node*>(this);
	return node;
}

// ToInternalNode
InternalNode *
Block::ToInternalNode()
{
	InternalNode *internalNode = NULL;
	if (Node *node = ToNode()) {
		if (node->IsInternal())
			internalNode = static_cast<InternalNode*>(node);
	}
	return internalNode;
}

// ToLeafNode
LeafNode *
Block::ToLeafNode()
{
	LeafNode *leafNode = NULL;
	if (Node *node = ToNode()) {
		if (node->IsLeaf())
			leafNode = static_cast<LeafNode*>(node);
	}
	return leafNode;
}

// IsFormatted
bool
Block::IsFormatted() const
{
	return (GetKind() == KIND_FORMATTED);
}

// IsLeaf
bool
Block::IsLeaf() const
{
	if (Node *node = const_cast<Block*>(this)->ToNode())
		return node->IsLeaf();
	return false;
}

// IsInternal
bool
Block::IsInternal() const
{
	if (Node *node = const_cast<Block*>(this)->ToNode())
		return node->IsInternal();
	return false;
}

// _SetTo
status_t
Block::_SetTo(BlockCache *cache, uint64 number)
{
	// unset
	_Unset();
	status_t error = (cache ? B_OK : B_BAD_VALUE);
	// set to new values
	fCache = cache;
	fNumber = number;
	if (error == B_OK) {
		fData = fCache->_GetBlock(fNumber);
		if (!fData)
			error = B_BAD_VALUE;
	}
	return error;
}

// _Unset
void
Block::_Unset()
{
	if (fCache && fData)
		fCache->_ReleaseBlock(fNumber, fData);
	fData = NULL;
	fCache = NULL;
	fNumber = 0;
	fData = NULL;
	fFlags = KIND_UNKNOWN;
	fRefCount = 0;
}

// _Get
void
Block::_Get()
{
	fRefCount++;
}

// _Put
bool
Block::_Put()
{
	return (--fRefCount == 0);
}


/*!
	\class Node
	\brief Represents a formatted cached disk block.

	A Node can be converted to an InternalNode or LeafNode, depending on
	its kind. Leaf nodes are located at level 1 only.
*/

// dummy constructor
Node::Node()
	: Block()
{
}

// GetLevel
uint16
Node::GetLevel() const
{
	return le2h(GetHeader()->blk_level);
}

// CountItems
/*!	\brief Returns the number of child "items" the node contains/refers to.

	If the node is a leaf node, this number is indeed the number of the
	items it contains. For internal node it is the number of keys, as oposed
	to the number of child nodes, which is CountItems() + 1.

	\return The number of child "items" the node contains/refers to.
*/
uint16
Node::CountItems() const
{
	return le2h(GetHeader()->blk_nr_item);
}

// FreeSpace
uint16
Node::GetFreeSpace() const
{
	return le2h(GetHeader()->blk_free_space);
}

// IsLeaf
bool
Node::IsLeaf() const
{
	return (GetLevel() == 1);
}

// IsInternal
bool
Node::IsInternal() const
{
	return (GetLevel() > 1);
}

// Check
status_t
Node::Check() const
{
	// check the minimal size of the node against its declared free space
	if (GetFreeSpace() + sizeof(block_head) > GetBlockSize()) {
		FATAL(("WARNING: bad node %" B_PRIu64
			": it declares more free space than "
			"possibly being available (%u vs %lu)!\n", GetNumber(),
			GetFreeSpace(), GetBlockSize() - sizeof(block_head)));
		return B_BAD_DATA;
	}
	return B_OK;
}

// GetHeader
block_head *
Node::GetHeader() const
{
	return (block_head*)fData;
}


/*!
	\class InternalNode
	\brief Represents an internal tree node.

	Internal tree node refer to its child nodes via DiskChilds.
*/

// GetKeys
const Key *
InternalNode::GetKeys() const
{
	return (const Key*)((uint8*)fData + sizeof(block_head));
}

// KeyAt
const Key *
InternalNode::KeyAt(int32 index) const
{
	const Key *k = NULL;
	if (index >= 0 && index < CountItems())
		k = GetKeys() + index;
	return k;
}

// GetChilds
const DiskChild *
InternalNode::GetChilds() const
{
	return (DiskChild*)(GetKeys() + CountItems());
}

// ChildAt
const DiskChild *
InternalNode::ChildAt(int32 index) const
{
	const DiskChild *child = NULL;
	if (index >= 0 && index <= CountItems())
		child = GetChilds() + index;
	return child;
}

// Check
status_t
InternalNode::Check() const
{
	// check the minimal size of the node against its declared free space
	// Note: This test is stricter than the that of the base class, so we
	// don't need to invoke it.
	uint32 size = (const uint8*)(GetChilds() + (CountItems() + 1))
				  - (const uint8*)GetData();
	if (size + GetFreeSpace() > GetBlockSize()) {
		FATAL(("WARNING: bad internal node %" B_PRIu64
			": it declares more free space "
			"than possibly being available (size: %" B_PRIu32 ", "
			"block size: %" B_PRIu32 ", "
			"free space: %u)!\n", GetNumber(), size, GetBlockSize(),
			GetFreeSpace()));
		return B_BAD_DATA;
	}
	return B_OK;
}


/*!
	\class LeafNode
	\brief Represents an tree leaf node.

	Leaf nodes contain items.
*/

// GetItemHeaders
const ItemHeader *
LeafNode::GetItemHeaders() const
{
	return (ItemHeader*)((uint8*)fData + sizeof(block_head));
}

// ItemHeaderAt
const ItemHeader *
LeafNode::ItemHeaderAt(int32 index) const
{
	const ItemHeader *header = NULL;
	if (index >= 0 && index < CountItems())
		header = GetItemHeaders() + index;
	return header;
}

// GetLeftKey
status_t
LeafNode::GetLeftKey(VKey *k) const
{
	status_t error = (k ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (const ItemHeader *header = ItemHeaderAt(0))
			header->GetKey(k);
		else
			error = B_ERROR;
	}
	return error;
}

// GetRightKey
status_t
LeafNode::GetRightKey(VKey *k) const
{
	status_t error = (k ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (const ItemHeader *header = ItemHeaderAt(CountItems() - 1))
			header->GetKey(k);
		else
			error = B_ERROR;
	}
	return error;
}

// Check
status_t
LeafNode::Check() const
{
	// check the minimal size of the node against its declared free space
	// Note: This test is stricter than the that of the base class, so we
	// don't need to invoke it.
	uint32 size = GetItemSpaceOffset();
	if (size + GetFreeSpace() > GetBlockSize()) {
		FATAL(("WARNING: bad leaf node %" B_PRIu64
			": it declares more free space "
			"than possibly being available "
			"(min size: %" B_PRIu32 ", block size: "
			"%" B_PRIu32 ", free space: %u)!\n",
			GetNumber(), size, GetBlockSize(), GetFreeSpace()));
		return B_BAD_DATA;
	}
	return B_OK;
}

// GetItemSpaceOffset
uint32
LeafNode::GetItemSpaceOffset() const
{
	return sizeof(ItemHeader) * CountItems() + sizeof(block_head);
}


/*!
	\class DiskChild
	\brief A structure referring to a child node of an internal node.
*/

