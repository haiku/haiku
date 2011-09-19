/*
 * Copyright 2002-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef SUPPORT_H
#define SUPPORT_H


#include <DiskDeviceDefs.h>
#include <HashMap.h>
#include <HashString.h>
#include <Slider.h>
#include <String.h>
#include "StringForSize.h"


class BPartition;


void dump_partition_info(const BPartition* partition);

bool is_valid_partitionable_space(size_t size);

enum {
	GO_CANCELED	= 0,
	GO_SUCCESS
};


static const uint32 kMegaByte = 0x100000;

class SpaceIDMap : public HashMap<HashString, partition_id> {
public:
								SpaceIDMap();
	virtual						~SpaceIDMap();

			partition_id		SpaceIDFor(partition_id parentID,
									off_t spaceOffset);

private:
			partition_id		fNextSpaceID;
};

class SizeSlider : public BSlider {
public:
								SizeSlider(const char* name, const char* label,
        							BMessage* message, int32 minValue,
        							int32 maxValue);
	virtual						~SizeSlider();

	virtual const char*			UpdateText() const;
			int32				Size();
			int32				Offset();
			int32				MaxPartitionSize();

private:
			off_t				fStartOffset;
			off_t				fEndOffset;
			off_t				fMaxPartitionSize;
	mutable	char				fStatusLabel[64];
};

#endif // SUPPORT_H
