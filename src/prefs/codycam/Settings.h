#ifndef __SETTINGS__
#define __SETTINGS__

#include "SettingsHandler.h"

void SetUpSettings(char *filename);
void QuitSettings();

class StringValueSetting : public SettingsArgvDispatcher {
	// simple string setting
public:
	StringValueSetting(const char *name, const char *defaultValue, 
		const char *valueExpectedErrorString,
		const char *wrongValueErrorString);

	virtual ~StringValueSetting();

	void ValueChanged(const char *newValue);
	const char *Value() const;
	virtual const char *Handle(const char *const *argv);	

protected:
	virtual void SaveSettingValue(Settings *);
	virtual bool NeedsSaving() const;

	const char *defaultValue;
	const char *valueExpectedErrorString;
	const char *wrongValueErrorString;
	char *value;
};

class EnumeratedStringValueSetting : public StringValueSetting {
	// string setting, values that do not match string enumeration
	// are rejected
public:
	EnumeratedStringValueSetting(const char *name, const char *defaultValue, 
		const char *const *values, const char *valueExpectedErrorString,
		const char *wrongValueErrorString);

	void ValueChanged(const char *newValue);
	virtual const char *Handle(const char *const *argv);	

protected:
	const char *const *values;
	char *value;
};

class ScalarValueSetting : public SettingsArgvDispatcher {
	// simple int32 setting
public:
	ScalarValueSetting(const char *name, int32 defaultValue,
		const char *valueExpectedErrorString, const char *wrongValueErrorString,
		int32 min = LONG_MIN, int32 max = LONG_MAX);

	void ValueChanged(int32 newValue);
	int32 Value() const;
	void GetValueAsString(char *) const;
	virtual const char *Handle(const char *const *argv);	

protected:
	virtual void SaveSettingValue(Settings *);
	virtual bool NeedsSaving() const;

	int32 defaultValue;
	int32 value;
	int32 max;
	int32 min;
	const char *valueExpectedErrorString;
	const char *wrongValueErrorString;
};

class BooleanValueSetting : public ScalarValueSetting {
	// on-off setting
public:
	BooleanValueSetting(const char *name, bool defaultValue);

	bool Value() const;
	virtual const char *Handle(const char *const *argv);	

protected:
	virtual void SaveSettingValue(Settings *);
};

#endif
