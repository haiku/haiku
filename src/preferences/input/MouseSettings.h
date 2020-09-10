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

#include <Archivable.h>
#include <Input.h>
#include <InterfaceDefs.h>
#include <Point.h>
#include <SupportDefs.h>
#include <String.h>

#include "kb_mouse_settings.h"


class BPath;

class MouseSettings {
public:
		MouseSettings(BString name);
		MouseSettings(mouse_settings settings, BString name);
		~MouseSettings();

		void Revert();
		bool IsRevertable();
		void Defaults();
		bool IsDefaultable();
		void Dump();

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
		status_t _RetrieveSettings();
		status_t _LoadLegacySettings();

		mouse_settings* GetSettings();

private:
		static status_t _GetSettingsPath(BPath &path);

private:
		BString						fName;
		mode_mouse					fMode, fOriginalMode;
		mode_focus_follows_mouse	fFocusFollowsMouseMode;
		mode_focus_follows_mouse	fOriginalFocusFollowsMouseMode;
		bool						fAcceptFirstClick;
		bool						fOriginalAcceptFirstClick;

		mouse_settings	fSettings, fOriginalSettings;
};


class MultipleMouseSettings: public BArchivable
{
	public:
		MultipleMouseSettings();
		~MultipleMouseSettings();

		status_t Archive(BMessage* into, bool deep = false) const;

		void Defaults();
		void Dump();
		status_t SaveSettings();

		/** Get or create settings for the given mouse */
		MouseSettings* AddMouseSettings(BString mouse_name);
		/** Get the existing settings, or return NULL */
		MouseSettings* GetMouseSettings(BString mouse_name);

	private:
		static status_t GetSettingsPath(BPath &path);
		void RetrieveSettings();

	private:
		MouseSettings*	fDeprecatedMouseSettings;

		typedef std::map<BString, MouseSettings*> mouse_settings_object;
		mouse_settings_object  fMouseSettingsObject;
};

#endif	// MOUSE_SETTINGS_H
