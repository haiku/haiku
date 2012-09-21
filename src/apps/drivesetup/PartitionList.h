/*
 * Copyright 2006-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ithamar R. Adema <ithamar@unet.nl>
 *		James Urquhart
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef PARTITIONLIST_H
#define PARTITIONLIST_H


#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <Partition.h>


class BPartition;


// A field type displaying both a bitmap and a string so that the
// tree display looks nicer (both text and bitmap are indented)
class BBitmapStringField : public BStringField {
	typedef BStringField Inherited;
public:
								BBitmapStringField(BBitmap* bitmap,
									const char* string);
	virtual						~BBitmapStringField();

			void				SetBitmap(BBitmap* bitmap);
			const BBitmap*		Bitmap() const
									{ return fBitmap; }

private:
			BBitmap*			fBitmap;
};


// BColumn for PartitionListView which knows how to render
// a BBitmapStringField
class PartitionColumn : public BTitledColumn {
	typedef BTitledColumn Inherited;
public:
								PartitionColumn(const char* title,
									float width, float minWidth,
									float maxWidth, uint32 truncateMode,
									alignment align = B_ALIGN_LEFT);

	virtual	void				DrawField(BField* field, BRect rect,
									BView* parent);
	virtual float				GetPreferredWidth(BField* field, BView* parent) const;

	virtual	bool				AcceptsField(const BField* field) const;

	static	void				InitTextMargin(BView* parent);

private:
			uint32				fTruncateMode;
	static	float				sTextMargin;
};


// BRow for the PartitionListView
class PartitionListRow : public BRow {
	typedef BRow Inherited;
public:
								PartitionListRow(BPartition* partition);
								PartitionListRow(partition_id parentID,
									partition_id id, off_t offset, off_t size);

			partition_id		ID() const
									{ return fPartitionID; }
			partition_id		ParentID() const
									{ return fParentID; }
			off_t				Offset() const
									{ return fOffset; }
			off_t				Size() const
									{ return fSize; }

			const char*			DevicePath();

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

	virtual	void				AttachedToWindow();

	virtual	bool				InitiateDrag(BPoint rowPoint, bool wasSelected);

			PartitionListRow*	FindRow(partition_id id,
									PartitionListRow* parent = NULL);
			PartitionListRow*	AddPartition(BPartition* partition);
			PartitionListRow*	AddSpace(partition_id parent,
									partition_id id, off_t offset, off_t size);

private:
			int32				_InsertIndexForOffset(PartitionListRow* parent,
									off_t offset) const;
};


#endif // PARTITIONLIST_H
