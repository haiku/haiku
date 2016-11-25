/*
 * Copyright 2013-2016, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Setting.h"

#include <new>


// #pragma mark - Setting


Setting::~Setting()
{
}


// #pragma mark - BoolSetting


setting_type
BoolSetting::Type() const
{
	return SETTING_TYPE_BOOL;
}


BVariant
BoolSetting::DefaultValue() const
{
	return DefaultBoolValue();
}


// #pragma mark - FloatSetting


setting_type
FloatSetting::Type() const
{
	return SETTING_TYPE_FLOAT;
}


BVariant
FloatSetting::DefaultValue() const
{
	return DefaultFloatValue();
}


// #pragma mark - SettingsOption


SettingsOption::~SettingsOption()
{
}


// #pragma mark - OptionsSetting


setting_type
OptionsSetting::Type() const
{
	return SETTING_TYPE_OPTIONS;
}


BVariant
OptionsSetting::DefaultValue() const
{
	SettingsOption* option = DefaultOption();
	return option != NULL
		? BVariant(option->ID(), B_VARIANT_DONT_COPY_DATA) : BVariant();
}


// #pragma mark - BoundedSetting


setting_type
BoundedSetting::Type() const
{
	return SETTING_TYPE_BOUNDED;
}


// #pragma mark - RangeSetting


setting_type
RangeSetting::Type() const
{
	return SETTING_TYPE_RANGE;
}


// #pragma mark - RectSetting

setting_type
RectSetting::Type() const
{
	return SETTING_TYPE_RECT;
}


BVariant
RectSetting::DefaultValue() const
{
	return DefaultRectValue();
}


// #pragma mark - StringSetting


setting_type
StringSetting::Type() const
{
	return SETTING_TYPE_STRING;
}


BVariant
StringSetting::DefaultValue() const
{
	return DefaultStringValue().String();
}


// #pragma mark - AbstractSetting


AbstractSetting::AbstractSetting(const BString& id, const BString& name)
	:
	fID(id),
	fName(name)
{
}


const char*
AbstractSetting::ID() const
{
	return fID;
}


const char*
AbstractSetting::Name() const
{
	return fName;
}


// #pragma mark - BoolSettingImpl


BoolSettingImpl::BoolSettingImpl(const BString& id, const BString& name,
	bool defaultValue)
	:
	AbstractSetting(id, name),
	fDefaultValue(defaultValue)
{
}


bool
BoolSettingImpl::DefaultBoolValue() const
{
	return fDefaultValue;
}


// #pragma mark - FloatSettingImpl


FloatSettingImpl::FloatSettingImpl(const BString& id, const BString& name,
	float defaultValue)
	:
	AbstractSetting(id, name),
	fDefaultValue(defaultValue)
{
}


float
FloatSettingImpl::DefaultFloatValue() const
{
	return fDefaultValue;
}


// #pragma mark - OptionsSettingImpl


class OptionsSettingImpl::Option : public SettingsOption {
public:
	Option(const BString& id, const BString& name)
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

private:
	BString	fID;
	BString	fName;
};


OptionsSettingImpl::OptionsSettingImpl(const BString& id, const BString& name)
	:
	AbstractSetting(id, name),
	fDefaultOption(NULL)
{
}


OptionsSettingImpl::~OptionsSettingImpl()
{
	SetDefaultOption(NULL);

	for (int32 i = 0; SettingsOption* option = fOptions.ItemAt(i); i++)
		option->ReleaseReference();
}


SettingsOption*
OptionsSettingImpl::DefaultOption() const
{
	return fDefaultOption != NULL ? fDefaultOption : fOptions.ItemAt(0);
}


int32
OptionsSettingImpl::CountOptions() const
{
	return fOptions.CountItems();
}


SettingsOption*
OptionsSettingImpl::OptionAt(int32 index) const
{
	return fOptions.ItemAt(index);
}


SettingsOption*
OptionsSettingImpl::OptionByID(const char* id) const
{
	for (int32 i = 0; SettingsOption* option = fOptions.ItemAt(i); i++) {
		if (strcmp(option->ID(), id) == 0)
			return option;
	}

	return NULL;
}


bool
OptionsSettingImpl::AddOption(SettingsOption* option)
{
	if (!fOptions.AddItem(option))
		return false;

	option->AcquireReference();
	return true;
}


bool
OptionsSettingImpl::AddOption(const BString& id, const BString& name)
{
	Option* option = new(std::nothrow) Option(id, name);
	if (option == NULL)
		return false;
	BReference<Option> optionReference(option, true);

	return AddOption(option);
}


void
OptionsSettingImpl::SetDefaultOption(SettingsOption* option)
{
	if (option == fDefaultOption)
		return;

	if (fDefaultOption != NULL)
		fDefaultOption->ReleaseReference();

	fDefaultOption = option;

	if (fDefaultOption != NULL)
		fDefaultOption->AcquireReference();
}


// #pragma mark - RangeSettingImpl


BoundedSettingImpl::BoundedSettingImpl(const BString& id, const BString& name,
	const BVariant& lowerBound, const BVariant& upperBound,
	const BVariant& defaultValue)
	:
	AbstractSetting(id, name),
	fLowerBound(lowerBound),
	fUpperBound(upperBound),
	fDefaultValue(defaultValue)
{
}


BVariant
BoundedSettingImpl::DefaultValue() const
{
	return fDefaultValue;
}


BVariant
BoundedSettingImpl::LowerBound() const
{
	return fLowerBound;
}


BVariant
BoundedSettingImpl::UpperBound() const
{
	return fUpperBound;
}


// #pragma mark - RangeSettingImpl


RangeSettingImpl::RangeSettingImpl(const BString& id, const BString& name,
	const BVariant& lowerBound, const BVariant& upperBound,
	const BVariant& lowerValue, const BVariant& upperValue)
	:
	AbstractSetting(id, name),
	fLowerBound(lowerBound),
	fUpperBound(upperBound),
	fLowerValue(lowerValue),
	fUpperValue(upperValue)
{
}


BVariant
RangeSettingImpl::DefaultValue() const
{
	// this one doesn't really make sense for RangeSetting since it
	// describes a pair of values, which BVariant can't readily
	// represent.
	return BVariant();
}


BVariant
RangeSettingImpl::LowerBound() const
{
	return fLowerBound;
}


BVariant
RangeSettingImpl::UpperBound() const
{
	return fUpperBound;
}


BVariant
RangeSettingImpl::LowerValue() const
{
	return fLowerValue;
}


BVariant
RangeSettingImpl::UpperValue() const
{
	return fUpperValue;
}


// #pragma mark - RectSettingImpl


RectSettingImpl::RectSettingImpl(const BString& id, const BString& name,
	const BRect& defaultValue)
	:
	AbstractSetting(id, name),
	fDefaultValue(defaultValue)
{
}


BRect
RectSettingImpl::DefaultRectValue() const
{
	return fDefaultValue;
}


// #pragma mark - StringSettingImpl


StringSettingImpl::StringSettingImpl(const BString& id, const BString& name,
	const BString& defaultValue)
	:
	AbstractSetting(id, name),
	fDefaultValue(defaultValue)
{
}


const BString&
StringSettingImpl::DefaultStringValue() const
{
	return fDefaultValue;
}
