#include "pwWindow.h"
#include "Screen.h"
#include <stdio.h>
#include "Application.h"

void pwWindow::setup(void) {
	BScreen theScreen(B_MAIN_SCREEN_ID);
	MoveTo((theScreen.Frame().IntegerWidth()-Bounds().IntegerWidth())/2,(theScreen.Frame().IntegerHeight()-Bounds().IntegerHeight())/2);
	bgd=new BView (Bounds(),"bgdView",0,0);
	bgd->SetHighColor(ui_color(B_MENU_BACKGROUND_COLOR));
	bgd->SetLowColor(ui_color(B_MENU_BACKGROUND_COLOR));
	bgd->SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	AddChild(bgd);
	BRect bounds(bgd->Bounds());	
	bounds.InsetBy(5,5);
	
	customBox=new BBox(bounds,"custBeBox",B_FOLLOW_NONE);
	customBox->SetLabel("Unlock screen saver");
	bgd->AddChild(customBox);

	password=new BTextControl(BRect(10,28,255,47),"pwdCntrl","Enter password:",NULL,B_FOLLOW_NONE);
	password->SetDivider(100);
	customBox->AddChild(password);

	unlock=new BButton(BRect(160,70,255,85),"unlock","Unlock",new BMessage ('DONE'),B_FOLLOW_NONE);
	unlock->SetTarget(NULL,be_app);
	customBox->AddChild(unlock);
	unlock->MakeDefault(true);
}

