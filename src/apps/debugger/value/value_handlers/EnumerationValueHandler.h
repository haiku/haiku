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

protected:
	virtual	integer_format		DefaultIntegerFormat(IntegerValue* value);
	virtual	status_t			AddIntegerFormatSettingOptions(
									IntegerValue* value,
									OptionsSettingImpl* setting);
	virtual	status_t			CreateTableCellValueRenderer(
									IntegerValue* value,
									TableCellIntegerRenderer::Config* config,
									TableCellValueRenderer*& _renderer);
};


#endif	// ENUMERATION_VALUE_HANDLER_H
