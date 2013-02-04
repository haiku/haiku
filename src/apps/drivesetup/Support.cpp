/*
 * Copyright 2002-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@unet.nl>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Bryce Groff <bgroff@hawaii.edu>
 *		Erik Jaesler <ejakowatz@users.sourceforge.net>
 */


#include "Support.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>
#include <Partition.h>
#include <String.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Support"


static const int32 kMaxSliderLimit = 0x7fffff80;
	// this is the maximum value that BSlider seem to work with fine


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


// #pragma mark -


SizeSlider::SizeSlider(const char* name, const char* label,
		BMessage* message, off_t offset, off_t size, uint32 minGranularity)
	:
	BSlider(name, label, message, 0, kMaxSliderLimit, B_HORIZONTAL,
		B_TRIANGLE_THUMB),
	fStartOffset(offset),
	fEndOffset(offset + size),
	fMaxPartitionSize(size),
	fGranularity(minGranularity)
{
	rgb_color fillColor = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	UseFillColor(true, &fillColor);

	// Lazy loop to get a power of two granularity
	while (size / fGranularity > kMaxSliderLimit)
		fGranularity *= 2;

	SetKeyIncrementValue(int32(1024 * 1024 * 1.0 * kMaxSliderLimit
		/ ((MaxPartitionSize() - 1) / fGranularity) + 0.5));

	char buffer[64];
	char minString[64];
	char maxString[64];
	snprintf(minString, sizeof(minString), B_TRANSLATE("Offset: %s"),
		string_for_size(fStartOffset, buffer, sizeof(buffer)));
	snprintf(maxString, sizeof(maxString), B_TRANSLATE("End: %s"),
		string_for_size(fEndOffset, buffer, sizeof(buffer)));
	SetLimitLabels(minString, maxString);
}


SizeSlider::~SizeSlider()
{
}


void
SizeSlider::SetValue(int32 value)
{
	BSlider::SetValue(value);

	fSize = (off_t(1.0 * (MaxPartitionSize() - 1) * Value()
		/ kMaxSliderLimit + 0.5) / fGranularity) * fGranularity;
}


const char*
SizeSlider::UpdateText() const
{
	return string_for_size(Size(), fStatusLabel, sizeof(fStatusLabel));
}


off_t
SizeSlider::Size() const
{
	return fSize;
}


void
SizeSlider::SetSize(off_t size)
{
	if (size == fSize)
		return;

	SetValue(int32(1.0 * kMaxSliderLimit / fGranularity * size
		/ ((MaxPartitionSize() - 1) / fGranularity) + 0.5));
	fSize = size;
	UpdateTextChanged();
}


off_t
SizeSlider::Offset() const
{
	// TODO: This should be the changed offset once a double
	// headed slider is implemented.
	return fStartOffset;
}


off_t
SizeSlider::MaxPartitionSize() const
{
	return fMaxPartitionSize;
}
