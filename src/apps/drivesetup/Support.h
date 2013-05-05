/*
 * Copyright 2002-2013 Haiku, Inc. All rights reserved.
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
									BMessage* message, off_t offset,
									off_t size, uint32 minGranularity);
	virtual						~SizeSlider();

	virtual	void				SetValue(int32 value);
	virtual const char*			UpdateText() const;

			off_t				Size() const;
			void				SetSize(off_t size);

			off_t				Offset() const;
			off_t				MaxPartitionSize() const;

private:
			off_t				fStartOffset;
			off_t				fEndOffset;
			off_t				fSize;
			off_t				fMaxPartitionSize;
			uint32				fGranularity;
	mutable	char				fStatusLabel[64];
};


#endif // SUPPORT_H
