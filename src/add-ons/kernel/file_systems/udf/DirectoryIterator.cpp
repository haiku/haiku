//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------

/*! \file DirectoryIterator.cpp
*/

#include "DirectoryIterator.h"

#include <stdio.h>

using namespace Udf;

status_t
DirectoryIterator::GetNextEntry(vnode_id *id, char *name, uint32 *length)
{
	if (!id || !name || !length)
		return B_BAD_VALUE;

	if (fCount < 5) 
		sprintf(name, "entry #%ld", fCount++);
	else 
		return B_ENTRY_NOT_FOUND;
		
	return B_OK;
}

/*	\brief Rewinds the iterator to point to the first
	entry in the directory.
*/
void
DirectoryIterator::Rewind()
{
	fCount = 0;
}

DirectoryIterator::DirectoryIterator(Icb *parent)
	: fParent(parent)
	, fCount(0)
{
}

void
DirectoryIterator::Invalidate()
{
	fParent = NULL;
}
 

