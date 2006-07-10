#ifndef PARTITIONLIST_H
#define PARTITIONLIST_H

class PartitionListRow;
class PartitionListView;

#include <interface/ColumnListView.h>
#include <storage/Partition.h>

// Forward declarations
class BPartition;

class PartitionListRow : public BRow
{
	typedef BRow Inherited;
public:
	PartitionListRow(BPartition* partition);
	
	partition_id ID() { return fPartitionID; }
private:
	partition_id fPartitionID;
};

class PartitionListView : public BColumnListView
{
	typedef BColumnListView Inherited;
public:
	PartitionListView(const BRect& frame);
	
	PartitionListRow* FindRow(partition_id id);
	PartitionListRow* AddPartition(BPartition* partition);
};

#endif /* PARTITIONLIST_H */
