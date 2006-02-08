/*
 * Copyright 2003-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */
#include <Application.h>
#include <Screen.h>
#include <StringView.h>
#include "PasswordWindow.h"


void 
PasswordWindow::Setup() 
{
	BScreen theScreen(this);
	MoveTo((theScreen.Frame().IntegerWidth()-Bounds().IntegerWidth())/2,(theScreen.Frame().IntegerHeight()-Bounds().IntegerHeight())/2);
	fBgd=new BView (Bounds(),"fBgdView",0,0);
	fBgd->SetHighColor(ui_color(B_MENU_BACKGROUND_COLOR));
	fBgd->SetLowColor(ui_color(B_MENU_BACKGROUND_COLOR));
	fBgd->SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	AddChild(fBgd);
	BRect bounds(fBgd->Bounds());	
	bounds.InsetBy(5,5);
	
	fCustomBox = new BBox(bounds, "custBeBox", B_FOLLOW_NONE);
	BStringView *label = new BStringView(BRect(0,0,60,15), "labelView", "Unlock screen saver");
	fCustomBox->SetLabel(label);
	fBgd->AddChild(fCustomBox);

	fPassword = new BTextControl(BRect(10,28,260,47),"pwdCntrl","Enter password:", NULL, B_FOLLOW_NONE);
	fPassword->TextView()->HideTyping(true);
	fPassword->SetDivider(100);
	fCustomBox->AddChild(fPassword);
	fPassword->MakeFocus(true);

	fUnlock = new BButton(BRect(160,70,255,85), "fUnlock", "Unlock", new BMessage(UNLOCK_MESSAGE), B_FOLLOW_NONE);
	fUnlock->SetTarget(NULL, be_app);
	fCustomBox->AddChild(fUnlock);
	fUnlock->MakeDefault(true);
}

