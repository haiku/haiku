/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "table/Table.h"

#include <new>


// #pragma mark - TableField


class TableField : public BField {
public:
	TableField(int32 rowIndex)
		:
		fRowIndex(rowIndex)
	{
	}

	int32 RowIndex() const
	{
		return fRowIndex;
	}

	void SetRowIndex(int32 rowIndex)
	{
		fRowIndex = rowIndex;
	}

private:
	int32	fRowIndex;
};


// #pragma mark - TableModelListener


TableModelListener::~TableModelListener()
{
}


void
TableModelListener::TableRowsAdded(TableModel* model, int32 rowIndex,
	int32 count)
{
}


void
TableModelListener::TableRowsRemoved(TableModel* model, int32 rowIndex,
	int32 count)
{
}


void
TableModelListener::TableRowsChanged(TableModel* model, int32 rowIndex,
	int32 count)
{
}


// #pragma mark - TableModel


TableModel::~TableModel()
{
}


bool
TableModel::AddListener(TableModelListener* listener)
{
	return fListeners.AddItem(listener);
}


void
TableModel::RemoveListener(TableModelListener* listener)
{
	fListeners.RemoveItem(listener);
}


void
TableModel::NotifyRowsAdded(int32 rowIndex, int32 count)
{
	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--) {
		TableModelListener* listener = fListeners.ItemAt(i);
		listener->TableRowsAdded(this, rowIndex, count);
	}
}


void
TableModel::NotifyRowsRemoved(int32 rowIndex, int32 count)
{
	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--) {
		TableModelListener* listener = fListeners.ItemAt(i);
		listener->TableRowsRemoved(this, rowIndex, count);
	}
}


void
TableModel::NotifyRowsChanged(int32 rowIndex, int32 count)
{
	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--) {
		TableModelListener* listener = fListeners.ItemAt(i);
		listener->TableRowsChanged(this, rowIndex, count);
	}
}


// #pragma mark - TableSelectionModel

TableSelectionModel::TableSelectionModel(Table* table)
	:
	fTable(table),
	fRows(NULL),
	fRowCount(-1)
{
}


TableSelectionModel::~TableSelectionModel()
{
	delete[] fRows;
}


int32
TableSelectionModel::CountRows()
{
	_Update();

	return fRowCount;
}


int32
TableSelectionModel::RowAt(int32 index)
{
	_Update();

	return index >= 0 && index < fRowCount ? fRows[index] : -1;
}


void
TableSelectionModel::_SelectionChanged()
{
	if (fRowCount >= 0) {
		fRowCount = -1;
		delete[] fRows;
		fRows = NULL;
	}
}


void
TableSelectionModel::_Update()
{
	if (fRowCount >= 0)
		return;

	// count the rows
	fRowCount = 0;
	BRow* row = NULL;
	while ((row = fTable->CurrentSelection(row)) != NULL)
		fRowCount++;

	if (fRowCount == 0)
		return;

	// allocate row array
	fRows = new(std::nothrow) int32[fRowCount];
	if (fRows == NULL) {
		fRowCount = 0;
		return;
	}

	// get the rows
	row = NULL;
	int32 index = 0;
	while ((row = fTable->CurrentSelection(row)) != NULL)
		fRows[index++] = fTable->_ModelIndexOfRow(row);
}


// #pragma mark - TableToolTipProvider


TableToolTipProvider::~TableToolTipProvider()
{
}


// #pragma mark - TableListener


TableListener::~TableListener()
{
}


void
TableListener::TableSelectionChanged(Table* table)
{
}


void
TableListener::TableRowInvoked(Table* table, int32 rowIndex)
{
}


void
TableListener::TableCellMouseDown(Table* table, int32 rowIndex,
	int32 columnIndex, BPoint screenWhere, uint32 buttons)
{
}


void
TableListener::TableCellMouseUp(Table* table, int32 rowIndex, int32 columnIndex,
	BPoint screenWhere, uint32 buttons)
{
}


// #pragma mark - Column


class Table::Column : public AbstractColumn {
public:
								Column(TableModel* model,
									TableColumn* tableColumn);
	virtual						~Column();

	virtual	void				SetModel(AbstractTableModelBase* model);

protected:
	virtual	void				DrawTitle(BRect rect, BView* targetView);
	virtual	void				DrawField(BField* field, BRect rect,
									BView* targetView);
	virtual	int					CompareFields(BField* field1, BField* field2);

	virtual	void				GetColumnName(BString* into) const;
	virtual	float				GetPreferredWidth(BField* field,
									BView* parent) const;

private:
			TableModel*			fModel;
};


Table::Column::Column(TableModel* model, TableColumn* tableColumn)
	:
	AbstractColumn(tableColumn),
	fModel(model)
{
}


Table::Column::~Column()
{
}


void
Table::Column::SetModel(AbstractTableModelBase* model)
{
	fModel = dynamic_cast<TableModel*>(model);
}


