/*

Devices Windows Header by Sikosis

(C)2003 OBOS

*/

#ifndef __DEVICESWINDOWS_H__
#define __DEVICESWINDOWS_H__

#include "Devices.h"
#include "DevicesViews.h"

class DevicesView;

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
		
        BStringView      *stvDeviceName;
        BStringView      *stvCurrentState;
        BMenuBar		 *menubar;        
        
	    DevicesView*	 ptrDevicesView;
};

#endif
