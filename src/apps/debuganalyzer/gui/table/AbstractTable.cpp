/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "table/AbstractTable.h"

#include <new>

#include <Looper.h>
#include <Message.h>

#include "table/TableColumn.h"


// #pragma mark - AbstractTableModelBase


AbstractTableModelBase::~AbstractTableModelBase()
{
}


// #pragma mark - AbstractColumn


AbstractTable::AbstractColumn::AbstractColumn(TableColumn* tableColumn)
	:
	BColumn(tableColumn->Width(), tableColumn->MinWidth(),
		tableColumn->MaxWidth(), tableColumn->Alignment()),
	fTableColumn(tableColumn),
	fTable(NULL)
{
	SetWantsEvents(true);
}


AbstractTable::AbstractColumn::~AbstractColumn()
{
	delete fTableColumn;
}


void
AbstractTable::AbstractColumn::SetTable(AbstractTable* table)
{
	fTable = table;
}


void
AbstractTable::AbstractColumn::MouseDown(BColumnListView* parent, BRow* row,
	BField* field, BRect fieldRect, BPoint point, uint32 _buttons)
{
	if (fTable == NULL)
		return;

	if (fTable == NULL)
		return;
	BLooper* window = fTable->Looper();
	if (window == NULL)
		return;

	BMessage* message = window->CurrentMessage();
	if (message == NULL)
		return;

	int32 buttons;
		// Note: The _buttons parameter cannot be trusted.
	BPoint screenWhere;
	if (message->FindInt32("buttons", &buttons) != B_OK
		|| message->FindPoint("screen_where", &screenWhere) != B_OK) {
		return;
	}

	fTable->ColumnMouseDown(this, row, field, screenWhere, buttons);
}


void
AbstractTable::AbstractColumn::MouseUp(BColumnListView* parent, BRow* row,
	BField* field)
{
	if (fTable == NULL)
		return;
	BLooper* window = fTable->Looper();
	if (window == NULL)
		return;

	BMessage* message = window->CurrentMessage();
	if (message == NULL)
		return;

	int32 buttons;
	BPoint screenWhere;
	if (message->FindInt32("buttons", &buttons) != B_OK
		|| message->FindPoint("screen_where", &screenWhere) != B_OK) {
		return;
	}

	fTable->ColumnMouseUp(this, row, field, screenWhere, buttons);
}


// #pragma mark - AbstractTable


AbstractTable::AbstractTable(const char* name, uint32 flags,
	border_style borderStyle, bool showHorizontalScrollbar)
	:
	BColumnListView(name, flags, borderStyle, showHorizontalScrollbar),
	fColumns(20, true)
{
}


AbstractTable::~AbstractTable()
{
}


void
AbstractTable::AddColumn(TableColumn* column)
{
	if (column == NULL)
		return;

	AbstractColumn* privateColumn = CreateColumn(column);
	privateColumn->SetTable(this);

	if (!fColumns.AddItem(privateColumn)) {
		delete privateColumn;
		throw std::bad_alloc();
	}

	BColumnListView::AddColumn(privateColumn, column->ModelIndex());

	// TODO: The derived classes need to be notified, so that they can create
	// fields for the existing rows.
}


void
AbstractTable::ResizeColumnToPreferred(int32 index)
{
	BColumnListView::ResizeColumnToPreferred(index);
}


void
AbstractTable::ResizeAllColumnsToPreferred()
{
	BColumnListView::ResizeAllColumnsToPreferred();
}


list_view_type
AbstractTable::SelectionMode() const
{
	return BColumnListView::SelectionMode();
}


void
AbstractTable::SetSelectionMode(list_view_type type)
{
	BColumnListView::SetSelectionMode(type);
}


void
AbstractTable::DeselectAll()
{
	BColumnListView::DeselectAll();
}


void
AbstractTable::SetSortingEnabled(bool enabled)
{
	BColumnListView::SetSortingEnabled(enabled);
}


bool
AbstractTable::SortingEnabled() const
{
	return BColumnListView::SortingEnabled();
}


void
AbstractTable::SetSortColumn(TableColumn* column, bool add, bool ascending)
{
	if (AbstractColumn* privateColumn = GetColumn(column))
		BColumnListView::SetSortColumn(privateColumn, add, ascending);
}


void
AbstractTable::ClearSortColumns()
{
	BColumnListView::ClearSortColumns();
}


AbstractTable::AbstractColumn*
AbstractTable::GetColumn(TableColumn* column) const
{
	for (int32 i = 0; AbstractColumn* privateColumn = fColumns.ItemAt(i); i++) {
		if (privateColumn->GetTableColumn() == column)
			return privateColumn;
	}

	return NULL;
}
