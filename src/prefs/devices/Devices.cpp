// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003-2004, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        Devices.cpp
//  Author:      Sikosis, Jérôme Duval
//  Description: Devices Preferences
//  Created :    March 04, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~


// Includes -------------------------------------------------------------------------------------------------- //
#include <Application.h>

#include "DevicesWindows.h"

#define APP_SIGNATURE "application/x-vnd.OBOS.Devices"  // Application Signature and Title

class Devices : public BApplication
{
	public:
    	Devices();
	    virtual void MessageReceived(BMessage *message);
	    
	private:
		
};

// ---------------------------------------------------------------------------------------------------------- //

// Devices -- constructor 
Devices::Devices() : BApplication (APP_SIGNATURE)
{
	BRect DevicesWindowRect(0,0,396,400);
	new DevicesWindow(DevicesWindowRect);
}
// ---------------------------------------------------------------------------------------------------------- //


// Devices::MessageReceived -- handles incoming messages
void Devices::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
	    default:
    	    BApplication::MessageReceived(message); // pass it along ... 
        	break;
    }
}
// ---------------------------------------------------------------------------------------------------------- //


// Devices Main
int main(void)
{
   Devices theApp;
   theApp.Run();
   return 0;
}
// end ------------------------------------------------------------------------------------------------------ //

