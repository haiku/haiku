/*

DevicesViews by Sikosis

(C)2003 OBOS

*/

// Includes -------------------------------------------------------------------------------------------------- //
#include <Alert.h>
#include <Application.h>
#include <Screen.h>
#include <stdio.h>
#include <Window.h>
#include <View.h>

#include "DevicesWindows.h"
#include "DevicesViews.h"
//#include "DevicesConstants.h"
// ------------------------------------------------------------------------------------------------------------- //

// DevicesView - Constructor
DevicesView::DevicesView (BRect frame) : BView (frame, "DevicesView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// ---------------------------------------------------------------------------------------------------------- //


void DevicesView::Draw(BRect updateRect)
{
	BRect r;
	r = Bounds();
}
// ---------------------------------------------------------------------------------------------------------- //

