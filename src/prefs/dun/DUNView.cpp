/*

DUNView by Sikosis (beos@gravity24hr.com)

(C) 2002 OpenBeOS under MIT license

*/

#include "app/Application.h"
#include "interface/Window.h"
#include "interface/View.h"
#include <stdio.h>

#include "DUN.h"
#include "ModemWindow.h"
#include "DUNWindow.h"
#include "DUNView.h"

// DUNView -- the view so we can put objects like text boxes on it
DUNView::DUNView (BRect frame) : BView (frame, "DUNView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW ) {
	// Set the Background Color
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

ModemView::ModemView (BRect frame) : BView (frame, "ModemView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	// Set the Background Color
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// end
