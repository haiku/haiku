#include "PartitionList.h"

#include <interface/ColumnTypes.h>
#include <storage/Path.h>

extern const char*
	SizeAsString(off_t size, char *string);; //FIXME: from MainWindow.cpp


PartitionListRow::PartitionListRow(BPartition* partition)
	: Inherited(),
	fPartitionID(partition->ID())
{
	BPath path;
	char size[1024];

	partition->GetPath(&path);

	SetField(new BBitmapField(NULL), 0);

	if (partition->IsDevice()) // Only show device path for actual devices (so only for /dev/disk/..../raw entries)
		SetField(new BStringField(path.Path()), 1);
	else
		SetField(new BStringField(""), 1);

	if (partition->ContainsPartitioningSystem()) {
		SetField(new BStringField(partition->ContentType()), 2);
	} else {
		SetField(new BStringField("n/a"), 2);
	}

	// For now, Partition type is always empty (as we do not care)
	SetField(new BStringField(""), 3);

	if (partition->ContainsFileSystem()) {
			SetField(new BStringField(partition->ContentType()), 4); // Filesystem
			SetField(new BStringField(partition->ContentName()), 5);	// Volume Name
	} else {
		SetField(new BStringField(""), 4);
		SetField(new BStringField(""), 5);
	}
	
	if (partition->IsMounted() && partition->GetMountPoint(&path) == B_OK) {
		SetField(new BStringField(path.Path()),  6);
	} else {
		SetField(new BStringField(""), 6);
	}

	SetField(new BStringField(SizeAsString(partition->Size(), size)), 7);
}

PartitionListView::PartitionListView(const BRect& frame)
	: Inherited(frame, "storagelist", B_FOLLOW_ALL, 0, B_PLAIN_BORDER, true)
{
	AddColumn(new BBitmapColumn("", 20, 20, 100, B_ALIGN_CENTER), 0);
	AddColumn(new BStringColumn("Device", 100, 50, 500, B_TRUNCATE_MIDDLE), 1);
	AddColumn(new BStringColumn("Map style", 100, 50, 500, B_TRUNCATE_MIDDLE), 2);
	AddColumn(new BStringColumn("Partition Type", 100, 50, 500, B_TRUNCATE_MIDDLE), 3);
	AddColumn(new BStringColumn("Filesystem", 100, 50, 500, B_TRUNCATE_MIDDLE), 4);
	AddColumn(new BStringColumn("Volume Name", 100, 50, 500, B_TRUNCATE_MIDDLE), 5);
	AddColumn(new BStringColumn("Mounted At", 100, 50, 500, B_TRUNCATE_MIDDLE), 6);
	AddColumn(new BStringColumn("Size", 100, 50, 500, B_TRUNCATE_END), 7);
}

PartitionListRow*
PartitionListView::FindRow(partition_id id)
{
	for (int32 idx=0; idx < CountRows(); idx++) {
		PartitionListRow* item = dynamic_cast<PartitionListRow*>(RowAt(idx));
		if (item != NULL && item->ID() == id)
			return item;
	}

	return NULL;
}

PartitionListRow*
PartitionListView::AddPartition(BPartition* partition)
{
	PartitionListRow* parent = NULL;
	PartitionListRow* partitionrow = NULL;
	
	// Forget about it if this partition is already in the listview
	if ((partitionrow=FindRow(partition->ID())) != NULL)
		return partitionrow;
	
	// Create the row for this partition
	partitionrow = new PartitionListRow(partition);

	// If this partition has a parent...
	if (partition->Parent() != NULL) {
		// check if it is in the listview
		parent = FindRow(partition->Parent()->ID());
		// If parent of this partition is not yet in the list
		if (parent == NULL)  //add it
			parent = AddPartition(partition->Parent());
		// Now it is ok to add this partition under its parent
		AddRow(partitionrow, parent);
	} else {
		// If this partition has no parent, add it in the 'root'
		AddRow(partitionrow);
	}
	
	return partitionrow;
}
