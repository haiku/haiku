// IndirectItem.h
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

#ifndef INDIRECT_ITEM_H
#define INDIRECT_ITEM_H

#include "endianess.h"

// IndirectItem
/*!
	\class IndirectItem
	\brief Provides access to the on-disk indirect item structure.

	An indirect item lists raw blocks containing the contents of a file.
*/
class IndirectItem : public Item {
public:
	IndirectItem() : Item() {}
	IndirectItem(LeafNode *node, ItemHeader *header)
		: Item(node, header) {}

	uint32 CountBlocks() const
	{
		return GetLen() / sizeof(uint32);
	}

	uint64 BlockNumberAt(int32 index) const
	{
		uint number = 0;
		if (index >= 0 && index < (int32)CountBlocks())
			number = le2h(((uint32*)GetData())[index]);
		return number;
	}
};

#endif	// INDIRECT_ITEM_H
