// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MouseSettings.h
//  Author:      Jérôme Duval, Andrew McCall (mccall@digitalparadise.co.uk)
//  Description: Media Preferences
//  Created :   December 10, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef MOUSE_SETTINGS_H_
#define MOUSE_SETTINGS_H_

#include <SupportDefs.h>
#include <InterfaceDefs.h>

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

class MouseSettings{
public :
	MouseSettings();
	~MouseSettings();
	status_t	InitCheck();
	
	BPoint WindowCorner() const { return fCorner; }
	void SetWindowCorner(BPoint corner);
	int32 MouseType() const { return fSettings.type; }
	void SetMouseType(int32 type);
	bigtime_t ClickSpeed() const { return -(fSettings.click_speed-1000000); } // -1000000 to correct the Sliders 0-100000 scale
	void SetClickSpeed(bigtime_t click_speed);
	int32 MouseSpeed() const { return fSettings.accel.speed; }
	void SetMouseSpeed(int32 speed);
	
private:
	status_t			fInitCheck;
	BPoint				fCorner;
	mouse_settings		fSettings;
};

#endif
