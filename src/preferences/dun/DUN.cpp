/*

DUN - DialUp Networking Preference Application

Authors: Sikosis (beos@gravity24hr.com)
		 Misza (misza@ihug.com.au) 

(C) 2002 OpenBeOS under MIT license

*/

#include <Application.h>
#include <Alert.h>
#include <Window.h>
#include <View.h>
#include <stdio.h>

#include "DUN.h"
#include "ModemWindow.h"
#include "DUNWindow.h"
#include "DUNView.h"
// ------------------------------------------------------------------------------- //


// Application Signature and Title
const char *APP_SIGNATURE = "application/x-vnd.Haiku-DialUpNetworking";

// Default Window Size
float FormTopDefault = 100;
float FormLeftDefault = 100;
float FormWidthDefault = 312;
float FormHeightDefault = 250;

float FormTopState1 = 100;
float FormLeftState1 = 100;
float FormWidthState1 = 312;
float FormHeightState1 = 282;

float FormTopState2 = 100;
float FormLeftState2 = 100;
float FormWidthState2 = 312;
float FormHeightState2 = 282;

BRect windowRect(FormTopDefault,FormLeftDefault,FormLeftDefault+FormWidthDefault,FormTopDefault+FormHeightDefault);

// DUN -- constructor for DUN Class
DUN::DUN() : BApplication (APP_SIGNATURE)
{
    ptrDUNWindow = new DUNWindow(windowRect);
}
// ------------------------------------------------------------------------------- //


// DUN::MessageReceived -- handles incoming messages
void DUN::MessageReceived (BMessage *message)
{
	switch(message->what)
	{
    	default:
        	BApplication::MessageReceived(message); // pass it along ... 
         	break;
	}
}
// ------------------------------------------------------------------------------- //

// DUN Main
int main(void)
{
	DUN theApp;
	theApp.Run();
	return 0;
}
// end


