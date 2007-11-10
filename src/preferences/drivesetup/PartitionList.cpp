/*
 * Copyright 2006-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@unet.nl>
 */
#include "PartitionList.h"
#include "Support.h"

#include <ColumnTypes.h>
#include <Path.h>


PartitionListRow::PartitionListRow(BPartition* partition)
	: Inherited()
	, fPartitionID(partition->ID())
{
	BPath path;
	char size[1024];

	partition->GetPath(&path);

//	SetField(new BBitmapField(NULL), 0);

//	if (partition->IsDevice()) // Only show device path for actual devices (so only for /dev/disk/..../raw entries)
		SetField(new BStringField(path.Path()), 1);
//	else
//		SetField(new BStringField("n/a"), 1);

//	if (partition->ContainsPartitioningSystem()) {
//		SetField(new BStringField(partition->ContentType()), 2);
//	} else {
//		SetField(new BStringField("n/a"), 2);
//	}

	if (partition->ContainsFileSystem()) {
		SetField(new BStringField(partition->ContentType()), 2); // Filesystem
		SetField(new BStringField(partition->ContentName()), 3); // Volume Name
	} else {
		SetField(new BStringField(""), 2);
		SetField(new BStringField(""), 3);
	}
	
	if (partition->IsMounted() && partition->GetMountPoint(&path) == B_OK) {
		SetField(new BStringField(path.Path()),  4);
	} else {
		SetField(new BStringField(""), 4);
	}

	SetField(new BStringField(string_for_size(partition->Size(), size)), 5);
}


PartitionListView::PartitionListView(const BRect& frame)
	: Inherited(frame, "storagelist", B_FOLLOW_ALL, 0, B_NO_BORDER, true)
{
//	AddColumn(new BBitmapColumn("", 20, 20, 100, B_ALIGN_CENTER), 0);
	AddColumn(new BStringColumn("Device", 100, 50, 500, B_TRUNCATE_MIDDLE), 1);
	AddColumn(new BStringColumn("Filesystem", 100, 50, 500, B_TRUNCATE_MIDDLE), 2);
	AddColumn(new BStringColumn("Volume Name", 100, 50, 500, B_TRUNCATE_MIDDLE), 3);
	AddColumn(new BStringColumn("Mounted At", 100, 50, 500, B_TRUNCATE_MIDDLE), 4);
	AddColumn(new BStringColumn("Size", 100, 50, 500, B_TRUNCATE_END), 5);
}


PartitionListRow*
PartitionListView::FindRow(partition_id id, PartitionListRow* parent)
{
	for (int32 i = 0; i < CountRows(parent); i++) {
		PartitionListRow* item = dynamic_cast<PartitionListRow*>(RowAt(i, parent));
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
	
	// Forget about it if this partition is already in the listview
	if (partitionrow != NULL) {
		return partitionrow;
	}
	
	// Create the row for this partition
	partitionrow = new PartitionListRow(partition);

	// If this partition has a parent...
	if (partition->Parent() != NULL) {
		// check if it is in the listview
		PartitionListRow* parent = FindRow(partition->Parent()->ID());
		// If parent of this partition is not yet in the list
		if (parent == NULL) {
			// add it
			parent = AddPartition(partition->Parent());
		}
		// Now it is ok to add this partition under its parent
		AddRow(partitionrow, parent);
	} else {
		// If this partition has no parent, add it in the 'root'
		AddRow(partitionrow);
	}
	
	return partitionrow;
}
