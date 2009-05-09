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


// #pragma mark - TableListener


TableListener::~TableListener()
{
}


void
TableListener::TableRowInvoked(Table* table, int32 rowIndex)
{
}


// #pragma mark - Column


class Table::Column : public AbstractColumn {
public:
								Column(TableModel* model,
									TableColumn* tableColumn);
	virtual						~Column();

	virtual	void				SetModel(AbstractTableModel* model);

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
Table::Column::SetModel(AbstractTableModel* model)
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
	Variant value;
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
	Variant value1;
	bool valid1 = fModel->GetValueAt(field1->RowIndex(), modelIndex, value1);
	Variant value2;
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
	Variant value;
	if (!fModel->GetValueAt(field->RowIndex(), modelIndex, value))
		return Width();
	return fTableColumn->GetPreferredWidth(value, parent);
}


// #pragma mark - Table


Table::Table(const char* name, uint32 flags, border_style borderStyle,
	bool showHorizontalScrollbar)
	:
	AbstractTable(name, flags, borderStyle, showHorizontalScrollbar),
	fModel(NULL)
{
}


Table::Table(TableModel* model, const char* name, uint32 flags,
	border_style borderStyle, bool showHorizontalScrollbar)
	:
	AbstractTable(name, flags, borderStyle, showHorizontalScrollbar),
	fModel(NULL)
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

		for (int32 i = 0; AbstractColumn* column = fColumns.ItemAt(i); i++)
			column->SetModel(NULL);
	}

	fModel = model;

	if (fModel == NULL)
		return;

	for (int32 i = 0; AbstractColumn* column = fColumns.ItemAt(i); i++)
		column->SetModel(fModel);

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


AbstractTable::AbstractColumn*
Table::CreateColumn(TableColumn* column)
{
	return new Column(fModel, column);
}


void
Table::ItemInvoked()
{
	if (fListeners.IsEmpty())
		return;

	BRow* row = CurrentSelection();
	if (row == NULL)
		return;

	TableField* field = dynamic_cast<TableField*>(row->GetField(0));
	if (field == NULL)
		return;

	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--)
		fListeners.ItemAt(i)->TableRowInvoked(this, field->RowIndex());
}
