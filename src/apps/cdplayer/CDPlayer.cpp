/*

CDPlayer

Author: Sikosis

(C)2004 Haiku - http://haiku-os.org/

*/

// Includes ------------------------------------------------------------------------------------------ //
#include <Alert.h>
#include <Application.h>
#include <Button.h>
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

#include "CDPlayer.h"
#include "CDPlayerWindows.h"
#include "CDPlayerViews.h"
#include "CDPlayerConstants.h"
// Constants ---------------------------------------------------------------------------------------- //

const char *APP_SIGNATURE = "application/x-vnd.Haiku.CDPlayer";  // Application Signature and Title

// -------------------------------------------------------------------------------------------------- //

CDPlayerWindow		*ptrCDPlayerWindow;

// CDPlayer - Constructor
CDPlayer::CDPlayer() : BApplication (APP_SIGNATURE)
{
	// Default Window Size - Position doesn't matter as we centre the form to the current screen size
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());

	float FormTopDefault = 0;
	float FormLeftDefault = 0;
	float FormWidthDefault = 500;
	float FormHeightDefault = 100;
	BRect CDPlayerWindowRect(FormTopDefault,FormLeftDefault,FormLeftDefault+FormWidthDefault,FormTopDefault+FormHeightDefault);

	ptrCDPlayerWindow = new CDPlayerWindow(CDPlayerWindowRect);
}
// ------------------------------------------------------------------------------------------------- //

// CDPlayer::MessageReceived -- handles incoming messages
void CDPlayer::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
		default:
			BApplication::MessageReceived(message); // pass it along ... 
			break;
	}
}
// ------------------------------------------------------------------------------------------------- //

// CDPlayer Main
int main(void)
{
	CDPlayer theApp;
	theApp.Run();
	return 0;
}
// end --------------------------------------------------------------------------------------------- //

