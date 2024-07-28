/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef MOUSE_SETTINGS_H
#define MOUSE_SETTINGS_H


#include <map>

#include <String.h>
#include <SupportDefs.h>

#include "kb_mouse_settings.h"


class BPath;

class MouseSettings {
public:
		MouseSettings(BString name);
		~MouseSettings();

		void Revert();
		bool IsRevertable() const;
		void Defaults();
		bool IsDefaultable() const;

		int32 MouseType() const { return fSettings.type; }
		void SetMouseType(int32 type);

		bigtime_t ClickSpeed() const;
		void SetClickSpeed(bigtime_t click_speed);

		int32 MouseSpeed() const { return fSettings.accel.speed; }
		void SetMouseSpeed(int32 speed);

		int32 AccelerationFactor() const { return fSettings.accel.accel_factor; }
		void SetAccelerationFactor(int32 factor);

		uint32 Mapping(int32 index) const;
		void Mapping(mouse_map &map) const;
		void SetMapping(int32 index, uint32 button);
		void SetMapping(mouse_map &map);

		mode_mouse MouseMode() const { return fMode; }
		void SetMouseMode(mode_mouse mode);

		mode_focus_follows_mouse FocusFollowsMouseMode() const {
			return fFocusFollowsMouseMode;
		}
		void SetFocusFollowsMouseMode(mode_focus_follows_mouse mode);

		bool AcceptFirstClick() const { return fAcceptFirstClick; }
		void SetAcceptFirstClick(bool accept_first_click);

		mouse_settings* GetSettings();

private:
		status_t _RetrieveSettings();

private:
		BString						fName;
		mode_mouse					fMode, fOriginalMode;
		mode_focus_follows_mouse	fFocusFollowsMouseMode;
		mode_focus_follows_mouse	fOriginalFocusFollowsMouseMode;
		bool						fAcceptFirstClick;
		bool						fOriginalAcceptFirstClick;

		mouse_settings	fSettings, fOriginalSettings;
};


class MultipleMouseSettings
{
	public:
		MultipleMouseSettings();
		~MultipleMouseSettings();

		MouseSettings* AddMouseSettings(BString mouse_name);

	private:
		typedef std::map<BString, MouseSettings*> mouse_settings_object;
		mouse_settings_object  fMouseSettingsObject;
};

#endif	// MOUSE_SETTINGS_H
