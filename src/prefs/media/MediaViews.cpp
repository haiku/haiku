/*

MediaViews by Sikosis

(C)2003

*/

// Includes -------------------------------------------------------------------------------------------------- //
#include <Alert.h>
#include <Application.h>
#include <Screen.h>
#include <stdio.h>
#include <Window.h>
#include <View.h>

#include "MediaWindows.h"
#include "MediaViews.h"
//#include "MediaConstants.h"
// ------------------------------------------------------------------------------------------------------------- //

// MediaView - Constructor
MediaView::MediaView (BRect frame) : BView (frame, "MediaView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// ---------------------------------------------------------------------------------------------------------- //


void MediaView::Draw(BRect updateRect)
{
	BRect r;
	r = Bounds();
}
// ---------------------------------------------------------------------------------------------------------- //

