// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        DevicesInfo.h
//  Author:      Jérôme Duval
//  Description: Devices Preferences
//  Created :    April 15, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef __DEVICESINFO_H__
#define __DEVICESINFO_H__

#include <drivers/config_manager.h>
#include <ListItem.h>
#include <Window.h>

class DevicesInfo
{
	public:
    	DevicesInfo(struct device_info *info, 
    		struct device_configuration *current, 
    		struct possible_device_configurations *possible);
	    ~DevicesInfo();
		struct device_info * GetInfo() { return fInfo;}
		char * GetDeviceName() const { return fDeviceName; }
		char * GetCardName() const { return fCardName; }
		char * GetBaseString() const { return fBaseString; }
		char * GetSubTypeString() const { return fSubTypeString; }
		struct device_configuration * GetCurrent() { return fCurrent;}
		char * GetISAName() const;
	private:
		struct device_info *fInfo;
		struct device_configuration *fCurrent; 
		struct possible_device_configurations *fPossible;
		char* fDeviceName, *fCardName, *fBaseString, *fSubTypeString;
};

class DeviceItem : public BListItem
{
	public:
		DeviceItem(DevicesInfo *info, const char* name);
		~DeviceItem();
		DevicesInfo *GetInfo() { return fInfo; }
		virtual void DrawItem(BView *, BRect, bool = false);
		BWindow *GetWindow() { return fWindow; };
		void SetWindow(BWindow *window) { fWindow = window; };
	private:
		DevicesInfo *fInfo;
		char* fName;
		BWindow *fWindow;
};

#endif
