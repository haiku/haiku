/*
	
	ChartView.cpp
	
	by Pierre Raynaud-Richard.
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "ChartView.h"
#include "ChartWindow.h"

/* Straightforward constructor */
ChartView::ChartView(BRect rect) :
BView(rect, "", B_FOLLOW_ALL, B_WILL_DRAW) {;}

/* The drawing function just draw the offscreen if it exists and is used */
void ChartView::Draw(BRect r)
{
	ChartWindow		*w;
	
	w = dynamic_cast<ChartWindow *>(Window());
	if ((w->offscreen != 0) && (w->set.display == DISPLAY_BITMAP))
		DrawBitmap(w->offscreen, r, r);
}

/* Send a message to the window if the user click anywhere in the animation
   view. This is used to go out of fullscreen demo mode. */
void ChartView::MouseDown(BPoint where)
{
	Window()->PostMessage(BACK_DEMO_MSG);
}

/* Another straightforward constructor. The default target setting for the
   frames/s vue-meter is 5 (as 5 * 12 = 60 frames/s) */
InstantView::InstantView(BRect rect) :
BView(rect, "", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	step = 5;
}

/* Draw the colored bars of the vue-meter depending the current framerate
   of the window animation. The color coding depends of the target framerate
   as encoded by step. */
void InstantView::Draw(BRect r)
{
	int32			i;
	ChartWindow		*w;
	
	w = dynamic_cast<ChartWindow *>(Window());
	for (i=0; i< w->instant_load_level; i++) {
		if (i<step) SetHighColor(255.0, 90.0, 90.0);
		else if ((i/step) & 1) SetHighColor(90.0, 255.0, 90.0);
		else SetHighColor(40.0, 200.0, 40.0);
		FillRect(BRect(3+i*4, 2, 5+i*4, 19));
	}
	Flush();
}

/* Straightforward constructor */
ChartColorControl::ChartColorControl(BPoint start, BMessage *message) :
BColorControl(start, B_CELLS_32x8, 8.0, "", message)
{
}

/* We overwrite SetValue to send a message to the target everytime
   the setting change and not only at the end. */
void ChartColorControl::SetValue(int32 color_value)
{
	BLooper		*looper;

	BColorControl::SetValue(color_value);
	Target(&looper);
	if (looper) {
		BMessage msg(*Message());
		msg.AddInt32("be:value", color_value);
		looper->PostMessage(&msg);
	}
}					
