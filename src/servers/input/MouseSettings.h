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

#include <SupportDefs.h>
#include <InterfaceDefs.h>


// ToDo: these should be defined somewhere else; the mouse
//		input driver or add-on must read them as well

typedef struct {
        bool    enabled;        // Acceleration on / off
        int32   accel_factor;   // accel factor: 256 = step by 1, 128 = step by 1/2
        int32   speed;          // speed accelerator (1=1X, 2 = 2x)...
} mouse_accel;

typedef struct {
        int32		    type;
        mouse_map       map;
        mouse_accel     accel;
        bigtime_t       click_speed;
} mouse_settings;


class MouseSettings {
	public:
		MouseSettings();
		~MouseSettings();

		void Defaults();
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

	private:
		static status_t GetSettingsPath(BPath &path);
		void RetrieveSettings();
		status_t SaveSettings();

		mouse_settings	fSettings, fOriginalSettings;
		mode_mouse		fMode, fOriginalMode;
};

#endif
