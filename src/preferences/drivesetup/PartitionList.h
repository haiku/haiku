/*
 * Copyright 2006-2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef PARTITIONLIST_H
#define PARTITIONLIST_H


class PartitionListRow;
class PartitionListView;


#include <ColumnListView.h>
#include <Partition.h>


class BPartition;


class PartitionListRow : public BRow {
	typedef BRow Inherited;
public:
								PartitionListRow(BPartition* partition);
	
			partition_id		ID() { return fPartitionID; }
private:
			partition_id		fPartitionID;
};


class PartitionListView : public BColumnListView {
	typedef BColumnListView Inherited;
public:
								PartitionListView(const BRect& frame);
	
			PartitionListRow*	FindRow(partition_id id);
			PartitionListRow*	AddPartition(BPartition* partition);
};


#endif // PARTITIONLIST_H
