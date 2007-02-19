// VNode.h
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

#ifndef V_NODE_H
#define V_NODE_H

#include "fsproto.h"
#include "StatItem.h"

class VNode {
public:
	VNode();
	VNode(const VNode &node);
	VNode(vnode_id id);
	VNode(uint32 dirID, uint32 objectID);
	~VNode();

	status_t SetTo(vnode_id id);
	status_t SetTo(uint32 dirID, uint32 objectID);

//	void SetParentDirID(uint32 id)		{ fParentDirID = id; }
//	uint32 GetParentDirID() const		{ return fParentDirID; }
//	void SetParentObjectID(uint32 id)	{ fParentObjectID = id; }
//	uint32 GetParentObjectID() const	{ return fParentObjectID; }
	void SetDirID(uint32 id)			{ fDirID = id; }
	uint32 GetDirID() const				{ return fDirID; }
	void SetObjectID(uint32 id)			{ fObjectID = id; }
	uint32 GetObjectID() const			{ return fObjectID; }

	vnode_id GetID() const;
	void SetParentID(uint32 dirID, uint32 objectID);
	void SetParentID(vnode_id id)		{ fParentID = id; }
	// only valid for dirs!
	vnode_id GetParentID() const		{ return fParentID; }

	StatData *GetStatData() { return &fStatData; }
	const StatData *GetStatData() const { return &fStatData; }

	bool IsDir() const { return fStatData.IsDir(); }
	bool IsFile() const { return fStatData.IsFile(); }
	bool IsSymlink() const { return fStatData.IsSymlink(); }
	bool IsEsoteric() const { return fStatData.IsEsoteric(); }

	// vnode ID <-> dir,object ID conversion
	static vnode_id GetIDFor(uint32 dirID, uint32 objectID)
		{ return (vnode_id)((uint64)dirID << 32 | (uint64)objectID); }
	static uint32 GetDirIDFor(vnode_id id) { return uint32((uint64)id >> 32); }
	static uint32 GetObjectIDFor(vnode_id id)
		{ return uint32((uint64)id & 0xffffffffULL); }

	VNode &operator=(const VNode &node);

private:
	vnode_id	fParentID;
	uint32		fDirID;
	uint32		fObjectID;
	StatData	fStatData;
};

#endif	// V_NODE_H
