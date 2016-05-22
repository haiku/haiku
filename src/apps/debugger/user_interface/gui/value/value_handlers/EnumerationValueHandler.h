/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ENUMERATION_VALUE_HANDLER_H
#define ENUMERATION_VALUE_HANDLER_H


#include "IntegerValueHandler.h"


class EnumerationValueHandler : public IntegerValueHandler {
public:
								EnumerationValueHandler();
								~EnumerationValueHandler();

			status_t			Init();

	virtual	float				SupportsValue(Value* value);
	virtual	status_t			GetValueFormatter(Value* value,
									ValueFormatter*& _formatter);
	virtual	status_t			GetTableCellValueEditor(Value* value,
									Settings* settings,
									TableCellValueEditor*& _editor);

protected:
	virtual	integer_format		DefaultIntegerFormat(IntegerValue* value);
	virtual	status_t			CreateValueFormatter(
									IntegerValueFormatter::Config* config,
									ValueFormatter*& _formatter);
	virtual	status_t			AddIntegerFormatSettingOptions(
									IntegerValue* value,
									OptionsSettingImpl* setting);
	virtual	status_t			CreateTableCellValueRenderer(
									IntegerValue* value,
									IntegerValueFormatter::Config* config,
									TableCellValueRenderer*& _renderer);
};


#endif	// ENUMERATION_VALUE_HANDLER_H
