/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
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
