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

private:
	int32	fRowIndex;
};


// #pragma mark - TableModel


TableModel::~TableModel()
{
}


// #pragma mark - TableColumn


TableColumn::TableColumn(int32 modelIndex, float width, float minWidth,
	float maxWidth, alignment align)
	:
	BColumn(width, minWidth, maxWidth, align),
	fModel(NULL),
	fModelIndex(modelIndex)
{
}


TableColumn::~TableColumn()
{
}


void
TableColumn::SetTableModel(TableModel* model)
{
	fModel = model;
}


void
TableColumn::DrawValue(const Variant& value, BRect rect, BView* targetView)
{
}


int
TableColumn::CompareValues(const Variant& a, const Variant& b)
{
	return 0;
}


float
TableColumn::GetPreferredValueWidth(const Variant& value, BView* parent) const
{
	return Width();
}


void
TableColumn::DrawField(BField* _field, BRect rect, BView* targetView)
{
	TableField* field = dynamic_cast<TableField*>(_field);
	if (field == NULL)
		return;

	Variant value;
	if (!fModel->GetValueAt(field->RowIndex(), fModelIndex, value))
		return;
	DrawValue(value, rect, targetView);
}


int
TableColumn::CompareFields(BField* _field1, BField* _field2)
{
	TableField* field1 = dynamic_cast<TableField*>(_field1);
	TableField* field2 = dynamic_cast<TableField*>(_field2);

	if (field1 == field2)
		return 0;

	if (field1 == NULL)
		return -1;
	if (field2 == NULL)
		return 1;

	Variant value1;
	bool valid1 = fModel->GetValueAt(field1->RowIndex(), fModelIndex, value1);
	Variant value2;
	bool valid2 = fModel->GetValueAt(field2->RowIndex(), fModelIndex, value2);

	if (!valid1)
		return valid2 ? -1 : 0;
	if (!valid2)
		return 1;

	return CompareValues(value1, value2);
}


float
TableColumn::GetPreferredWidth(BField* _field, BView* parent) const
{
	TableField* field = dynamic_cast<TableField*>(_field);
	if (field == NULL)
		return Width();

	Variant value;
	if (!fModel->GetValueAt(field->RowIndex(), fModelIndex, value))
		return Width();
	return GetPreferredValueWidth(value, parent);
}


// #pragma mark - TableListener


TableListener::~TableListener()
{
}


void
TableListener::TableRowInvoked(Table* table, int32 rowIndex)
{
}


// #pragma mark - Table


Table::Table(const char* name, uint32 flags, border_style borderStyle,
	bool showHorizontalScrollbar)
	:
	BColumnListView(name, flags, borderStyle, showHorizontalScrollbar),
	fModel(NULL)
{
}


Table::Table(TableModel* model, const char* name, uint32 flags,
	border_style borderStyle, bool showHorizontalScrollbar)
	:
	BColumnListView(name, flags, borderStyle, showHorizontalScrollbar),
	fModel(NULL),
	fColumns(20, true)
{
	SetTableModel(model);
}


Table::~Table()
{
	for (int32 i = CountColumns() - 1; i >= 0; i--)
		RemoveColumn(ColumnAt(i));
}


void
Table::SetTableModel(TableModel* model)
{
	if (model == fModel)
		return;

	if (fModel != NULL) {
		Clear();

		for (int32 i = 0; TableColumn* column = fColumns.ItemAt(i); i++)
			column->SetTableModel(NULL);
	}

	fModel = model;

	if (fModel == NULL)
		return;

	for (int32 i = 0; TableColumn* column = fColumns.ItemAt(i); i++)
		column->SetTableModel(fModel);

	// create the rows
	int32 rowCount = fModel->CountRows();
	int32 columnCount = fModel->CountColumns();

	for (int32 i = 0; i < rowCount; i++) {
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
				return;
			}

			row->SetField(field, columnIndex);
		}

		// add row
		AddRow(row);
	}
}


void
Table::AddColumn(TableColumn* column)
{
	if (column == NULL)
		return;

	if (!fColumns.AddItem(column))
		throw std::bad_alloc();

	column->SetTableModel(fModel);

	BColumnListView::AddColumn(column, column->ModelIndex());
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


void
Table::ItemInvoked()
{
	if (fListeners.IsEmpty())
		return;

	BRow* row = CurrentSelection();
	if (row == NULL)
		return;

	int32 index = IndexOf(row);
	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--)
		fListeners.ItemAt(i)->TableRowInvoked(this, index);
}
