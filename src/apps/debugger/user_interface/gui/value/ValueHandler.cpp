/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ValueHandler.h"


ValueHandler::~ValueHandler()
{
}


status_t
ValueHandler::CreateTableCellValueSettingsMenu(Value* value, Settings* settings,
	SettingsMenu*& _menu)
{
	_menu = NULL;
	return B_OK;
}


status_t
ValueHandler::GetTableCellValueEditor(Value* value, Settings* settings,
	TableCellValueEditor*& _editor)
{
	_editor = NULL;
	return B_OK;
}
