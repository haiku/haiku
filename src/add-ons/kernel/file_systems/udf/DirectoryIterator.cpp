/*
 * Copyright 2003, Tyler Dauwalder, tyler@dauwalder.net.
 * Distributed under the terms of the MIT License.
 */

/*! \file DirectoryIterator.cpp */

#include "DirectoryIterator.h"

#include "Icb.h"
#include "UdfString.h"
#include "Utils.h"

#include <dirent.h>
#include <stdio.h>


status_t
DirectoryIterator::GetNextEntry(char *name, uint32 *length, ino_t *id)
{
	DEBUG_INIT_ETC("DirectoryIterator",
	               ("name: %p, length: %p, id: %p", name, length, id));

	if (!id || !name || !length)
		return B_BAD_VALUE;

	PRINT(("fPosition:          %Ld\n", fPosition));
	PRINT(("Parent()->Length(): %Ld\n", Parent()->Length()));

	status_t error = B_OK;
	if (fAtBeginning) {
		sprintf(name, ".");
		*length = 2;
		*id = Parent()->Id();
		fAtBeginning = false;
	} else {

		if (uint64(fPosition) >= Parent()->Length()) 
			RETURN(B_ENTRY_NOT_FOUND);

		uint8 data[kMaxFileIdSize];
		file_id_descriptor *entry = reinterpret_cast<file_id_descriptor*>(data);

		uint32 block = 0;
		off_t offset = fPosition;

		size_t entryLength = kMaxFileIdSize;
		// First read in the static portion of the file id descriptor,
		// then, based on the information therein, read in the variable
		// length tail portion as well.
		error = Parent()->Read(offset, entry, &entryLength, &block);
		if (!error && entryLength >= sizeof(file_id_descriptor)
			&& entry->tag().init_check(block) == B_OK) {
			PDUMP(entry);
			offset += entry->total_length();

			if (entry->is_parent()) {
				sprintf(name, "..");
				*length = 3;
			} else {
				UdfString string(entry->id(), entry->id_length());
				PRINT(("id == `%s'\n", string.Utf8()));
				DUMP(entry->icb());
				sprintf(name, "%s", string.Utf8());
				*length = string.Utf8Length();
			}
			*id = to_vnode_id(entry->icb());
		}

		if (!error)
			fPosition = offset;
	}

 	RETURN(error);
}


/*	\brief Rewinds the iterator to point to the first entry in the directory. */
void
DirectoryIterator::Rewind()
{
	fAtBeginning = true;
	fPosition = 0;
}


//	#pragma - Private methods


DirectoryIterator::DirectoryIterator(Icb *parent)
	:
	fAtBeginning(true),
	fParent(parent),
	fPosition(0)
{
}
