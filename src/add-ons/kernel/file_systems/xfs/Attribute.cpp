/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */


#include "Attribute.h"

#include "LeafAttribute.h"
#include "NodeAttribute.h"
#include "ShortAttribute.h"


Attribute::~Attribute()
{
}


Attribute*
Attribute::Init(Inode* inode)
{
	if (inode->AttrFormat() == XFS_DINODE_FMT_LOCAL) {
		TRACE("Attribute:Init: LOCAL\n");
		ShortAttribute* shortAttr = new(std::nothrow) ShortAttribute(inode);
		return shortAttr;
	}
	if (inode->AttrFormat() == XFS_DINODE_FMT_EXTENTS) {
		TRACE("Attribute::Init: EXTENTS\n");
		// check if Inode has extents or not?
		if (inode->AttrExtentsCount() == 0)
			return NULL;

		LeafAttribute* leafAttr = new(std::nothrow) LeafAttribute(inode);
		if (leafAttr == NULL)
			return NULL;

		status_t status = leafAttr->Init();

		if (status == B_OK)
			return leafAttr;
		delete leafAttr;

		NodeAttribute* nodeAttr = new(std::nothrow) NodeAttribute(inode);
		if (nodeAttr == NULL)
			return NULL;

		status = nodeAttr->Init();

		if (status == B_OK)
			return nodeAttr;
		delete nodeAttr;

		// Invalid format in extents return NULL
		return NULL;
	}

	// Invalid format
	return NULL;
}