/*

DUNView - DialUp Networking BView

Authors: Sikosis (beos@gravity24hr.com)
		 Misza (misza@ihug.com.au) 

(C) 2002 OpenBeOS under MIT license

*/

#include <Application.h>
#include <stdio.h>

#include "DUN.h"
#include "ModemWindow.h"
#include "DUNWindow.h"
#include "DUNView.h"
#include "SettingsWindow.h"
#include "NewConnectionWindow.h"

// DUNView -- the view so we can put objects like text boxes on it
DUNView::DUNView (BRect frame) : BView (frame, "DUNView", B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW ) {
	// Set the Background Color
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
}

ModemView::ModemView (BRect frame) : BView (frame, "ModemView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	// Set the Background Color
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

SettingsView::SettingsView (BRect frame) : BView (frame, "SettingsView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	// Set the Background Color
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

NewConnectionWindowView::NewConnectionWindowView (BRect frame) : BView (frame, "NewConnectionWindowView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	// Set the Background Color
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


// end
