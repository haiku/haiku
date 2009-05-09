/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "table/AbstractTable.h"

#include <new>

#include "table/TableColumn.h"


// #pragma mark - AbstractTableModel


AbstractTableModel::~AbstractTableModel()
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
}
