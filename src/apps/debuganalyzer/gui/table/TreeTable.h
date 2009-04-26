/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TREE_TABLE_H
#define TREE_TABLE_H

#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <ObjectList.h>

#include "table/TableColumn.h"

#include "Variant.h"


class TreeTable;


class TreeTableModel {
public:
	virtual						~TreeTableModel();

	virtual	void*				Root() const = 0;
									// the root itself isn't shown

	virtual	int32				CountChildren(void* parent) const = 0;
	virtual	void*				ChildAt(void* parent, int32 index) const = 0;

	virtual	int32				CountColumns() const = 0;

	virtual	bool				GetValueAt(void* object, int32 columnIndex,
									Variant& value) = 0;
};


class TreeTableListener {
public:
	virtual						~TreeTableListener();

	virtual	void				TreeTableNodeInvoked(TreeTable* table,
									void* node);
};


class TreeTable : private BColumnListView {
public:
								TreeTable(const char* name, uint32 flags,
									border_style borderStyle = B_NO_BORDER,
									bool showHorizontalScrollbar = true);
								TreeTable(TreeTableModel* model,
									const char* name, uint32 flags,
									border_style borderStyle = B_NO_BORDER,
									bool showHorizontalScrollbar = true);
	virtual						~TreeTable();

			BView*				ToView()				{ return this; }

			void				SetTreeTableModel(TreeTableModel* model);
			TreeTableModel*		GetTreeTableModel() const	{ return fModel; }

			void				AddColumn(TableColumn* column);

			bool				AddTreeTableListener(
									TreeTableListener* listener);
			void				RemoveTreeTableListener(
									TreeTableListener* listener);

private:
			class Column;

			typedef BObjectList<Column>				ColumnList;
			typedef BObjectList<TreeTableListener>	ListenerList;

private:
	virtual	void				ItemInvoked();

			void				_AddChildRows(void* parent, BRow* parentRow,
									int32 columnCount);


private:
			TreeTableModel*		fModel;
			ColumnList			fColumns;
			ListenerList		fListeners;
};


#endif	// TREE_TABLE_H
