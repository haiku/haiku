/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_H
#define TABLE_H

#include <ColumnTypes.h>

#include <ObjectList.h>
#include <util/DoublyLinkedList.h>

#include "table/AbstractTable.h"
#include "table/TableColumn.h"

#include "Variant.h"


class Table;
class TableModel;


class TableModelListener : public DoublyLinkedListLinkImpl<TableModelListener> {
public:
	virtual						~TableModelListener();

	virtual	void				TableRowsAdded(TableModel* model,
									int32 rowIndex, int32 count);
	virtual	void				TableRowsRemoved(TableModel* model,
									int32 rowIndex, int32 count);
};


class TableModel : public AbstractTableModelBase {
public:
	virtual						~TableModel();

	virtual	int32				CountRows() const = 0;

	virtual	bool				GetValueAt(int32 rowIndex, int32 columnIndex,
									Variant& value) = 0;

	virtual	void				AddListener(TableModelListener* listener);
	virtual	void				RemoveListener(TableModelListener* listener);

protected:
			typedef DoublyLinkedList<TableModelListener> ListenerList;

protected:
			void				NotifyRowsAdded(int32 rowIndex, int32 count);
			void				NotifyRowsRemoved(int32 rowIndex, int32 count);

protected:
			ListenerList		fListeners;
};


class TableListener {
public:
	virtual						~TableListener();

	virtual	void				TableRowInvoked(Table* table, int32 rowIndex);
};


class Table : public AbstractTable, private TableModelListener {
public:
								Table(const char* name, uint32 flags,
									border_style borderStyle = B_NO_BORDER,
									bool showHorizontalScrollbar = true);
								Table(TableModel* model, const char* name,
									uint32 flags,
									border_style borderStyle = B_NO_BORDER,
									bool showHorizontalScrollbar = true);
	virtual						~Table();

			void				SetTableModel(TableModel* model);
			TableModel*			GetTableModel() const	{ return fModel; }

			bool				AddTableListener(TableListener* listener);
			void				RemoveTableListener(TableListener* listener);

protected:
	virtual	AbstractColumn*		CreateColumn(TableColumn* column);

private:
	virtual	void				TableRowsAdded(TableModel* model,
									int32 rowIndex, int32 count);
	virtual	void				TableRowsRemoved(TableModel* model,
									int32 rowIndex, int32 count);

private:
			class Column;

			typedef BObjectList<TableListener>	ListenerList;
			typedef BObjectList<BRow> RowList;

private:
	virtual	void				ItemInvoked();

private:
			TableModel*			fModel;
			RowList				fRows;
			ListenerList		fListeners;
};


#endif	// TABLE_H
