/*

CDPlayer View

Author: Sikosis

(C)2004 Haiku - http://haiku-os.org/

*/

// Includes ------------------------------------------------------------------------------------------ //
#include <Alert.h>
#include <Application.h>
#include <Screen.h>
#include <stdio.h>
#include <Window.h>
#include <View.h>

#include "CDPlayerWindows.h"
#include "CDPlayerViews.h"
// -------------------------------------------------------------------------------------------------- //

// CDPlayerView - Constructor
CDPlayerView::CDPlayerView (BRect frame) : BView (frame, "CDPlayerView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// ------------------------------------------------------------------------------------------------- //

void CDPlayerView::Draw(BRect /*updateRect*/)
{
	BRect r;
	r = Bounds();
}
// ------------------------------------------------------------------------------------------------- //
