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


// ResourceUsageView - Constructor
ResourceUsageView::ResourceUsageView (BRect frame) : BView (frame, "ResourceUsageView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// ---------------------------------------------------------------------------------------------------------- //


// IRQView - used in Resource Usage Window
IRQView::IRQView (BRect frame) : BView (frame, "IRQView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// ---------------------------------------------------------------------------------------------------------- //


// DMAView - used in Resource Usage Window
DMAView::DMAView (BRect frame) : BView (frame, "DMAView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// ---------------------------------------------------------------------------------------------------------- //

// IORangeView - used in Resource Usage Window
IORangeView::IORangeView (BRect frame) : BView (frame, "IORangeView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// ---------------------------------------------------------------------------------------------------------- //

// MemoryRangeView - used in Resource Usage Window
MemoryRangeView::MemoryRangeView (BRect frame) : BView (frame, "MemoryRangeView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// ---------------------------------------------------------------------------------------------------------- //

// ModemView - Constructor
ModemView::ModemView (BRect frame) : BView (frame, "ModemView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// ---------------------------------------------------------------------------------------------------------- //
