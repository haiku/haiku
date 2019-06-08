// Item.cpp
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

#include "Item.h"
#include "Block.h"

/*!
	\class ItemHeader
	\brief Represents the on-disk structure for an item header.

	An ItemHeader is located in a leaf nodes and provides information about
	a respective item. Aside from the location and the size of the item
	it also contains the leftmost key of the item (used for searching)
	and the format version of that key.
*/

/*!
	\class Item
	\brief Provides access to the on-disk item structure.

	Item is the base class for DirItem, IndirectItem and StatItem.
	It does little more than to hold a pointer to the leaf node it resides
	on and to its ItemHeader. The Item locks the node in the block cache
	as longs as it exists. An Item can be savely casted into one of the
	derived types.
*/

// constructor
Item::Item()
	: fNode(NULL),
	  fHeader(NULL)
{
}

// constructor
Item::Item(const Item &item)
	: fNode(NULL),
	  fHeader(NULL)
{
	SetTo(item.GetNode(), item.GetHeader());
}

// constructor
Item::Item(LeafNode *node, const ItemHeader *header)
	: fNode(NULL),
	  fHeader(NULL)
{
	SetTo(node, header);
}

// destructor
Item::~Item()
{
	Unset();
}

// SetTo
status_t
Item::SetTo(LeafNode *node, const ItemHeader *header)
{
	Unset();
	status_t error = (node && header ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		fNode = node;
		fHeader = const_cast<ItemHeader*>(header);
		fNode->Get();
		// check the item
		error = Check();
		if (error != B_OK)
			Unset();
	}
	return error;
}

// SetTo
status_t
Item::SetTo(LeafNode *node, int32 index)
{
	Unset();
	status_t error = (node ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		error = SetTo(node, node->ItemHeaderAt(index));
	return error;
}

// Unset
void
Item::Unset()
{
	if (fNode) {
		fNode->Put();
		fNode = NULL;
	}
	fHeader = NULL;
}

// GetNode
LeafNode *
Item::GetNode() const
{
	return fNode;
}

// GetHeader
ItemHeader *
Item::GetHeader() const
{
	return fHeader;
}

// GetIndex
int32
Item::GetIndex() const
{
	return (fHeader - fNode->GetItemHeaders());
}

// GetData
void *
Item::GetData() const
{
	return (uint8*)fNode->GetData() + fHeader->GetLocation();
}

// GetLen
uint16
Item::GetLen() const
{
	return fHeader->GetLen();
}

// GetType
uint16
Item::GetType() const
{
	return fHeader->GetType();
}

// GetDirID
uint32
Item::GetDirID() const
{
	return fHeader->GetDirID();
}

// GetObjectID
uint32
Item::GetObjectID() const
{
	return fHeader->GetObjectID();
}

// GetOffset
uint64
Item::GetOffset() const
{
	return fHeader->GetOffset();
}

// GetKey
const Key *
Item::GetKey() const
{
	return fHeader->GetKey();
}

// GetKey
VKey *
Item::GetKey(VKey *k) const
{
	return fHeader->GetKey(k);
}

// GetEntryCount
uint16
Item::GetEntryCount() const
{
	return fHeader->GetEntryCount();
}

// Check
status_t
Item::Check() const
{
	// check location of the item
	uint32 blockSize = fNode->GetBlockSize();
	uint32 itemSpaceOffset = fNode->GetItemSpaceOffset();
	uint32 location = fHeader->GetLocation();
	if (location < itemSpaceOffset
		|| location + fHeader->GetLen() > blockSize) {
		FATAL(("WARNING: bad item %" B_PRId32
			" on node %" B_PRIu64 ": it can not be located "
			"where it claims to be: (location: %" B_PRIu32 ", len: %u, "
			"item space offset: %" B_PRIu32 ", "
			"block size: %" B_PRIu32 ")!\n", GetIndex(),
			fNode->GetNumber(), location, fHeader->GetLen(),
			itemSpaceOffset, blockSize));
		return B_BAD_DATA;
	}
	return B_OK;
}

// =
Item &
Item::operator=(const Item &item)
{
	SetTo(item.GetNode(), item.GetHeader());
	return *this;
}

