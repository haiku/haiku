// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:			MouseSettings.h
//  Authors:		Jérôme Duval,
//					Andrew McCall (mccall@digitalparadise.co.uk),
//					Axel Dörfler (axeld@pinc-software.de)
//  Description:	Input Server
//  Created:		August 29, 2004
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef MOUSE_SETTINGS_H_
#define MOUSE_SETTINGS_H_

#include <Archivable.h>
#include <InterfaceDefs.h>
#include <kb_mouse_settings.h>
#include <Path.h>
#include <String.h>
#include <SupportDefs.h>

#include <map>


class MouseSettings {
	public:
		MouseSettings();
		MouseSettings(mouse_settings* originalSettings);
		~MouseSettings();

		void Defaults();
		void Dump();

		int32 MouseType() const { return fSettings.type; }
		void SetMouseType(int32 type);

		bigtime_t ClickSpeed() const;
		void SetClickSpeed(bigtime_t click_speed);

		int32 MouseSpeed() const { return fSettings.accel.speed; }
		void SetMouseSpeed(int32 speed);

		int32 AccelerationFactor() const
			{ return fSettings.accel.accel_factor; }
		void SetAccelerationFactor(int32 factor);

		uint32 Mapping(int32 index) const;
		void Mapping(mouse_map &map) const;
		void SetMapping(int32 index, uint32 button);
		void SetMapping(mouse_map &map);

		mode_mouse MouseMode() const { return fMode; }
		void SetMouseMode(mode_mouse mode);

		mode_focus_follows_mouse FocusFollowsMouseMode() const
			{ return fFocusFollowsMouseMode; }
		void SetFocusFollowsMouseMode(mode_focus_follows_mouse mode);

		bool AcceptFirstClick() const { return fAcceptFirstClick; }
		void SetAcceptFirstClick(bool acceptFirstClick);

		void RetrieveSettings();
		status_t SaveSettings();

		const mouse_settings* GetSettings() { return &fSettings; }

	private:
		static status_t GetSettingsPath(BPath &path);

		mouse_settings	fSettings, fOriginalSettings;

		// FIXME all these extra settings are not specific to each mouse.
		// They should be moved into MultipleMouseSettings directly
		mode_mouse		fMode, fOriginalMode;
		mode_focus_follows_mouse	fFocusFollowsMouseMode;
		mode_focus_follows_mouse	fOriginalFocusFollowsMouseMode;
		bool			fAcceptFirstClick;
		bool			fOriginalAcceptFirstClick;
};


class MultipleMouseSettings: public BArchivable {
	public:
		MultipleMouseSettings();
		~MultipleMouseSettings();

		status_t Archive(BMessage* into, bool deep = false) const;

		void Defaults();
		void Dump();
		status_t SaveSettings();


		MouseSettings* AddMouseSettings(BString mouse_name);
		MouseSettings* GetMouseSettings(BString mouse_name);

	private:
		static status_t GetSettingsPath(BPath &path);
		void RetrieveSettings();

		MouseSettings*	fDeprecatedMouseSettings;

		typedef std::map<BString, MouseSettings*> mouse_settings_object;
		mouse_settings_object  fMouseSettingsObject;
};

#endif	// MOUSE_SETTINGS_H
