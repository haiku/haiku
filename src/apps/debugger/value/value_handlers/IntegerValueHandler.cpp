/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "IntegerValueHandler.h"

#include <new>

#include <AutoDeleter.h>

#include "IntegerValue.h"
#include "Setting.h"
#include "Settings.h"
#include "SettingsDescription.h"
#include "SettingsMenu.h"
#include "TableCellIntegerRenderer.h"


static const char* const kFormatSettingID = "format";


// #pragma mark - FormatOption


class IntegerValueHandler::FormatOption : public SettingsOption {
public:
	FormatOption(const char* id, const char* name, integer_format format)
		:
		fID(id),
		fName(name),
		fFormat(format)
	{
	}

	virtual const char* ID() const
	{
		return fID;
	}

	virtual const char* Name() const
	{
		return fName;
	}

	integer_format Format() const
	{
		return fFormat;
	}

private:
	const char*		fID;
	const char*		fName;
	integer_format	fFormat;
};


// #pragma mark - TableCellRendererConfig


class IntegerValueHandler::TableCellRendererConfig
	: public TableCellIntegerRenderer::Config {
public:
	TableCellRendererConfig()
		:
		fSettings(NULL),
		fFormatSetting(NULL)
	{
	}

	~TableCellRendererConfig()
	{
		if (fSettings != NULL)
			fSettings->ReleaseReference();
	}

	status_t Init(SettingsDescription* settingsDescription)
	{
		fSettings = new(std::nothrow) Settings(settingsDescription);
		if (fSettings == NULL)
			return B_NO_MEMORY;

		status_t error = fSettings->Init();
		if (error != B_OK)
			return error;

		fFormatSetting = dynamic_cast<OptionsSetting*>(
			settingsDescription->SettingByID(kFormatSettingID));
		if (fFormatSetting == NULL)
			return B_BAD_VALUE;

		return B_OK;
	}

	virtual Settings* GetSettings() const
	{
		return fSettings;
	}

	virtual integer_format IntegerFormat() const
	{
		FormatOption* option = dynamic_cast<FormatOption*>(
			fSettings->OptionValue(fFormatSetting));
		return option != NULL ? option->Format() : INTEGER_FORMAT_DEFAULT;
	}

private:
	Settings*		fSettings;
	OptionsSetting*	fFormatSetting;
};


// #pragma mark - IntegerValueHandler


IntegerValueHandler::IntegerValueHandler()
{
}


IntegerValueHandler::~IntegerValueHandler()
{
}


status_t
IntegerValueHandler::Init()
{
	return B_OK;
}


float
IntegerValueHandler::SupportsValue(Value* value)
{
	return dynamic_cast<IntegerValue*>(value) != NULL ? 0.5f : 0;
}


status_t
IntegerValueHandler::GetValueFormatter(Value* value,
	ValueFormatter*& _formatter)
{
	// TODO:...
	return B_UNSUPPORTED;
}


status_t
IntegerValueHandler::GetTableCellValueRenderer(Value* _value,
	TableCellValueRenderer*& _renderer)
{
	IntegerValue* value = dynamic_cast<IntegerValue*>(_value);
	if (value == NULL)
		return B_BAD_VALUE;

	// create a settings description
	SettingsDescription* settingsDescription
		= _CreateTableCellSettingsDescription(value);
	if (settingsDescription == NULL)
		return B_NO_MEMORY;
	BReference<SettingsDescription> settingsDescriptionReference(
		settingsDescription, true);

	// create config
	TableCellRendererConfig* config = new(std::nothrow) TableCellRendererConfig;
	if (config == NULL)
		return B_NO_MEMORY;
	BReference<TableCellRendererConfig> configReference(config, true);

	status_t error = config->Init(settingsDescription);
	if (error != B_OK)
		return error;

	// create the renderer
	return CreateTableCellValueRenderer(value, config, _renderer);
}


status_t
IntegerValueHandler::CreateTableCellValueSettingsMenu(Value* value,
	Settings* settings, SettingsMenu*& _menu)
{
	// get the format option
	OptionsSetting* formatSetting = dynamic_cast<OptionsSetting*>(
		settings->Description()->SettingByID(kFormatSettingID));
	if (formatSetting == NULL)
		return B_BAD_VALUE;

	// create the settings menu
	SettingsMenuImpl* menu = new(std::nothrow) SettingsMenuImpl(settings);
	if (menu == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<SettingsMenu> menuDeleter(menu);

	// add the format option menu item
	if (!menu->AddOptionsItem(formatSetting))
		return B_NO_MEMORY;

	_menu = menuDeleter.Detach();
	return B_OK;
}


integer_format
IntegerValueHandler::DefaultIntegerFormat(IntegerValue* value)
{
	return value->IsSigned() ? INTEGER_FORMAT_SIGNED : INTEGER_FORMAT_UNSIGNED;
}


status_t
IntegerValueHandler::AddIntegerFormatSettingOptions(IntegerValue* value,
	OptionsSettingImpl* setting)
{
	status_t error = AddIntegerFormatOption(setting, "signed", "Signed",
		INTEGER_FORMAT_SIGNED);
	if (error != B_OK)
		return error;

	error = AddIntegerFormatOption(setting, "unsigned", "Unsigned",
		INTEGER_FORMAT_UNSIGNED);
	if (error != B_OK)
		return error;

	error = AddIntegerFormatOption(setting, "hex", "Hexadecimal",
		INTEGER_FORMAT_HEX_DEFAULT);
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
IntegerValueHandler::CreateTableCellValueRenderer(IntegerValue* value,
	TableCellIntegerRenderer::Config* config,
	TableCellValueRenderer*& _renderer)
{
	TableCellValueRenderer* renderer
		= new(std::nothrow) TableCellIntegerRenderer(config);
	if (renderer == NULL)
		return B_NO_MEMORY;

	_renderer = renderer;
	return B_OK;
}


status_t
IntegerValueHandler::AddIntegerFormatOption(OptionsSettingImpl* setting,
	const char* id, const char* name, integer_format format)
{
	FormatOption* option = new(std::nothrow) FormatOption(id, name, format);
	if (option == NULL || !setting->AddOption(option)) {
		delete option;
		return B_NO_MEMORY;
	}

	return B_OK;
}


SettingsDescription*
IntegerValueHandler::_CreateTableCellSettingsDescription(
	IntegerValue* value)
{
	// create description object
	SettingsDescription* description = new(std::nothrow) SettingsDescription;
	if (description == NULL)
		return NULL;
	BReference<SettingsDescription> descriptionReference(description, true);

	// integer format setting
	OptionsSettingImpl* setting = new(std::nothrow) OptionsSettingImpl(
		kFormatSettingID, "Format");
	if (setting == NULL)
		return NULL;
	BReference<OptionsSettingImpl> settingReference(setting, true);

	// add options
	if (AddIntegerFormatSettingOptions(value, setting) != B_OK)
		return NULL;

	// set default
	integer_format defaultFormat = DefaultIntegerFormat(value);
	SettingsOption* defaultOption = setting->OptionAt(0);
	for (int32 i = 0;
		FormatOption* option
			= dynamic_cast<FormatOption*>(setting->OptionAt(i));
		i++) {
		if (option->Format() == defaultFormat) {
			defaultOption = option;
			break;
		}
	}

	setting->SetDefaultOption(defaultOption);

	// add setting
	if (!description->AddSetting(setting))
		return NULL;

	return descriptionReference.Detach();
}
