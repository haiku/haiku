/*

Devices Windows Header by Sikosis

(C)2003 OBOS

*/

#ifndef __DEVICESWINDOWS_H__
#define __DEVICESWINDOWS_H__

#include "Devices.h"
#include "DevicesViews.h"

class DevicesView;
class ResourceUsageView;
class IRQView;
class ModemView;


class ResourceUsageWindow : public BWindow
{
	public:
    	ResourceUsageWindow(BRect frame);
	    ~ResourceUsageWindow();
	    virtual void MessageReceived(BMessage *message);
	private:
		void InitWindow(void);
	    ResourceUsageView*	 ptrResourceUsageView;
	    IRQView*             ptrIRQView;
	    BTabView		     *tabView;
	    BTab			     *tab;
};


class ModemWindow : public BWindow
{
	public:
    	ModemWindow(BRect frame);
	    ~ModemWindow();
	    virtual void MessageReceived(BMessage *message);
	private:
		void InitWindow(void);
	    ModemView*	 ptrModemView;
	    
	    BBox		 *boxInternalModem;
	    BButton      *btnAdd;
	    BButton		 *btnCancel;
};


class DevicesWindow : public BWindow
{
	public:
    	DevicesWindow(BRect frame);
	    ~DevicesWindow();
    	virtual bool QuitRequested();
	    virtual void MessageReceived(BMessage *message);
   	    virtual void FrameResized(float width, float height); 
	private:
		void InitWindow(void);
		void LoadSettings(BMessage *msg);
		void SaveSettings(void);
		ResourceUsageWindow*	ptrResourceUsageWindow;
		ModemWindow* 			ptrModemWindow;
		
        BStringView      *stvDeviceName;
        BStringView      *stvCurrentState;
        BMenuBar		 *menubar;        
        
	    DevicesView*	 ptrDevicesView;
};

#endif