void
Table::Column::DrawTitle(BRect rect, BView* targetView)
{
	fTableColumn->DrawTitle(rect, targetView);
}


void
Table::Column::DrawField(BField* _field, BRect rect, BView* targetView)
{
	TableField* field = dynamic_cast<TableField*>(_field);
	if (field == NULL)
		return;

	int32 modelIndex = fTableColumn->ModelIndex();
	BVariant value;
	if (!fModel->GetValueAt(field->RowIndex(), modelIndex, value))
		return;
	fTableColumn->DrawValue(value, rect, targetView);
}


int
Table::Column::CompareFields(BField* _field1, BField* _field2)
{
	TableField* field1 = dynamic_cast<TableField*>(_field1);
	TableField* field2 = dynamic_cast<TableField*>(_field2);

	if (field1 == field2)
		return 0;

	if (field1 == NULL)
		return -1;
	if (field2 == NULL)
		return 1;

	int32 modelIndex = fTableColumn->ModelIndex();
	BVariant value1;
	bool valid1 = fModel->GetValueAt(field1->RowIndex(), modelIndex, value1);
	BVariant value2;
	bool valid2 = fModel->GetValueAt(field2->RowIndex(), modelIndex, value2);

	if (!valid1)
		return valid2 ? -1 : 0;
	if (!valid2)
		return 1;

	return fTableColumn->CompareValues(value1, value2);
}


void
Table::Column::GetColumnName(BString* into) const
{
	fTableColumn->GetColumnName(into);
}


float
Table::Column::GetPreferredWidth(BField* _field, BView* parent) const
{
	TableField* field = dynamic_cast<TableField*>(_field);
	if (field == NULL)
		return Width();

	int32 modelIndex = fTableColumn->ModelIndex();
	BVariant value;
	if (!fModel->GetValueAt(field->RowIndex(), modelIndex, value))
		return Width();
	return fTableColumn->GetPreferredWidth(value, parent);
}


// #pragma mark - Table


Table::Table(const char* name, uint32 flags, border_style borderStyle,
	bool showHorizontalScrollbar)
	:
	AbstractTable(name, flags, borderStyle, showHorizontalScrollbar),
	fModel(NULL),
	fToolTipProvider(NULL),
	fSelectionModel(this),
	fIgnoreSelectionChange(0)
{
}


Table::Table(TableModel* model, const char* name, uint32 flags,
	border_style borderStyle, bool showHorizontalScrollbar)
	:
	AbstractTable(name, flags, borderStyle, showHorizontalScrollbar),
	fModel(NULL),
	fToolTipProvider(NULL),
	fSelectionModel(this),
	fIgnoreSelectionChange(0)
{
	SetTableModel(model);
}


Table::~Table()
{
	for (int32 i = CountColumns() - 1; i >= 0; i--)
		RemoveColumn(ColumnAt(i));

	// rows are deleted by the BColumnListView destructor automatically
}


void
Table::SetTableModel(TableModel* model)
{
	if (model == fModel)
		return;

	if (fModel != NULL) {
		fModel->RemoveListener(this);

		fRows.MakeEmpty();
		Clear();

		for (int32 i = 0; AbstractColumn* column = fColumns.ItemAt(i); i++)
			column->SetModel(NULL);
	}

	fModel = model;

	if (fModel == NULL)
		return;

	fModel->AddListener(this);

	for (int32 i = 0; AbstractColumn* column = fColumns.ItemAt(i); i++)
		column->SetModel(fModel);

	TableRowsAdded(fModel, 0, fModel->CountRows());
}


void
Table::SetToolTipProvider(TableToolTipProvider* toolTipProvider)
{
	fToolTipProvider = toolTipProvider;
}


TableSelectionModel*
Table::SelectionModel()
{
	return &fSelectionModel;
}


void
Table::SelectRow(int32 rowIndex, bool extendSelection)
{
	BRow* row = fRows.ItemAt(rowIndex);
	if (row == NULL)
		return;

	if (!extendSelection) {
		fIgnoreSelectionChange++;
		DeselectAll();
		fIgnoreSelectionChange--;
	}

	AddToSelection(row);
}


void
Table::DeselectRow(int32 rowIndex)
{
	if (BRow* row = fRows.ItemAt(rowIndex))
		Deselect(row);
}


void
Table::DeselectAllRows()
{
	DeselectAll();
}


bool
Table::AddTableListener(TableListener* listener)
{
	return fListeners.AddItem(listener);
}


void
Table::RemoveTableListener(TableListener* listener)
{
	fListeners.RemoveItem(listener);
}


bool
Table::GetToolTipAt(BPoint point, BToolTip** _tip)
{
	if (fToolTipProvider == NULL)
		return AbstractTable::GetToolTipAt(point, _tip);

	// get the table row
	BRow* row = RowAt(point);
	int32 rowIndex = row != NULL ? _ModelIndexOfRow(row) : -1;
	if (rowIndex < 0)
		return AbstractTable::GetToolTipAt(point, _tip);

	// get the table column
	BColumn* column = ColumnAt(point);
	int32 columnIndex = column != NULL ? column->LogicalFieldNum() : -1;

	return fToolTipProvider->GetToolTipForTableCell(rowIndex, columnIndex,
		_tip);
}


