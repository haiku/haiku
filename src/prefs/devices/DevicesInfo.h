// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
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
		char * GetName() const { return fName; }
		char * GetBaseString() const { return fBaseString; }
		char * GetSubTypeString() const { return fSubTypeString; }
		struct device_configuration * GetCurrent() { return fCurrent;}
	private:
		struct device_info *fInfo;
		struct device_configuration *fCurrent; 
		struct possible_device_configurations *fPossible;
		char* fName, *fBaseString, *fSubTypeString;
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
