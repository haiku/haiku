/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "table/AbstractTable.h"

#include <new>

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
	fTableColumn(tableColumn)
{
}


AbstractTable::AbstractColumn::~AbstractColumn()
{
	delete fTableColumn;
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
