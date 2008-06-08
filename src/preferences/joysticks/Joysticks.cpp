/*
 * Copyright 2007 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *		Ryan Leavengood, leavengood@gmail.com
 */


#include "Joysticks.h"
#include "JoyWin.h"

#include <stdio.h>
#include <stdlib.h>

#include <Window.h>
#include <View.h>
#include <Button.h>
#include <Box.h>
#include <StringView.h>

int main(void)
{
	Joysticks application("application/x-vnd.Haiku-Joysticks");
	application.Run();
	return 0;
}


Joysticks::Joysticks(const char *signature) 
	: BApplication(signature)
{
}


Joysticks::~Joysticks()
{
	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
}


bool
Joysticks::QuitRequested()
{
	return BApplication::QuitRequested();
}


void
Joysticks::ReadyToRun()
{
	fJoywin = new JoyWin(BRect(100, 100, 500, 400), "Joysticks");	
	if (fJoywin != NULL)
		fJoywin->Show();
	else
		be_app->PostMessage(B_QUIT_REQUESTED);
}

