/*
 * Copyright 2022, Raghav Sharma, raghavself28@gmail.com
 * Distributed under the terms of the MIT License.
 */


#include "Attribute.h"

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

	// Invalid format
	return NULL;
}