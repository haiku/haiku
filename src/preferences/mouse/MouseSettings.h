/*
 * Copyright 2003-2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Andrew McCall (mccall@digitalparadise.co.uk)
 *		Axel Dörfler (axeld@pinc-software.de)
 */
#ifndef MOUSE_SETTINGS_H
#define MOUSE_SETTINGS_H


#include "kb_mouse_settings.h"

#include <SupportDefs.h>
#include <InterfaceDefs.h>

class BPath;


class MouseSettings {
	public:
		MouseSettings();
		~MouseSettings();

		void Revert();
		void Defaults();
		bool IsDefaultable();
		void Dump();

		BPoint WindowPosition() const { return fWindowPosition; }
		void SetWindowPosition(BPoint corner);

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

	private:
		static status_t GetSettingsPath(BPath &path);
		void RetrieveSettings();
		status_t SaveSettings();

		mouse_settings	fSettings, fOriginalSettings;
		mode_mouse		fMode, fOriginalMode;
		BPoint			fWindowPosition;
};

#endif	// MOUSE_SETTINGS_H
