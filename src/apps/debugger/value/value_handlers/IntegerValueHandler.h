/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef INTEGER_VALUE_HANDLER_H
#define INTEGER_VALUE_HANDLER_H


#include "IntegerFormatter.h"
#include "TableCellIntegerRenderer.h"
#include "ValueHandler.h"


class IntegerValue;
class OptionsSettingImpl;
class SettingsDescription;


class IntegerValueHandler : public ValueHandler {
public:
								IntegerValueHandler();
								~IntegerValueHandler();

			status_t			Init();

	virtual	float				SupportsValue(Value* value);
	virtual	status_t			GetValueFormatter(Value* value,
									ValueFormatter*& _formatter);
	virtual	status_t			GetTableCellValueRenderer(Value* value,
									TableCellValueRenderer*& _renderer);
	virtual	status_t			CreateTableCellValueSettingsMenu(Value* value,
									Settings* settings, SettingsMenu*& _menu);

protected:
	virtual	integer_format		DefaultIntegerFormat(IntegerValue* value);
	virtual	status_t			AddIntegerFormatSettingOptions(
									IntegerValue* value,
									OptionsSettingImpl* setting);
	virtual	status_t			CreateTableCellValueRenderer(
									IntegerValue* value,
									TableCellIntegerRenderer::Config* config,
									TableCellValueRenderer*& _renderer);

			status_t			AddIntegerFormatOption(
									OptionsSettingImpl* setting, const char* id,
									const char* name, integer_format format);

private:
			class FormatOption;
			class TableCellRendererConfig;

private:
			SettingsDescription* _CreateTableCellSettingsDescription(
									IntegerValue* value);
};


#endif	// INTEGER_VALUE_HANDLER_H
