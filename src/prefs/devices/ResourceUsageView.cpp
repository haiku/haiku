/*

ResourceUsageView

Author: Sikosis

(C)2003 OBOS - Released under the MIT License

*/

// Includes ------------------------------------------------------------------------------------------ //
#include <Alert.h>
#include <Application.h>
#include <Screen.h>
#include <stdio.h>
#include <Window.h>
#include <View.h>

#include "DevicesWindows.h"
#include "DevicesViews.h"
// -------------------------------------------------------------------------------------------------- //

// ResourceUsageView - Constructor
ResourceUsageView::ResourceUsageView (BRect frame) : BView (frame, "ResourceUsageView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// ------------------------------------------------------------------------------------------------- //
----------------------- //
