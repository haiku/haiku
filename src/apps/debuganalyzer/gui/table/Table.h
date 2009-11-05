/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TABLE_H
#define TABLE_H

#include <ColumnTypes.h>

#include <ObjectList.h>
#include <util/DoublyLinkedList.h>
#include <Variant.h>

#include "table/AbstractTable.h"
#include "table/TableColumn.h"


class Table;
class TableModel;


class TableModelListener {
public:
	virtual						~TableModelListener();

	virtual	void				TableRowsAdded(TableModel* model,
									int32 rowIndex, int32 count);
	virtual	void				TableRowsRemoved(TableModel* model,
									int32 rowIndex, int32 count);
	virtual	void				TableRowsChanged(TableModel* model,
									int32 rowIndex, int32 count);
};


class TableModel : public AbstractTableModelBase {
public:
	virtual						~TableModel();

	virtual	int32				CountRows() const = 0;

	virtual	bool				GetValueAt(int32 rowIndex, int32 columnIndex,
									BVariant& value) = 0;

	virtual	bool				AddListener(TableModelListener* listener);
	virtual	void				RemoveListener(TableModelListener* listener);

protected:
			typedef BObjectList<TableModelListener> ListenerList;

protected:
			void				NotifyRowsAdded(int32 rowIndex, int32 count);
			void				NotifyRowsRemoved(int32 rowIndex, int32 count);
			void				NotifyRowsChanged(int32 rowIndex, int32 count);

protected:
			ListenerList		fListeners;
};


class TableSelectionModel {
public:
								TableSelectionModel(Table* table);
								~TableSelectionModel();

			int32				CountRows();
			int32				RowAt(int32 index);

private:
			friend class Table;

private:
			void				_SelectionChanged();
			void				_Update();

private:
			Table*				fTable;
			int32*				fRows;
			int32				fRowCount;
};


class TableToolTipProvider {
public:
	virtual						~TableToolTipProvider();

	virtual	bool				GetToolTipForTableCell(int32 rowIndex,
									int32 columnIndex, BToolTip** _tip) = 0;
									// columnIndex can be -1, if not in a column
};


class TableListener {
public:
	virtual						~TableListener();

	virtual	void				TableSelectionChanged(Table* table);
	virtual	void				TableRowInvoked(Table* table, int32 rowIndex);

	virtual	void				TableCellMouseDown(Table* table, int32 rowIndex,
									int32 columnIndex, BPoint screenWhere,
									uint32 buttons);
	virtual	void				TableCellMouseUp(Table* table, int32 rowIndex,
									int32 columnIndex, BPoint screenWhere,
									uint32 buttons);
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

			void				SetToolTipProvider(
									TableToolTipProvider* toolTipProvider);
			TableToolTipProvider* ToolTipProvider() const
									{ return fToolTipProvider; }

			TableSelectionModel* SelectionModel();

			void				SelectRow(int32 rowIndex, bool extendSelection);
			void				DeselectRow(int32 rowIndex);
			void				DeselectAllRows();

			bool				AddTableListener(TableListener* listener);
			void				RemoveTableListener(TableListener* listener);

protected:
	virtual bool				GetToolTipAt(BPoint point, BToolTip** _tip);

	virtual	void				SelectionChanged();

	virtual	AbstractColumn*		CreateColumn(TableColumn* column);

	virtual	void				ColumnMouseDown(AbstractColumn* column,
									BRow* row, BField* field,
									BPoint screenWhere, uint32 buttons);
	virtual	void				ColumnMouseUp(AbstractColumn* column,
									BRow* row, BField* field,
									BPoint screenWhere, uint32 buttons);

private:
	virtual	void				TableRowsAdded(TableModel* model,
									int32 rowIndex, int32 count);
	virtual	void				TableRowsRemoved(TableModel* model,
									int32 rowIndex, int32 count);
	virtual	void				TableRowsChanged(TableModel* model,
									int32 rowIndex, int32 count);

private:
			class Column;

			friend class TableSelectionModel;

			typedef BObjectList<TableListener>	ListenerList;
			typedef BObjectList<BRow> RowList;

private:
	virtual	void				ItemInvoked();

			void				_UpdateRowIndices(int32 fromIndex);
			int32				_ModelIndexOfRow(BRow* row);

private:
			TableModel*			fModel;
			TableToolTipProvider* fToolTipProvider;
			RowList				fRows;
			TableSelectionModel	fSelectionModel;
			ListenerList		fListeners;
			int32				fIgnoreSelectionChange;
};


#endif	// TABLE_H
