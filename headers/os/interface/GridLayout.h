/*
 * Copyright 2006-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_GRID_LAYOUT_H
#define	_GRID_LAYOUT_H


#include <TwoDimensionalLayout.h>


class BGridLayout : public BTwoDimensionalLayout {
public:
								BGridLayout(float horizontal
										= B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
								BGridLayout(BMessage* from);
	virtual						~BGridLayout();

			int32				CountColumns() const;
			int32				CountRows() const;

			float				HorizontalSpacing() const;
			float				VerticalSpacing() const;

			void				SetHorizontalSpacing(float spacing);
			void				SetVerticalSpacing(float spacing);
			void				SetSpacing(float horizontal, float vertical);

			float				ColumnWeight(int32 column) const;
			void				SetColumnWeight(int32 column, float weight);

			float				MinColumnWidth(int32 column) const;
			void				SetMinColumnWidth(int32 column, float width);

			float				MaxColumnWidth(int32 column) const;
			void				SetMaxColumnWidth(int32 column, float width);

			float				RowWeight(int32 row) const;
			void				SetRowWeight(int32 row, float weight);

			float				MinRowHeight(int row) const;
			void				SetMinRowHeight(int32 row, float height);

			float				MaxRowHeight(int32 row) const;
			void				SetMaxRowHeight(int32 row, float height);

			BLayoutItem*		ItemAt(int32 column, int32 row) const;

	virtual	BLayoutItem*		AddView(BView* child);
	virtual	BLayoutItem*		AddView(int32 index, BView* child);
	virtual	BLayoutItem*		AddView(BView* child, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);

	virtual	bool				AddItem(BLayoutItem* item);
	virtual	bool				AddItem(int32 index, BLayoutItem* item);
	virtual	bool				AddItem(BLayoutItem* item, int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;
	virtual status_t			AllArchived(BMessage* into) const;
	virtual	status_t			AllUnarchived(const BMessage* from);
	static	BArchivable*		Instantiate(BMessage* from);

	virtual status_t			ItemArchived(BMessage* into,
									BLayoutItem* item, int32 index) const;
	virtual status_t			ItemUnarchived(const BMessage* from,
									BLayoutItem* item, int32 index);

	virtual	status_t			Perform(perform_code d, void* arg);

protected:
	virtual	bool				ItemAdded(BLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(BLayoutItem* item, int32 fromIndex);

	virtual	bool				HasMultiColumnItems();
	virtual	bool				HasMultiRowItems();

	virtual	int32				InternalCountColumns();
	virtual	int32				InternalCountRows();
	virtual	void				GetColumnRowConstraints(
									enum orientation orientation,
									int32 index,
									ColumnRowConstraints* constraints);
	virtual	void				GetItemDimensions(BLayoutItem* item,
									Dimensions* dimensions);
private:
			class DummyLayoutItem;
			class RowInfoArray;
			struct ItemLayoutData;

			bool				_IsGridCellEmpty(int32 column, int32 row);
			bool				_AreGridCellsEmpty(int32 column, int32 row,
									int32 columnCount, int32 rowCount);

			bool				_InsertItemIntoGrid(BLayoutItem* item);
			bool				_ResizeGrid(int32 columnCount, int32 rowCount);

			ItemLayoutData*		_LayoutDataForItem(BLayoutItem* item) const;

private:

	// FBC padding
	virtual	void				_ReservedGridLayout1();
	virtual	void				_ReservedGridLayout2();
	virtual	void				_ReservedGridLayout3();
	virtual	void				_ReservedGridLayout4();
	virtual	void				_ReservedGridLayout5();
	virtual	void				_ReservedGridLayout6();
	virtual	void				_ReservedGridLayout7();
	virtual	void				_ReservedGridLayout8();
	virtual	void				_ReservedGridLayout9();
	virtual	void				_ReservedGridLayout10();

	// forbidden methods
								BGridLayout(const BGridLayout&);
			void				operator =(const BGridLayout&);

			BLayoutItem***		fGrid;
			int32				fColumnCount;
			int32				fRowCount;

			RowInfoArray*		fRowInfos;
			RowInfoArray*		fColumnInfos;

			int32				fMultiColumnItems;
			int32				fMultiRowItems;

			uint32				_reserved[5];
};


#endif	// _GRID_LAYOUT_H
