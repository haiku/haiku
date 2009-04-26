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


class Table::Column : public BColumn {
public:
								Column(TableModel* model,
									TableColumn* tableColumn);
	virtual						~Column();

			void				SetModel(TableModel* model);

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
			TableColumn*		fTableColumn;
};


Table::Column::Column(TableModel* model, TableColumn* tableColumn)
	:
	BColumn(tableColumn->Width(), tableColumn->MinWidth(),
		tableColumn->MaxWidth(), tableColumn->Alignment()),
	fModel(model),
	fTableColumn(tableColumn)
{
}


Table::Column::~Column()
{
	delete fTableColumn;
}


void
Table::Column::SetModel(TableModel* model)
{
	fModel = model;
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

		for (int32 i = 0; Column* column = fColumns.ItemAt(i); i++)
			column->SetModel(NULL);
	}

	fModel = model;

	if (fModel == NULL)
		return;

	for (int32 i = 0; Column* column = fColumns.ItemAt(i); i++)
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


void
Table::AddColumn(TableColumn* column)
{
	if (column == NULL)
		return;

	Column* privateColumn = new Column(fModel, column);

	if (!fColumns.AddItem(privateColumn)) {
		delete privateColumn;
		throw std::bad_alloc();
	}

	BColumnListView::AddColumn(privateColumn, column->ModelIndex());
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

	TableField* field = dynamic_cast<TableField*>(row->GetField(0));
	if (field == NULL)
		return;

	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--)
		fListeners.ItemAt(i)->TableRowInvoked(this, field->RowIndex());
}
