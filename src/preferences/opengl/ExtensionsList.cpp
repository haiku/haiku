/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#include "ExtensionsList.h"

#include <ColumnTypes.h>
#include <String.h>


ExtensionRow::ExtensionRow(const char* extensionName)
	: BRow(),
	fExtensionName(extensionName)
{
	SetField(new BStringField(extensionName), 0);
}


ExtensionRow::~ExtensionRow()
{
	
}


ExtensionsList::ExtensionsList()
	: BColumnListView("ExtensionsList", B_FOLLOW_ALL)
{
	BStringColumn* column = new BStringColumn("Available", 280, 280, 280,
		B_TRUNCATE_MIDDLE);
	AddColumn(column, 0);
	SetSortingEnabled(true);
	SetSortColumn(column, true, true);
}


ExtensionsList::~ExtensionsList()
{
	BRow *row;
	while ((row = RowAt((int32)0, NULL)) != NULL) {
		RemoveRow(row);
		delete row;
	}
}
