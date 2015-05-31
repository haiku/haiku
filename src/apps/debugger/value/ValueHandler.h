/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef VALUE_HANDLER_H
#define VALUE_HANDLER_H


#include <Referenceable.h>


class Settings;
class SettingsMenu;
class TableCellValueEditor;
class TableCellValueRenderer;
class Value;
class ValueFormatter;


class ValueHandler : public BReferenceable {
public:
	virtual						~ValueHandler();

	virtual	float				SupportsValue(Value* value) = 0;
	virtual	status_t			GetValueFormatter(Value* value,
									ValueFormatter*& _formatter) = 0;
									// returns a reference
	virtual	status_t			GetTableCellValueRenderer(Value* value,
									TableCellValueRenderer*& _renderer) = 0;
									// returns a reference
	virtual	status_t			GetTableCellValueEditor(Value* value,
									Settings* settings,
										// may be NULL if preconfiguration with
										// particular settings isn't desired.
									TableCellValueEditor*& _editor);
									// may return NULL, otherwise a reference
	virtual	status_t			CreateTableCellValueSettingsMenu(Value* value,
									Settings* settings, SettingsMenu*& _menu);
									// may return NULL, otherwise a reference
};


#endif	// VALUE_HANDLER_H
