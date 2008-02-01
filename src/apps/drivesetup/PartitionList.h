/*
 * Copyright 2006-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@unet.nl>
 *		Stephan Aßmus <superstippi@gmx.de>
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
								PartitionListRow(partition_id parentID,
									off_t offset, off_t size);
	
			partition_id		ID() const
									{ return fPartitionID; }
			off_t				Offset() const
									{ return fOffset; }
			off_t				Size() const
									{ return fSize; }
private:
			partition_id		fPartitionID;
			partition_id		fParentID;
			off_t				fOffset;
			off_t				fSize;
};


class PartitionListView : public BColumnListView {
	typedef BColumnListView Inherited;
public:
								PartitionListView(const BRect& frame,
									uint32 resizeMode);
	
			PartitionListRow*	FindRow(partition_id id,
									PartitionListRow* parent = NULL);
			PartitionListRow*	AddPartition(BPartition* partition);
			PartitionListRow*	AddSpace(partition_id parent,
									off_t offset, off_t size);

private:
			int32				_InsertIndexForOffset(PartitionListRow* parent,
									off_t offset) const;
};


#endif // PARTITIONLIST_H
