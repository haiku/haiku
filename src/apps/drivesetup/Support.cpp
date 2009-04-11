/*
 * Copyright 2002-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Erik Jaesler <ejakowatz@users.sourceforge.net>
 *		Ithamar R. Adema <ithamar@unet.nl>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Support.h"

#include <stdio.h>

#include <Partition.h>
#include <String.h>


const char*
string_for_size(off_t size, char *string)
{
	double kb = size / 1024.0;
	if (kb < 1.0) {
		sprintf(string, "%Ld B", size);
		return string;
	}
	float mb = kb / 1024.0;
	if (mb < 1.0) {
		sprintf(string, "%3.1f KB", kb);
		return string;
	}
	float gb = mb / 1024.0;
	if (gb < 1.0) {
		sprintf(string, "%3.1f MB", mb);
		return string;
	}
	float tb = gb / 1024.0;
	if (tb < 1.0) {
		sprintf(string, "%3.1f GB", gb);
		return string;
	}
	sprintf(string, "%.1f TB", tb);
	return string;
}


void
dump_partition_info(const BPartition* partition)
{
	char size[1024];
	printf("\tOffset(): %Ld\n", partition->Offset());
	printf("\tSize(): %s\n", string_for_size(partition->Size(),size));
	printf("\tContentSize(): %s\n", string_for_size(partition->ContentSize(),
		size));
	printf("\tBlockSize(): %ld\n", partition->BlockSize());
	printf("\tIndex(): %ld\n", partition->Index());
	printf("\tStatus(): %ld\n\n", partition->Status());
	printf("\tContainsFileSystem(): %s\n",
		partition->ContainsFileSystem() ? "true" : "false");
	printf("\tContainsPartitioningSystem(): %s\n\n",
		partition->ContainsPartitioningSystem() ? "true" : "false");
	printf("\tIsDevice(): %s\n", partition->IsDevice() ? "true" : "false");
	printf("\tIsReadOnly(): %s\n", partition->IsReadOnly() ? "true" : "false");
	printf("\tIsMounted(): %s\n", partition->IsMounted() ? "true" : "false");
	printf("\tIsBusy(): %s\n\n", partition->IsBusy() ? "true" : "false");
	printf("\tFlags(): %lx\n\n", partition->Flags());
	printf("\tName(): %s\n", partition->Name());
	printf("\tContentName(): %s\n", partition->ContentName());
	printf("\tType(): %s\n", partition->Type());
	printf("\tContentType(): %s\n", partition->ContentType());
	printf("\tID(): %lx\n\n", partition->ID());
}


bool
is_valid_partitionable_space(size_t size)
{
	// TODO: remove this again, the DiskDeviceAPI should
	// not even show these spaces to begin with
	return size >= 8 * 1024 * 1024;
}


// #pragma mark - SpaceIDMap


SpaceIDMap::SpaceIDMap()
	: HashMap<HashString, partition_id>(),
	fNextSpaceID(-2)
{
}


SpaceIDMap::~SpaceIDMap()
{
}


partition_id
SpaceIDMap::SpaceIDFor(partition_id parentID, off_t spaceOffset)
{
	BString key;
	key << parentID << ':' << (uint64)spaceOffset;

	if (ContainsKey(key.String()))
		return Get(key.String());

	partition_id newID = fNextSpaceID--;
	Put(key.String(), newID);

	return newID;
}

