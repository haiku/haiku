// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003-2004, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        DevicesWindows.h
//  Author:      Sikosis, Jérôme Duval
//  Description: Devices Preferences
//  Created :    March 04, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~


#ifndef __DEVICESWINDOWS_H__
#define __DEVICESWINDOWS_H__

#include <ListView.h>
#include <Menu.h>
#include <PopUpMenu.h>
#include <Window.h>

#include "DevicesInfo.h"

#include "cm_wrapper.h"

#define MODEM_ADDED 'moad'

void CenterWindowOnScreen(BWindow* w);

class ResourceUsageWindow : public BWindow
{
	public:
    	ResourceUsageWindow(BRect frame, BList &);
	    ~ResourceUsageWindow();
	    virtual void MessageReceived(BMessage *message);
	    
	private:
		void InitWindow(BList &);
};


class ModemWindow : public BWindow
{
	public:
    	ModemWindow(BRect frame, BMessenger);
	    ~ModemWindow();
	    virtual void MessageReceived(BMessage *message);
	private:
		void InitWindow(void);
	    BMessenger fMessenger;
};

class ConfigurationWindow : public BWindow
{
	public:
		ConfigurationWindow(BRect frame, DeviceItem *item);
	    ~ConfigurationWindow();
    	virtual bool QuitRequested();
	    virtual void MessageReceived(BMessage *message);
	private:
		void InitWindow(void);
	    DeviceItem	*fItem;
	    BMenu	*fConfigurationMenu;
	    BPopUpMenu 	*fIRQMenu[3];
	    BMenuField 	*fIRQField[3];
	    BPopUpMenu	*fDMAMenu[3];
	    BMenuField 	*fDMAField[3];
	    BListView *ioListView, *memoryListView; 
};


class DevicesWindow : public BWindow
{
	public:
    	DevicesWindow(BRect frame);
	    ~DevicesWindow();
    	virtual bool QuitRequested();
	    virtual void MessageReceived(BMessage *message);
	private:
		void InitWindow(void);
		void InitDevices(bus_type bus);
		void LoadSettings(BMessage *msg);
		void SaveSettings(void);
		void UpdateDeviceInfo();
		
		ResourceUsageWindow*	ptrResourceUsageWindow;
		ModemWindow* 			ptrModemWindow;
		
        BStringView      *stvDeviceName;
        BStringView      *stvCurrentState;
        BMenuBar		 *menubar;        
        
	    BList			fList;
	    
	    BListItem        *systemMenu, *isaMenu, *pciMenu, *jumperedMenu;
	    BOutlineListView *outline;
	    BStringView *stvIRQ;
		BStringView *stvDMA;
		BStringView *stvIORanges;
		BStringView *stvMemoryRanges;
		BButton *btnConfigure;		
};

#endif


