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

class ResourceUsageWindow : public BWindow
{
	public:
    	ResourceUsageWindow(BRect frame);
	    ~ResourceUsageWindow();
	    virtual void MessageReceived(BMessage *message);
	private:
		void InitWindow(void);
	    ResourceUsageView*	 ptrResourceUsageView;
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
		ResourceUsageWindow* ptrResourceUsageWindow; 
		
        BStringView      *stvDeviceName;
        BStringView      *stvCurrentState;
        BMenuBar		 *menubar;        
        
	    DevicesView*	 ptrDevicesView;
};

#endif


