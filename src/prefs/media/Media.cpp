/*

Media by Sikosis

(C)2003

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
#include <TextControl.h>
#include <Window.h>
#include <View.h>

#include "Media.h"
#include "MediaWindows.h"
#include "MediaViews.h"
#include "MediaConstants.h"
// ---------------------------------------------------------------------------------------------------------- //

MediaWindow   *ptrMediaWindow;

// Media -- constructor 
Media::Media() : BApplication (APP_SIGNATURE)
{
	// Default Window Size - even though we centre the form to the current screen size
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	
	float FormTopDefault = 0;
	float FormLeftDefault = 0;
	float FormWidthDefault = 654;
	float FormHeightDefault = 392;
	
	BRect MediaWindowRect(FormTopDefault,FormLeftDefault,FormLeftDefault+FormWidthDefault,FormTopDefault+FormHeightDefault);

	ptrMediaWindow = new MediaWindow(MediaWindowRect);
}
// ---------------------------------------------------------------------------------------------------------- //


// Media::MessageReceived -- handles incoming messages
void Media::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
	    default:
    	    BApplication::MessageReceived(message); // pass it along ... 
        	break;
    }
}
// ---------------------------------------------------------------------------------------------------------- //

// Media Main
int main(void)
{
   Media theApp;
   theApp.Run();
   return 0;
}
// end ------------------------------------------------------------------------------------------------------ //
