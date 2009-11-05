/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_H
#define SETTINGS_H


#include <Locker.h>
#include <Message.h>

#include <ObjectList.h>
#include <Referenceable.h>
#include <Variant.h>

#include "Setting.h"


class SettingsDescription;


class Settings : public BReferenceable {
public:
	class Listener;

public:
								Settings(SettingsDescription* description);
	virtual						~Settings();

			status_t			Init();

			bool				Lock()		{ return fLock.Lock(); }
			void				Unlock()	{ fLock.Unlock(); }

			SettingsDescription* Description() const	{ return fDescription; }
			const BMessage&		Message() const			{ return fValues; }

			BVariant			Value(Setting* setting) const;
			BVariant			Value(const char* settingID) const;
			bool				SetValue(Setting* setting,
									const BVariant& value);

			bool				BoolValue(BoolSetting* setting) const
									{ return Value(setting).ToBool(); }
			SettingsOption*		OptionValue(OptionsSetting* setting) const;
			BVariant			RangeValue(RangeSetting* setting) const
									{ return Value(setting); }

			bool				AddListener(Listener* listener);
			void				RemoveListener(Listener* listener);

private:
			typedef BObjectList<Listener> ListenerList;

private:
	mutable	BLocker				fLock;
			SettingsDescription* fDescription;
			BMessage			fValues;
			ListenerList		fListeners;
};


class Settings::Listener {
public:
	virtual						~Listener();

	virtual	void				SettingValueChanged(Setting* setting) = 0;
};


#endif	// SETTINGS_H
