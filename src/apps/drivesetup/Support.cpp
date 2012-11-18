/*
 * Copyright 2002-2012 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Erik Jaesler <ejakowatz@users.sourceforge.net>
 *		Ithamar R. Adema <ithamar@unet.nl>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Bryce Groff <bgroff@hawaii.edu>
 */


#include "Support.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>
#include <Partition.h>
#include <String.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Support"


void
dump_partition_info(const BPartition* partition)
{
	char size[1024];
	printf("\tOffset(): %" B_PRIdOFF "\n", partition->Offset());
	printf("\tSize(): %s\n", string_for_size(partition->Size(), size,
		sizeof(size)));
	printf("\tContentSize(): %s\n", string_for_size(partition->ContentSize(),
		size, sizeof(size)));
	printf("\tBlockSize(): %" B_PRId32 "\n", partition->BlockSize());
	printf("\tIndex(): %" B_PRId32 "\n", partition->Index());
	printf("\tStatus(): %" B_PRId32 "\n\n", partition->Status());
	printf("\tContainsFileSystem(): %s\n",
		partition->ContainsFileSystem() ? "true" : "false");
	printf("\tContainsPartitioningSystem(): %s\n\n",
		partition->ContainsPartitioningSystem() ? "true" : "false");
	printf("\tIsDevice(): %s\n", partition->IsDevice() ? "true" : "false");
	printf("\tIsReadOnly(): %s\n", partition->IsReadOnly() ? "true" : "false");
	printf("\tIsMounted(): %s\n", partition->IsMounted() ? "true" : "false");
	printf("\tIsBusy(): %s\n\n", partition->IsBusy() ? "true" : "false");
	printf("\tFlags(): %" B_PRIx32 "\n\n", partition->Flags());
	printf("\tName(): %s\n", partition->Name());
	printf("\tContentName(): %s\n", partition->ContentName());
	printf("\tType(): %s\n", partition->Type());
	printf("\tContentType(): %s\n", partition->ContentType());
	printf("\tID(): %" B_PRIx32 "\n\n", partition->ID());
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
	:
	HashMap<HashString, partition_id>(),
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


SizeSlider::SizeSlider(const char* name, const char* label,
		BMessage* message, int32 minValue, int32 maxValue)
	:
	BSlider(name, label, message, minValue, maxValue,
	B_HORIZONTAL, B_TRIANGLE_THUMB),
	fStartOffset(minValue),
	fEndOffset(maxValue),
	fMaxPartitionSize(maxValue - minValue)
{
	rgb_color fillColor = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	UseFillColor(true, &fillColor);
	char minString[64];
	char maxString[64];
	snprintf(minString, sizeof(minString), B_TRANSLATE("Offset: %ld MB"),
		fStartOffset);
	snprintf(maxString, sizeof(maxString), B_TRANSLATE("End: %ld MB"),
		fEndOffset);
	SetLimitLabels(minString, maxString);
}


SizeSlider::~SizeSlider()
{
}


const char*
SizeSlider::UpdateText() const
{
	// TODO: Perhaps replace with string_for_size, but it looks like
	// Value() and fStartOffset are always in MiB.
	snprintf(fStatusLabel, sizeof(fStatusLabel), B_TRANSLATE("%ld MiB"),
		Value() - fStartOffset);

	return fStatusLabel;
}


int32
SizeSlider::Size()
{
	return Value() - fStartOffset;
}


int32
SizeSlider::Offset()
{
	// TODO: This should be the changed offset once a double
	// headed slider is implemented.
	return fStartOffset;
}

int32
SizeSlider::MaxPartitionSize()
{
	return fMaxPartitionSize;
}
