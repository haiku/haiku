/*

Devices Windows Header by Sikosis

(C)2003 OBOS

*/

#ifndef __DEVICESWINDOWS_H__
#define __DEVICESWINDOWS_H__

#include "Devices.h"
#include "cm_wrapper.h"

#define MODEM_ADDED 'moad'

class ResourceUsageWindow : public BWindow
{
	public:
    	ResourceUsageWindow(BRect frame, BList &);
	    ~ResourceUsageWindow();
	    virtual void MessageReceived(BMessage *message);
	    
	private:
		void InitWindow(BList &);
	    BTabView		     *tabView;
	    BTab			     *tab;
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


