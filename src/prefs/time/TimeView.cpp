/*
	
	TimeView.cpp
	
*/
#include <InterfaceDefs.h>

#include "TimeView.h"
#include "TimeMessages.h"

TimeView::TimeView(BRect rect)
	   	   : BBox(rect, "time_view",
					B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
					B_PLAIN_BORDER)
{

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

}

void TimeView::Draw(BRect updateFrame)
{
	inherited::Draw(updateFrame);

}