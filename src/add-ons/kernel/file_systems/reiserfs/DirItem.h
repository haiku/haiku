// DirItem.h
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

#ifndef DIR_ITEM_H
#define DIR_ITEM_H

#include <string.h>

#include "Block.h"
#include "Debug.h"
#include "endianess.h"
#include "Item.h"
#include "String.h"

// DirEntry
/*!
	\class DirEntry
	\brief Represents the on-disk structure for a directory entry.

	A DirEntry has an offset identifying it uniquely in the list of
	entries, and it knows about the dir and object ID of the actual
	entry. Note, that the dir ID is note necessarily the object ID of
	the parent directory! The DirEntry also knows the relative
	location of its name in the dir item.
*/
class DirEntry : private reiserfs_de_head {
public:
	DirEntry() {}

	uint32 GetOffset() const { return le2h(deh_offset); }
	uint32 GetDirID() const { return le2h(deh_dir_id); }
	uint32 GetObjectID() const { return le2h(deh_objectid); }
	uint16 GetLocation() const { return le2h(deh_location); }
	uint16 GetState() const { return le2h(deh_state); }

	bool IsVisible() const { return (GetState() & (1 << DEH_Visible)); }
	bool IsHidden() const { return !IsVisible(); }

	void Dump()
	{
		PRINT(("  dir entry\n"));
		PRINT(("    offset:    %" B_PRIu32 "\n", GetOffset()));
		PRINT(("    dir ID:    %" B_PRIu32 "\n", GetDirID()));
		PRINT(("    object ID: %" B_PRIu32 "\n", GetObjectID()));
		PRINT(("    location:  %hu\n", GetLocation()));
		PRINT(("    state:     %hx\n", GetState()));
	}
} _PACKED;

// DirItem
/*!
	\class DirItem
	\brief Provides access to the on-disk dir item structure.

	A dir item consists of an array of DirEntrys and the names of these
	entries. Note, that in general the names are not null terminated.
	EntryNameAt() returns the length of the name.
*/
class DirItem : public Item {
public:
	DirItem() : Item() {}
	DirItem(LeafNode *node, ItemHeader *header)
		: Item(node, header) {}

	DirEntry *EntryAt(int32 index) const
	{
		DirEntry *entry = NULL;
		if (index >= 0 && index < GetEntryCount())
			entry = (DirEntry*)GetData() + index;
		return entry;
	}

	const char *EntryNameAt(int32 index, size_t *nameLen = NULL) const
	{
		const char *name = NULL;
		if (DirEntry *entry = EntryAt(index)) {
			// check the name location
			uint32 location = entry->GetLocation();
			if (location < GetEntryNameSpaceOffset() || location > GetLen()) {
				// bad location
				FATAL(("WARNING: bad dir entry %" B_PRId32 " "
					"in item %" B_PRId32 " on node %" B_PRIu64 ": "
					"the entry's name location is %" B_PRIu32 ", "
					"which is outside the entry name space "
					"(%" B_PRIu32 " - %u)!\n",
					index, GetIndex(), fNode->GetNumber(), location,
					GetEntryNameSpaceOffset(), GetLen()));
			} else {
				// get the name
				name = (char*)((uint8*)GetData() + location);
			}
			if (name && nameLen) {
				size_t maxLength = 0;
				if (index == 0)
					maxLength = fHeader->GetLen() - entry->GetLocation();
				else {
					maxLength = EntryAt(index -1)->GetLocation()
						- entry->GetLocation();
				}
				*nameLen = strnlen(name, maxLength);
			}
		}
		return name;
	}

	status_t GetEntryNameAt(int32 index, char *buffer, size_t bufferSize)
	{
		status_t error = (buffer && index >= 0 && index < GetEntryCount()
						  ? B_OK : B_BAD_VALUE);
		if (error == B_OK) {
			size_t nameLen = 0;
			const char *name = EntryNameAt(index, &nameLen);
			if (name && nameLen > 0) {
				if (nameLen + 1 <= bufferSize) {
					strncpy(buffer, name, nameLen);
					buffer[nameLen] = 0;
				} else	// buffer too small
					error = B_BAD_VALUE;
			} else	// bad name
				error = B_BAD_DATA;
		}
		return error;
	}

	int32 IndexOfName(const char *name) const
	{
		if (name == NULL)
			return -1;

		int32 count = GetEntryCount();
		size_t len = strlen(name);
		for (int32 i = 0; i < count; i++) {
			size_t nameLen = 0;
			const char *itemName = EntryNameAt(i, &nameLen);
			if (nameLen == len && !strncmp(name, itemName, len)) {
				return i;
			}
		}
		return -1;
	}

	status_t Check() const
	{
		// the base class version checks the location of the item
		status_t error = Item::Check();
		// check whether the entry headers can possibly fit into the item
		if (error == B_OK) {
			if (GetEntryNameSpaceOffset() > GetLen()) {
				FATAL(("WARNING: bad dir item %" B_PRId32 " "
					"on node %" B_PRIu64 ": the item has "
					"len %u and can thus impossibly contain %u entry "
					"headers!\n", GetIndex(), fNode->GetNumber(), GetLen(),
					GetEntryCount()));
				return B_BAD_DATA;
			}
		}
		return error;
	}

private:
	uint32 GetEntryNameSpaceOffset() const
		{ return GetEntryCount() * sizeof(DirEntry); }
};

#endif	// DIR_ITEM_H
