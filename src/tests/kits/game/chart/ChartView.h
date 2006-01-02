/*
	
	ChartView.h
	
	by Pierre Raynaud-Richard.
	
*/

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef CHART_VIEW_H
#define CHART_VIEW_H

#include <View.h>
#include <ColorControl.h>

/* This view used for the star animation area. It just need to know how
   to draw and handle mouse down event. */
class ChartView : public BView {
public:
					ChartView(BRect frame); 
virtual	void		Draw(BRect updateRect);
virtual	void		MouseDown(BPoint where);
};

/* This view is used to draw the instant load vue-meter */
class InstantView : public BView {
public:
	int32			step;

					InstantView(BRect frame);
virtual void		Draw(BRect updateRect);
};

/* This view is used to work around a bug in the current ColorControl,
   that doesn't allow live feedback when changing the color. */
class ChartColorControl : public BColorControl {
public:
						ChartColorControl(BPoint start, BMessage *message);
virtual	void			SetValue(int32 color_value);	
};

#endif
