/*
 * Copyright 2006-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@unet.nl>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "PartitionList.h"
#include "Support.h"

#include <ColumnTypes.h>
#include <Path.h>


static const char* kUnavailableString = "";

enum {
//	kBitmapColumn,
	kDeviceColumn,
	kFilesystemColumn,
	kVolumeNameColumn,
	kMountedAtColumn,
	kSizeColumn
};


PartitionListRow::PartitionListRow(BPartition* partition)
	: Inherited()
	, fPartitionID(partition->ID())
	, fOffset(partition->Offset())
	, fSize(partition->Size())
{
//	SetField(new BBitmapField(NULL), kBitmapColumn);

	BPath path;
	partition->GetPath(&path);
	SetField(new BStringField(path.Path()), kDeviceColumn);

	if (partition->ContainsFileSystem()) {
		SetField(new BStringField(partition->ContentType()), kFilesystemColumn);
		SetField(new BStringField(partition->ContentName()), kVolumeNameColumn);
	} else {
		SetField(new BStringField(kUnavailableString), kFilesystemColumn);
		SetField(new BStringField(kUnavailableString), kVolumeNameColumn);
	}
	
	if (partition->IsMounted() && partition->GetMountPoint(&path) == B_OK) {
		SetField(new BStringField(path.Path()),  kMountedAtColumn);
	} else {
		SetField(new BStringField(kUnavailableString), kMountedAtColumn);
	}

	char size[1024];
	SetField(new BStringField(string_for_size(partition->Size(), size)),
		kSizeColumn);
}


PartitionListRow::PartitionListRow(partition_id id, off_t offset, off_t size)
	: Inherited()
	, fPartitionID(id)
	, fOffset(offset)
	, fSize(size)
{
//	SetField(new BBitmapField(NULL), kBitmapColumn);

	SetField(new BStringField("-"), kDeviceColumn);

	SetField(new BStringField("Empty"), kFilesystemColumn);
	SetField(new BStringField(kUnavailableString), kVolumeNameColumn);
	
	SetField(new BStringField(kUnavailableString), kMountedAtColumn);

	char sizeString[1024];
	SetField(new BStringField(string_for_size(size, sizeString)), kSizeColumn);
}


PartitionListView::PartitionListView(const BRect& frame, uint32 resizeMode)
	: Inherited(frame, "storagelist", resizeMode, 0, B_NO_BORDER, true)
{
//	AddColumn(new BBitmapColumn("", 20, 20, 100, B_ALIGN_CENTER),
//		kBitmapColumn);
	AddColumn(new BStringColumn("Device", 150, 50, 500, B_TRUNCATE_MIDDLE),
		kDeviceColumn);
	AddColumn(new BStringColumn("Filesystem", 100, 50, 500, B_TRUNCATE_MIDDLE),
		kFilesystemColumn);
	AddColumn(new BStringColumn("Volume Name", 130, 50, 500, B_TRUNCATE_MIDDLE),
		kVolumeNameColumn);
	AddColumn(new BStringColumn("Mounted At", 100, 50, 500, B_TRUNCATE_MIDDLE),
		kMountedAtColumn);
	AddColumn(new BStringColumn("Size", 100, 50, 500, B_TRUNCATE_END,
		B_ALIGN_RIGHT), kSizeColumn);

	SetSortingEnabled(false);
}


PartitionListRow*
PartitionListView::FindRow(partition_id id, PartitionListRow* parent)
{
	for (int32 i = 0; i < CountRows(parent); i++) {
		PartitionListRow* item
			= dynamic_cast<PartitionListRow*>(RowAt(i, parent));
		if (item != NULL && item->ID() == id)
			return item;
		if (CountRows(item) > 0) {
			// recurse into child rows
			item = FindRow(id, item);
			if (item)
				return item;
		}
	}

	return NULL;
}


PartitionListRow*
PartitionListView::AddPartition(BPartition* partition)
{
	PartitionListRow* partitionrow = FindRow(partition->ID());
	
	// forget about it if this partition is already in the listview
	if (partitionrow != NULL)
		return partitionrow;
	
	// create the row for this partition
	partitionrow = new PartitionListRow(partition);

	// see if this partition has a parent, or should have
	// a parent (add it in this case)
	PartitionListRow* parent = NULL;
	if (partition->Parent() != NULL) {
		// check if it is in the listview
		parent = FindRow(partition->Parent()->ID());
		// If parent of this partition is not yet in the list
		if (parent == NULL) {
			// add it
			parent = AddPartition(partition->Parent());
		}
	}

	// find a proper insertion index based on the on-disk offset
	int32 index = _InsertIndexForOffset(parent, partition->Offset());

	// add the row, parent may be NULL (add at top level)
	AddRow(partitionrow, index, parent);

	// make sure the row is initially expanded
	ExpandOrCollapse(partitionrow, true);
	
	return partitionrow;
}


PartitionListRow*
PartitionListView::AddSpace(partition_id parentID, partition_id id,
	off_t offset, off_t size)
{
	PartitionListRow* partitionrow = FindRow(id);
	
	// forget about it if this item is already in the listview
	if (partitionrow != NULL)
		return partitionrow;

	// the parent should already be in the listview
	PartitionListRow* parent = FindRow(parentID);
	if (!parent)
		return NULL;
	
	// create the row for this partition
	partitionrow = new PartitionListRow(id, offset, size);

	// find a proper insertion index based on the on-disk offset
	int32 index = _InsertIndexForOffset(parent, offset);

	// add the row, parent may be NULL (add at top level)
	AddRow(partitionrow, index, parent);

	// make sure the row is initially expanded
	ExpandOrCollapse(partitionrow, true);
	
	return partitionrow;
}


int32
PartitionListView::_InsertIndexForOffset(PartitionListRow* parent,
	off_t offset) const
{
	int32 index = 0;
	int32 count = CountRows(parent);
	for (; index < count; index++) {
		const PartitionListRow* item
			= dynamic_cast<const PartitionListRow*>(RowAt(index, parent));
		if (item && item->Offset() > offset)
			break;
	}
	return index;
}


