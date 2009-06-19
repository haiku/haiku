/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ABSTRACT_TABLE_H
#define ABSTRACT_TABLE_H

#include <ObjectList.h>
#include <ColumnListView.h>


class TableColumn;


class AbstractTableModelBase {
public:
	virtual						~AbstractTableModelBase();

	virtual	int32				CountColumns() const = 0;
};


class AbstractTable : protected BColumnListView {
public:
								AbstractTable(const char* name, uint32 flags,
									border_style borderStyle = B_NO_BORDER,
									bool showHorizontalScrollbar = true);
	virtual						~AbstractTable();

			BView*				ToView()				{ return this; }

	virtual	void				AddColumn(TableColumn* column);
			void				ResizeColumnToPreferred(int32 index);
			void				ResizeAllColumnsToPreferred();

			list_view_type		SelectionMode() const;
			void				SetSelectionMode(list_view_type type);
			void 				DeselectAll();

			void				SetSortingEnabled(bool enabled);
			bool				SortingEnabled() const;
			void				SetSortColumn(TableColumn* column, bool add,
									bool ascending);
			void				ClearSortColumns();

protected:
			class AbstractColumn;

			typedef BObjectList<AbstractColumn>		ColumnList;

protected:
	virtual	AbstractColumn*		CreateColumn(TableColumn* column) = 0;

			AbstractColumn*		GetColumn(TableColumn* column) const;

protected:
			ColumnList			fColumns;
};


// implementation private

class AbstractTable::AbstractColumn : public BColumn {
public:
								AbstractColumn(TableColumn* tableColumn);
	virtual						~AbstractColumn();

	virtual	void				SetModel(AbstractTableModelBase* model) = 0;

			TableColumn*		GetTableColumn() const	{ return fTableColumn; }

protected:
			TableColumn*		fTableColumn;
};


#endif	// ABSTRACT_TABLE_H
