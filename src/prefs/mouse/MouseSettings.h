// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:			MouseSettings.h
//  Authors:		Jérôme Duval,
//					Andrew McCall (mccall@digitalparadise.co.uk)
//					Axel Dörfler (axeld@pinc-software.de)
//  Description:	Mouse Preferences
//  Created:		December 10, 2003
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

		void Revert();
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

		void Mapping(uint32 &first, uint32 &second, uint32 &third) const;
		void SetMapping(uint32 first, uint32 second, uint32 third);

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

#endif