void
Table::SelectionChanged()
{
	if (fIgnoreSelectionChange > 0)
		return;

	fSelectionModel._SelectionChanged();

	if (!fListeners.IsEmpty()) {
		int32 listenerCount = fListeners.CountItems();
		for (int32 i = listenerCount - 1; i >= 0; i--)
			fListeners.ItemAt(i)->TableSelectionChanged(this);
	}
}


AbstractTable::AbstractColumn*
Table::CreateColumn(TableColumn* column)
{
	return new Column(fModel, column);
}


void
Table::ColumnMouseDown(AbstractColumn* column, BRow* row, BField* field,
	BPoint screenWhere, uint32 buttons)
{
	if (!fListeners.IsEmpty()) {
		// get the table row and column indices
		int32 rowIndex = _ModelIndexOfRow(row);
		int32 columnIndex = column->LogicalFieldNum();
		if (rowIndex < 0 || columnIndex < 0)
			return;

		// notify listeners
		int32 listenerCount = fListeners.CountItems();
		for (int32 i = listenerCount - 1; i >= 0; i--) {
			fListeners.ItemAt(i)->TableCellMouseDown(this, rowIndex,
				columnIndex, screenWhere, buttons);
		}
	}
}


void
Table::ColumnMouseUp(AbstractColumn* column, BRow* row, BField* field,
	BPoint screenWhere, uint32 buttons)
{
	if (!fListeners.IsEmpty()) {
		// get the table row and column indices
		int32 rowIndex = _ModelIndexOfRow(row);
		int32 columnIndex = column->LogicalFieldNum();
		if (rowIndex < 0 || columnIndex < 0)
			return;

		// notify listeners
		int32 listenerCount = fListeners.CountItems();
		for (int32 i = listenerCount - 1; i >= 0; i--) {
			fListeners.ItemAt(i)->TableCellMouseUp(this, rowIndex, columnIndex,
				screenWhere, buttons);
		}
	}
}


void
Table::TableRowsAdded(TableModel* model, int32 rowIndex, int32 count)
{
	// create the rows
	int32 endRow = rowIndex + count;
	int32 columnCount = fModel->CountColumns();

	for (int32 i = rowIndex; i < endRow; i++) {
		// create row
		BRow* row = new(std::nothrow) BRow();
		if (row == NULL) {
			// TODO: Report error!
			return;
		}

		// add dummy fields to row
		for (int32 columnIndex = 0; columnIndex < columnCount; columnIndex++) {
			// It would be nice to create only a single field and set it for all
			// columns, but the row takes ultimate ownership, so it have to be
			// individual objects.
			TableField* field = new(std::nothrow) TableField(i);
			if (field == NULL) {
				// TODO: Report error!
				delete row;
				return;
			}

			row->SetField(field, columnIndex);
		}

		// add row
		if (!fRows.AddItem(row, i)) {
			// TODO: Report error!
			delete row;
			return;
		}

		AddRow(row, i);
	}

	// re-index the subsequent rows
	_UpdateRowIndices(endRow);
}


void
Table::TableRowsRemoved(TableModel* model, int32 rowIndex, int32 count)
{
	for (int32 i = rowIndex + count - 1; i >= rowIndex; i--) {
		if (BRow* row = fRows.RemoveItemAt(i)) {
			RemoveRow(row);
			delete row;
		}
	}

	// re-index the subsequent rows
	_UpdateRowIndices(rowIndex);
}


void
Table::TableRowsChanged(TableModel* model, int32 rowIndex, int32 count)
{
	int32 endIndex = rowIndex + count;
	for (int32 i = rowIndex; i < endIndex; i++) {
		if (BRow* row = fRows.ItemAt(i))
			UpdateRow(row);
	}
}


void
Table::ItemInvoked()
{
	if (fListeners.IsEmpty())
		return;

	int32 index = _ModelIndexOfRow(CurrentSelection());
	if (index < 0)
		return;

	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--)
		fListeners.ItemAt(i)->TableRowInvoked(this, index);
}


void
Table::_UpdateRowIndices(int32 fromIndex)
{
	for (int32 i = fromIndex; BRow* row = fRows.ItemAt(i); i++) {
		for (int32 k = 0;
			TableField* field = dynamic_cast<TableField*>(row->GetField(k));
			k++) {
			field->SetRowIndex(i);
		}
	}
}


int32
Table::_ModelIndexOfRow(BRow* row)
{
	if (row == NULL)
		return -1;

	TableField* field = dynamic_cast<TableField*>(row->GetField(0));
	if (field == NULL)
		return -1;

	return field->RowIndex();
}
