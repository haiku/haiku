/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTING_H
#define SETTING_H


#include <String.h>

#include <ObjectList.h>
#include <Referenceable.h>
#include <Variant.h>


enum setting_type {
	SETTING_TYPE_BOOL,
	SETTING_TYPE_FLOAT,
	SETTING_TYPE_OPTIONS,
	SETTING_TYPE_RANGE,
	SETTING_TYPE_RECT
};


class Setting : public BReferenceable {
public:
	virtual						~Setting();

	virtual	setting_type		Type() const = 0;
	virtual	const char*			ID() const = 0;
	virtual	const char*			Name() const = 0;

	virtual	BVariant			DefaultValue() const = 0;
};


class BoolSetting : public virtual Setting {
public:
	virtual	setting_type		Type() const;

	virtual	BVariant			DefaultValue() const;

	virtual	bool				DefaultBoolValue() const = 0;
};


class FloatSetting : public virtual Setting {
public:
	virtual	setting_type		Type() const;

	virtual	BVariant			DefaultValue() const;

	virtual	float				DefaultFloatValue() const = 0;
};


class SettingsOption : public BReferenceable {
public:
	virtual						~SettingsOption();

	virtual	const char*			ID() const = 0;
	virtual	const char*			Name() const = 0;
};


class OptionsSetting : public virtual Setting {
public:
	virtual	setting_type		Type() const;

	virtual	BVariant			DefaultValue() const;

	virtual	int32				CountOptions() const = 0;
	virtual	SettingsOption*		OptionAt(int32 index) const = 0;
	virtual	SettingsOption*		OptionByID(const char* id) const = 0;

	virtual	SettingsOption*		DefaultOption() const = 0;
};


class RangeSetting : public virtual Setting {
public:
	virtual	setting_type		Type() const;

	virtual	BVariant			LowerBound() const = 0;
	virtual	BVariant			UpperBound() const = 0;
};


class RectSetting : public virtual Setting {
public:
	virtual	setting_type		Type() const;

	virtual	BVariant			DefaultValue() const;

	virtual	BRect				DefaultRectValue() const = 0;
};


class AbstractSetting : public virtual Setting {
public:
								AbstractSetting(const BString& id,
									const BString& name);

	virtual	const char*			ID() const;
	virtual	const char*			Name() const;

private:
			BString				fID;
			BString				fName;
};


class BoolSettingImpl : public AbstractSetting, public BoolSetting {
public:
								BoolSettingImpl(const BString& id,
									const BString& name, bool defaultValue);

	virtual	bool				DefaultBoolValue() const;

private:
			bool				fDefaultValue;
};


class FloatSettingImpl : public AbstractSetting, public FloatSetting {
public:
								FloatSettingImpl(const BString& id,
									const BString& name, float defaultValue);

	virtual	float				DefaultFloatValue() const;

private:
			float				fDefaultValue;
};


class OptionsSettingImpl : public AbstractSetting, public OptionsSetting {
public:
								OptionsSettingImpl(const BString& id,
									const BString& name);
	virtual						~OptionsSettingImpl();

	virtual	SettingsOption*		DefaultOption() const;

	virtual	int32				CountOptions() const;
	virtual	SettingsOption*		OptionAt(int32 index) const;
	virtual	SettingsOption*		OptionByID(const char* id) const;

			bool				AddOption(SettingsOption* option);
			bool				AddOption(const BString& id,
									const BString& name);

			void				SetDefaultOption(SettingsOption* option);

private:
			class Option;

			typedef BObjectList<SettingsOption> OptionList;

private:
			OptionList			fOptions;
			SettingsOption*		fDefaultOption;
};


class RangeSettingImpl : public AbstractSetting, public RangeSetting {
public:
								RangeSettingImpl(const BString& id,
									const BString& name,
									const BVariant& lowerBound,
									const BVariant& upperBound,
									const BVariant& defaultValue);

	virtual	BVariant			DefaultValue() const;

	virtual	BVariant			LowerBound() const;
	virtual	BVariant			UpperBound() const;

private:
			BVariant			fLowerBound;
			BVariant			fUpperBound;
			BVariant			fDefaultValue;
};


class RectSettingImpl : public AbstractSetting, public RectSetting {
public:
								RectSettingImpl(const BString& id,
									const BString& name,
									const BRect& defaultValue);

	virtual	BRect				DefaultRectValue() const;

private:
			BRect				fDefaultValue;
};


#endif	// SETTING_H
