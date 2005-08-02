/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <Screen.h>
#include "PasswordWindow.h"


void 
PasswordWindow::Setup() 
{
	BScreen theScreen(B_MAIN_SCREEN_ID);
	MoveTo((theScreen.Frame().IntegerWidth()-Bounds().IntegerWidth())/2,(theScreen.Frame().IntegerHeight()-Bounds().IntegerHeight())/2);
	fBgd=new BView (Bounds(),"fBgdView",0,0);
	fBgd->SetHighColor(ui_color(B_MENU_BACKGROUND_COLOR));
	fBgd->SetLowColor(ui_color(B_MENU_BACKGROUND_COLOR));
	fBgd->SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	AddChild(fBgd);
	BRect bounds(fBgd->Bounds());	
	bounds.InsetBy(5,5);
	
	fCustomBox = new BBox(bounds,"custBeBox",B_FOLLOW_NONE);
	fCustomBox->SetLabel("Unlock screen saver");
	fBgd->AddChild(fCustomBox);

	fPassword = new BTextControl(BRect(10,28,255,47),"pwdCntrl","Enter fPassword:",NULL,B_FOLLOW_NONE);
	fPassword->SetDivider(100);
	fCustomBox->AddChild(fPassword);

	fUnlock = new BButton(BRect(160,70,255,85), "fUnlock", "Unlock", new BMessage(UNLOCK_MESSAGE), B_FOLLOW_NONE);
	fUnlock->SetTarget(NULL,be_app);
	fCustomBox->AddChild(fUnlock);
	fUnlock->MakeDefault(true);
}

