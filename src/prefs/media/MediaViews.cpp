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



// IconView - Constructor
IconView::IconView (BRect frame) : BView (frame, "IconView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	
}
// ---------------------------------------------------------------------------------------------------------- //


void IconView::Draw(BRect updateRect)
{
	BRect r;
	r = Bounds();
}
// ---------------------------------------------------------------------------------------------------------- //


// AvailableViewArea - Constructor
AvailableViewArea::AvailableViewArea (BRect frame) : BView (frame, "AvailableViewArea", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// ---------------------------------------------------------------------------------------------------------- //


void AvailableViewArea::Draw(BRect updateRect)
{
	BRect r;
	r = Bounds();
	
	// Display the 3D Look Divider Bar
	SetHighColor(140,140,140,0);
	StrokeLine(BPoint(0,27),BPoint(490,27));
	SetHighColor(255,255,255,0);
	StrokeLine(BPoint(1,28),BPoint(489,28));
}

// ---------------------------------------------------------------------------------------------------------- //


// AudioSettingsView - Constructor
AudioSettingsView::AudioSettingsView (BRect frame) : BView (frame, "AudioSettingsView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// ---------------------------------------------------------------------------------------------------------- //


void AudioSettingsView::Draw(BRect updateRect)
{
	BRect r;
	r = Bounds();
}
// ---------------------------------------------------------------------------------------------------------- //


// AudioMixerView - Constructor
AudioMixerView::AudioMixerView (BRect frame) : BView (frame, "AudioMixerView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW )
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}
// ---------------------------------------------------------------------------------------------------------- //


void AudioMixerView::Draw(BRect updateRect)
{
	BRect r;
	r = Bounds();
}
// ---------------------------------------------------------------------------------------------------------- //

