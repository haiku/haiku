/*

Devices by Sikosis

(C)2003 OBOS

*/

// Includes -------------------------------------------------------------------------------------------------- //
#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Deskbar.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <ListView.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <stdio.h>
#include <string.h>
#include <Window.h>
#include <View.h>

#include "Devices.h"
#include "DevicesWindows.h"
#include "DevicesViews.h"
#include "DevicesConstants.h"
// ---------------------------------------------------------------------------------------------------------- //

DevicesWindow   *ptrDevicesWindow;

// Devices -- constructor 
Devices::Devices() : BApplication (APP_SIGNATURE)
{
	// Default Window Size - even though we centre the form to the current screen size
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	
	float FormTopDefault = 0;
	float FormLeftDefault = 0;
	float FormWidthDefault = 396;
	float FormHeightDefault = 400;
	
	BRect DevicesWindowRect(FormTopDefault,FormLeftDefault,FormLeftDefault+FormWidthDefault,FormTopDefault+FormHeightDefault);

	ptrDevicesWindow = new DevicesWindow(DevicesWindowRect);
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
