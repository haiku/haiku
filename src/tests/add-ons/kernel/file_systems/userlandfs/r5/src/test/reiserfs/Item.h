// Item.h
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

#ifndef ITEM_H
#define ITEM_H

#include "endianess.h"
#include "reiserfs.h"
#include "Key.h"

class LeafNode;

// ItemHeader
class ItemHeader : private item_head {
public:
	ItemHeader() {}

	uint16 GetLen() const { return le2h(ih_item_len); }
	uint16 GetLocation() const { return le2h(ih_item_location); }
	uint16 GetVersion() const { return le2h(ih_version); }
	const Key *GetKey() const { return Key::CastFrom(&ih_key); }
	VKey *GetKey(VKey *k) const { k->SetTo(GetKey(), GetVersion()); return k; }

	// indirect item only
	uint16 GetFreeSpaceReserved() const
	{
		return (GetVersion() == KEY_FORMAT_3_6
			? 0 : le2h(u.ih_free_space_reserved));
	}

	// directory item only
	uint16 GetEntryCount() const { return le2h(u.ih_entry_count); }

	uint32 GetDirID() const { return GetKey()->GetDirID(); }
	uint32 GetObjectID() const { return GetKey()->GetObjectID(); }
	uint64 GetOffset() const { return GetKey()->GetOffset(GetVersion()); }
	uint16 GetType() const { return GetKey()->GetType(GetVersion()); }
} _PACKED;


// Item
class Item {
public:
	Item();
	Item(const Item &item);
	Item(LeafNode *node, const ItemHeader *header);
	~Item();

	status_t SetTo(LeafNode *node, const ItemHeader *header);
	status_t SetTo(LeafNode *node, int32 index);
	void Unset();

	LeafNode *GetNode() const;
	ItemHeader *GetHeader() const;
	int32 GetIndex() const;

	void *GetData() const;
	uint16 GetLen() const;
	uint16 GetType() const;
	uint32 GetDirID() const;
	uint32 GetObjectID() const;
	uint64 GetOffset() const;
	const Key *GetKey() const;
	VKey *GetKey(VKey *k) const;

	uint16 GetEntryCount() const;

	status_t Check() const;

	Item &operator=(const Item &item);

protected:
	LeafNode	*fNode;
	ItemHeader	*fHeader;
};

#endif	// ITEM_H
