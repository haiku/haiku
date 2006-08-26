/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_GRID_LAYOUT_H
#define	_GRID_LAYOUT_H

#include <TwoDimensionalLayout.h>



class BGridLayout : public BTwoDimensionalLayout {
public:
								BGridLayout(float horizontal = 0.0f,
									float vertical = 0.0f);
	virtual						~BGridLayout();

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

	virtual	BLayoutItem*		AddView(BView* child);
	virtual	BLayoutItem*		AddView(int32 index, BView* child);
	virtual	BLayoutItem*		AddView(BView* child, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);

	virtual	bool				AddItem(BLayoutItem* item);
	virtual	bool				AddItem(int32 index, BLayoutItem* item);
	virtual	bool				AddItem(BLayoutItem* item, int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);

protected:	
	virtual	void				ItemAdded(BLayoutItem* item);
	virtual	void				ItemRemoved(BLayoutItem* item);

	virtual	bool				HasMultiColumnItems();
	virtual	bool				HasMultiRowItems();
	
	virtual	int32				InternalCountColumns();
	virtual	int32				InternalCountRows();
	virtual	void				GetColumnRowConstraints(
									enum orientation orientation,
									int32 index,
									ColumnRowConstraints* constraints);
	virtual	void	 			GetItemDimensions(BLayoutItem* item,
									Dimensions* dimensions);

private:	
			class DummyLayoutItem;
			class RowInfoArray;
			struct ItemLayoutData;
			
			bool				_IsGridCellEmpty(int32 column, int32 row);
			bool				_AreGridCellsEmpty(int32 column, int32 row,
									int32 columnCount, int32 rowCount);
	
			bool				_ResizeGrid(int32 columnCount, int32 rowCount);

			ItemLayoutData*		_LayoutDataForItem(BLayoutItem* item) const;

private:
			BLayoutItem***		fGrid;
			int32				fColumnCount;
			int32				fRowCount;

			RowInfoArray*		fRowInfos;
			RowInfoArray*		fColumnInfos;

			int32				fMultiColumnItems;
			int32				fMultiRowItems;
};

#endif	// _GRID_LAYOUT_H
