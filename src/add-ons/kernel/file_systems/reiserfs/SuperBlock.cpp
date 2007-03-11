// SuperBlock.cpp
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

#include <new>
#include <string.h>
#include <unistd.h>

#include "Debug.h"
#include "SuperBlock.h"

using std::nothrow;

/*!
	\class DirEntry
	\brief Represents the on-disk structure for super block of the FS.

	There exist two versions of the structure and this class can deal with
	both of them. The Init() methods tries to find and read the super block
	from disk.
*/

// read_super_block
template<typename super_block_t>
static
status_t
read_super_block(int device, off_t offset, const char *magic,
				 super_block_t **_superBlock)
{
	super_block_t *superBlock = NULL;
	status_t error = B_OK;
	// allocate memory for the super block
	if (error == B_OK) {
		superBlock = new(nothrow) super_block_t;
		if (!superBlock)
			error = B_NO_MEMORY;
	}
	// read the super block
	if (error == B_OK) {
		size_t size = sizeof(super_block_t);
		if (read_pos(device, offset, superBlock, size) != (int32)size)
			error = B_IO_ERROR;
	}
	// check magic
	if (error == B_OK) {
		size_t len = strlen(magic);
		if (strncmp(superBlock->s_magic, magic, len))
			error = B_ERROR;
	}
	// set result / cleanup on failure
	if (error == B_OK)
		*_superBlock = superBlock;
	else if (superBlock)
		delete superBlock;
	return error;
}

// constructor
SuperBlock::SuperBlock()
	: fFormatVersion(REISERFS_3_6),
	  fCurrentData(NULL)
{
}

// destructor
SuperBlock::~SuperBlock()
{
	if (GetFormatVersion() == REISERFS_3_6) {
		if (fCurrentData)
			delete fCurrentData;
	} else {
		if (fOldData)
			delete fOldData;
	}
}

// Init
status_t
SuperBlock::Init(int device, off_t offset)
{
	// Not sure, if I understand the weird versioning.
	status_t error = B_OK;
	// try old version
	if (read_super_block(device, REISERFS_OLD_DISK_OFFSET_IN_BYTES + offset,
						 REISERFS_SUPER_MAGIC_STRING, &fOldData) == B_OK) {
PRINT(("SuperBlock: ReiserFS 3.5\n"));
		fFormatVersion = REISERFS_3_5;
	// try new version
	} else if (read_super_block(device, REISERFS_DISK_OFFSET_IN_BYTES + offset,
			REISER2FS_SUPER_MAGIC_STRING, &fOldData) == B_OK) {
PRINT(("SuperBlock: ReiserFS 3.6\n"));
		fFormatVersion = REISERFS_3_6;
	// failure
	} else
		error = B_ERROR;
	// TODO: checks...
	return error;
}

