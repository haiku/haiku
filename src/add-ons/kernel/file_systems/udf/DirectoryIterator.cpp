//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------

/*! \file DirectoryIterator.cpp
*/

#include "DirectoryIterator.h"

#include <dirent.h>
#include <stdio.h>

#include "Icb.h"

#include "CS0String.h"
#include "Utils.h"

using namespace Udf;

status_t
DirectoryIterator::GetNextEntry(char *name, uint32 *length, vnode_id *id)
{
	DEBUG_INIT_ETC(CF_PUBLIC | CF_DIRECTORY_OPS, "DirectoryIterator",
	               ("name: %p, length: %p, id: %p", name, length, id));

	if (!id || !name || !length)
		return B_BAD_VALUE;

	status_t error = B_OK;
	if (fAtBeginning) {
		sprintf(name, ".");
		*length = 2;
		*id = Parent()->Id();
		fAtBeginning = false;
	} else {

	if (uint64(fPosition) >= Parent()->Length())
		return B_ENTRY_NOT_FOUND;

	uint8 data[kMaxFileIdSize];
	file_id_descriptor *entry = reinterpret_cast<file_id_descriptor*>(data);

	uint32 block = 0;
	off_t offset = fPosition;

	size_t entryLength = kMaxFileIdSize;
	// First read in the static portion of the file id descriptor,
	// then, based on the information therein, read in the variable
	// length tail portion as well.
	error = Parent()->Read(offset, entry, &entryLength, &block);
	if (!error && entryLength >= sizeof(file_id_descriptor) && entry->tag().init_check(block) == B_OK) {
		PDUMP(entry);
		offset += entry->total_length();
		
		if (entry->is_parent()) {
			sprintf(name, "..");
			*length = 3;
		} else {
			CS0String string(entry->id(), entry->id_length());
			PRINT(("id == `%s'\n", string.String()));
			DUMP(entry->icb());
			sprintf(name, "%s", string.String());
			*length = string.Length();
		}		
		*id = to_vnode_id(entry->icb());
	}	

	if (!error)
		fPosition = offset;
	}
 
 	RETURN(error);
}

/*	\brief Rewinds the iterator to point to the first
	entry in the directory.
*/
void
DirectoryIterator::Rewind()
{
	fPosition = 0;
	fAtBeginning = true;
}

DirectoryIterator::DirectoryIterator(Icb *parent)
	: fParent(parent)
	, fPosition(0)
	, fAtBeginning(true)
{
}

void
DirectoryIterator::Invalidate()
{
	fParent = NULL;
}
 

