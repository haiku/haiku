/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "table/TreeTable.h"

#include <new>


// #pragma mark - TreeTableField


class TreeTableField : public BField {
public:
	TreeTableField(void* object)
		:
		fObject(object)
	{
	}

	void* Object() const
	{
		return fObject;
	}

private:
	void*	fObject;
};


// #pragma mark - TreeTableModel


TreeTableModel::~TreeTableModel()
{
}


// #pragma mark - TreeTableListener


TreeTableListener::~TreeTableListener()
{
}


void
TreeTableListener::TreeTableNodeInvoked(TreeTable* table, void* node)
{
}


// #pragma mark - Column


class TreeTable::Column : public BColumn {
public:
								Column(TreeTableModel* model,
									TableColumn* tableColumn);
	virtual						~Column();

			void				SetModel(TreeTableModel* model);

protected:
	virtual	void				DrawTitle(BRect rect, BView* targetView);
	virtual	void				DrawField(BField* field, BRect rect,
									BView* targetView);
	virtual	int					CompareFields(BField* field1, BField* field2);

	virtual	void				GetColumnName(BString* into) const;
	virtual	float				GetPreferredWidth(BField* field,
									BView* parent) const;

private:
			TreeTableModel*		fModel;
			TableColumn*		fTableColumn;
};


TreeTable::Column::Column(TreeTableModel* model, TableColumn* tableColumn)
	:
	BColumn(tableColumn->Width(), tableColumn->MinWidth(),
		tableColumn->MaxWidth(), tableColumn->Alignment()),
	fModel(model),
	fTableColumn(tableColumn)
{
}


TreeTable::Column::~Column()
{
	delete fTableColumn;
}


void
TreeTable::Column::SetModel(TreeTableModel* model)
{
	fModel = model;
}


void
TreeTable::Column::DrawTitle(BRect rect, BView* targetView)
{
	fTableColumn->DrawTitle(rect, targetView);
}


void
TreeTable::Column::DrawField(BField* _field, BRect rect, BView* targetView)
{
	TreeTableField* field = dynamic_cast<TreeTableField*>(_field);
	if (field == NULL)
		return;

	int32 modelIndex = fTableColumn->ModelIndex();
	Variant value;
	if (!fModel->GetValueAt(field->Object(), modelIndex, value))
		return;
	fTableColumn->DrawValue(value, rect, targetView);
}


int
TreeTable::Column::CompareFields(BField* _field1, BField* _field2)
{
	TreeTableField* field1 = dynamic_cast<TreeTableField*>(_field1);
	TreeTableField* field2 = dynamic_cast<TreeTableField*>(_field2);

	if (field1 == field2)
		return 0;

	if (field1 == NULL)
		return -1;
	if (field2 == NULL)
		return 1;

	int32 modelIndex = fTableColumn->ModelIndex();
	Variant value1;
	bool valid1 = fModel->GetValueAt(field1->Object(), modelIndex, value1);
	Variant value2;
	bool valid2 = fModel->GetValueAt(field2->Object(), modelIndex, value2);

	if (!valid1)
		return valid2 ? -1 : 0;
	if (!valid2)
		return 1;

	return fTableColumn->CompareValues(value1, value2);
}


void
TreeTable::Column::GetColumnName(BString* into) const
{
	fTableColumn->GetColumnName(into);
}


float
TreeTable::Column::GetPreferredWidth(BField* _field, BView* parent) const
{
	TreeTableField* field = dynamic_cast<TreeTableField*>(_field);
	if (field == NULL)
		return Width();

	int32 modelIndex = fTableColumn->ModelIndex();
	Variant value;
	if (!fModel->GetValueAt(field->Object(), modelIndex, value))
		return Width();
	return fTableColumn->GetPreferredWidth(value, parent);
}


// #pragma mark - Table


TreeTable::TreeTable(const char* name, uint32 flags, border_style borderStyle,
	bool showHorizontalScrollbar)
	:
	BColumnListView(name, flags, borderStyle, showHorizontalScrollbar),
	fModel(NULL)
{
}


TreeTable::TreeTable(TreeTableModel* model, const char* name, uint32 flags,
	border_style borderStyle, bool showHorizontalScrollbar)
	:
	BColumnListView(name, flags, borderStyle, showHorizontalScrollbar),
	fModel(NULL),
	fColumns(20, true)
{
	SetTreeTableModel(model);
}


TreeTable::~TreeTable()
{
	for (int32 i = CountColumns() - 1; i >= 0; i--)
		RemoveColumn(ColumnAt(i));
}


void
TreeTable::SetTreeTableModel(TreeTableModel* model)
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

	// recursively create the rows
	_AddChildRows(fModel->Root(), NULL, fModel->CountColumns());
}


void
TreeTable::_AddChildRows(void* parent, BRow* parentRow, int32 columnCount)
{
	int32 childCount = fModel->CountChildren(parent);
	for (int32 i = 0; i < childCount; i++) {
		void* child = fModel->ChildAt(parent, i);

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
			TreeTableField* field = new(std::nothrow) TreeTableField(child);
			if (field == NULL) {
				// TODO: Report error!
				return;
			}

			row->SetField(field, columnIndex);
		}

		// add row
		AddRow(row, parentRow);

		// recursively create children
		_AddChildRows(child, row, columnCount);
	}
}


void
TreeTable::AddColumn(TableColumn* column)
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
TreeTable::AddTreeTableListener(TreeTableListener* listener)
{
	return fListeners.AddItem(listener);
}


void
TreeTable::RemoveTreeTableListener(TreeTableListener* listener)
{
	fListeners.RemoveItem(listener);
}


void
TreeTable::ItemInvoked()
{
	if (fListeners.IsEmpty())
		return;

	BRow* row = CurrentSelection();
	if (row == NULL)
		return;

	TreeTableField* field = dynamic_cast<TreeTableField*>(row->GetField(0));
	if (field == NULL)
		return;

	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--)
		fListeners.ItemAt(i)->TreeTableNodeInvoked(this, field->Object());
}
