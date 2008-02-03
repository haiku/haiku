/*
 * Copyright 2002-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef SUPPORT_H
#define SUPPORT_H


#include <DiskDeviceDefs.h>
#include <HashMap.h>
#include <HashString.h>


class BPartition;


const char* string_for_size(off_t size, char *string);

void dump_partition_info(const BPartition* partition);

bool is_valid_partitionable_space(size_t size);


class SpaceIDMap : public HashMap<HashString, partition_id> {
public:
								SpaceIDMap();
	virtual						~SpaceIDMap();

			partition_id		SpaceIDFor(partition_id parentID,
									off_t spaceOffset);

private:
			partition_id		fNextSpaceID;
};


#endif // SUPPORT_H
