/*

MediaListItem.cpp by Sikosis

(C)2003

*/

// Includes -------------------------------------------------------------------------------------------------- //
#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Font.h>
#include <ListView.h>
#include <math.h>
#include <OutlineListView.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <Shape.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <StringView.h>
#include <TextView.h>
#include <TextControl.h>
#include <Window.h>
#include <View.h>

//#include "MediaConstants.h"
#include "MediaListItem.h"

const char *media_names[] = {
	"Audio Settings","Audio Mixer","None In","None Out",
	"Video Settings","Video Window Consumer"
};


// MediaListItem - Constructor
MediaListItem::MediaListItem(const char *label, int32 MediaName, BBitmap *bitmap, BMessage *msg, uint32 modifiers = 0) : BListItem()
{
	kMediaName = MediaName;
	icon = bitmap;
}
//--------------------------------------------------------------------------------------------------------------//


//MediaListItem - DrawItem
void MediaListItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	rgb_color kHighlight = { 220,220,220,0 };
	rgb_color kBlackColor = { 0,0,0,0 };
	rgb_color kMedGray = { 140,140,140,0 };
	
	BRect r;
	
	r.top = frame.top;
	r.left = frame.left-10;
	r.right = frame.right;
	r.bottom = frame.bottom;
		
	if (IsSelected() || complete) 
	{
		rgb_color color;
		if (IsSelected())
		{
			color = kHighlight;
		} else {
			color = owner->ViewColor();
		}
		owner->SetHighColor(color);
		owner->FillRect(r);
	}
	
	owner->MovePenTo(frame.left-10,frame.bottom-13);
	owner->DrawBitmap(icon);
	
	owner->MovePenTo(frame.left+10, frame.bottom-2);
	if (IsEnabled())
	{
		owner->SetHighColor(kBlackColor);
	} else {
		owner->SetHighColor(kMedGray);
	}
	owner->DrawString(media_names[kMediaName]);
	
	owner->AttachedToWindow();
}
//--------------------------------------------------------------------------------------------------------------//
