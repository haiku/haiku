/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_H
#define TABLE_H

#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <ObjectList.h>


class Table;


class TableModel {
public:
	virtual						~TableModel();

	virtual	int32				CountColumns() const = 0;
	virtual	int32				CountRows() const = 0;

	virtual	void				GetColumnName(int columnIndex,
									BString& name) const = 0;

	virtual	void*				ValueAt(int32 rowIndex, int32 columnIndex) = 0;
};


class TableColumn : private BColumn {
public:
								TableColumn(BColumn* columnDelegate,
									int32 modelIndex, float width,
									float minWidth, float maxWidth,
									alignment align);
	virtual						~TableColumn();

			int32				ModelIndex() const	{ return fModelIndex; }

			float				Width() const		{ return BColumn::Width(); }

protected:
	virtual	void				DrawTitle(BRect rect, BView* targetView);
	virtual	void				GetColumnName(BString* into) const;

	virtual	void				DrawValue(void* value, BRect rect,
									BView* targetView);
	virtual	int					CompareValues(void* a, void* b);
	virtual	float				GetPreferredValueWidth(void* value,
									BView* parent) const;

private:
	virtual	void				DrawField(BField* field, BRect rect,
									BView* targetView);
	virtual	int					CompareFields(BField* field1, BField* field2);

	virtual	float				GetPreferredWidth(BField* field,
									BView* parent) const;

			void				SetTableModel(TableModel* model);
									// package private
			
private:
			friend class Table;

private:
			TableModel*			fModel;
			BColumn*			fColumnDelegate;
			int32				fModelIndex;
};


class StringTableColumn : public TableColumn {
public:
								StringTableColumn(int32 modelIndex,
									const char* title, float width,
									float minWidth, float maxWidth,
									uint32 truncate,
									alignment align = B_ALIGN_LEFT);
	virtual						~StringTableColumn();

protected:
	virtual	void				DrawValue(void* value, BRect rect,
									BView* targetView);
	virtual	int					CompareValues(void* a, void* b);
	virtual	float				GetPreferredValueWidth(void* value,
									BView* parent) const;

private:
			BStringColumn		fColumn;
	mutable	BStringField		fField;
};


class Table : private BColumnListView {
public:
								Table(const char* name, uint32 flags,
									border_style borderStyle = B_NO_BORDER,
									bool showHorizontalScrollbar = true);
								Table(TableModel* model, const char* name,
									uint32 flags,
									border_style borderStyle = B_NO_BORDER,
									bool showHorizontalScrollbar = true);
	virtual						~Table();

			BView*				ToView()				{ return this; }

			void				SetTableModel(TableModel* model);
			TableModel*			GetTableModel() const	{ return fModel; }

			void				AddColumn(TableColumn* column);

private:
			typedef BObjectList<TableColumn>	ColumnList;

private:
			TableModel*			fModel;
			ColumnList			fColumns;
};



#endif	// TABLE_H
