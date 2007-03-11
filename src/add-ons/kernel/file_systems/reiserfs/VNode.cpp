// VNode.cpp
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

#include "VNode.h"

/*!
	\class VNode
	\brief Represents a vnode, i.e. an object in a file system.

	This class bundles all information relevant to a vnode, i.e. its ID,
	the ID of its parent node and its stat data (for fast and convenient
	access). VNode knows how to convert between the VFS vnode_id
	and the object+dir ID representation in ReiserFS.
*/

// constructor
VNode::VNode()
	: fParentID(0),
	  fDirID(0),
	  fObjectID(0),
	  fStatData()
{
}

// constructor
VNode::VNode(const VNode &node)
	: fParentID(0),
	  fDirID(0),
	  fObjectID(0),
	  fStatData()
{
	*this = node;
}

// constructor
VNode::VNode(vnode_id id)
	: fParentID(0),
	  fDirID(GetDirIDFor(id)),
	  fObjectID(GetObjectIDFor(id)),
	  fStatData()
{
}

// constructor
VNode::VNode(uint32 dirID, uint32 objectID)
	: fParentID(0),
	  fDirID(dirID),
	  fObjectID(objectID),
	  fStatData()
{
}

// destructor
VNode::~VNode()
{
}

// SetTo
status_t
VNode::SetTo(vnode_id id)
{
	return SetTo(GetDirIDFor(id), GetObjectIDFor(id));
}

// SetTo
status_t
VNode::SetTo(uint32 dirID, uint32 objectID)
{
	fParentID = 0;
	fDirID = dirID;
	fObjectID = objectID;
	fStatData.Unset();
	return B_OK;
}

// GetID
vnode_id
VNode::GetID() const
{
	return GetIDFor(fDirID, fObjectID);
}

// SetParentID
void
VNode::SetParentID(uint32 dirID, uint32 objectID)
{
	SetParentID(GetIDFor(dirID, objectID));
}

// =
VNode &
VNode::operator=(const VNode &node)
{
	if (&node != this) {
		fParentID = node.fParentID;
		fDirID = node.fDirID;
		fObjectID = node.fObjectID;
		fStatData = node.fStatData;
	}
	return *this;
}

