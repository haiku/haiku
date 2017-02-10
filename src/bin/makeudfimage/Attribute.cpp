//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file Attribute.h

	BDataIO wrapper around a given attribute for a file. (implementation)
*/

#include "Attribute.h"

Attribute::Attribute(BNode &node, const char *attribute)
	: fNode(node)
	, fAttribute(attribute)
{
}

ssize_t
Attribute::Read(void *buffer, size_t size)
{
	return B_ERROR;
}

ssize_t
Attribute::Write(const void *buffer, size_t size)
{
	return B_ERROR;
}
