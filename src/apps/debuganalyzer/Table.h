/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_H
#define TABLE_H

#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <ObjectList.h>

#include "Variant.h"


class Table;


class TableModel {
public:
	virtual						~TableModel();

	virtual	int32				CountColumns() const = 0;
	virtual	int32				CountRows() const = 0;

	virtual	bool				GetValueAt(int32 rowIndex, int32 columnIndex,
									Variant& value) = 0;
};


class TableColumn : protected BColumn {
public:
								TableColumn(int32 modelIndex, float width,
									float minWidth, float maxWidth,
									alignment align);
	virtual						~TableColumn();

			int32				ModelIndex() const	{ return fModelIndex; }

			float				Width() const		{ return BColumn::Width(); }

protected:
	virtual	void				DrawValue(const Variant& value, BRect rect,
									BView* targetView);
	virtual	int					CompareValues(const Variant& a,
									const Variant& b);
	virtual	float				GetPreferredValueWidth(const Variant& value,
									BView* parent) const;

protected:
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
			int32				fModelIndex;
};


class DelagateBasedTableColumn : public TableColumn {
public:
								DelagateBasedTableColumn(
									BColumn* columnDelegate,
									int32 modelIndex, float width,
									float minWidth, float maxWidth,
									alignment align);
	virtual						~DelagateBasedTableColumn();

protected:
	virtual	void				DrawTitle(BRect rect, BView* targetView);
	virtual	void				GetColumnName(BString* into) const;

	virtual	void				DrawValue(const Variant& value, BRect rect,
									BView* targetView);
	virtual	float				GetPreferredValueWidth(const Variant& value,
									BView* parent) const;

	virtual	BField*				PrepareField(const Variant& value) const = 0;

protected:
			BColumn*			fColumnDelegate;
};


class StringTableColumn : public DelagateBasedTableColumn {
public:
								StringTableColumn(int32 modelIndex,
									const char* title, float width,
									float minWidth, float maxWidth,
									uint32 truncate,
									alignment align = B_ALIGN_LEFT);
	virtual						~StringTableColumn();

protected:
	virtual	BField*				PrepareField(const Variant& value) const;

	virtual	int					CompareValues(const Variant& a,
									const Variant& b);

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
