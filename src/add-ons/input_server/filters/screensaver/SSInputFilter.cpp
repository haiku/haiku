#include "SSInputFilter.h"
#include "Messenger.h"
#include "OS.h"

extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

SSISFilter::SSISFilter()
{
	first=real_time_clock();
	enabled=false;
	ssApp=NULL;
}

BInputServerFilter* instantiate_input_filter() 
{ 
	return (new SSISFilter()); 
}

void SSISFilter::UpdateRectangles(void)
{
	BRegion region;
	BRect frame;
	GetScreenRegion(&region);
	frame=region.Frame();

	topLeft.Set(frame.left,frame.top,frame.left+CORNER_SIZE,frame.top+CORNER_SIZE);
	topRight.Set(frame.right-CORNER_SIZE,frame.top,frame.right,frame.top+CORNER_SIZE);
	bottomLeft.Set(frame.left,frame.bottom-CORNER_SIZE,frame.left+CORNER_SIZE,frame.bottom);
	bottomRight.Set(frame.right-CORNER_SIZE,frame.bottom-CORNER_SIZE,frame.right,frame.bottom);
}

void SSISFilter::Cornered(cornerPos pos)
{
	static cornerPos oldPos;
	if (oldPos!=pos)
		enabled=(B_OK == ssApp->SendMessage((int32)(pos))); 
	oldPos=pos;
}

filter_result SSISFilter::Filter(BMessage *msg,BList *outList)
{
	status_t err;
	if (msg->what==B_SCREEN_CHANGED)
		UpdateRectangles();
	else if (enabled && msg->what==B_MOUSE_MOVED)
		{
		BPoint pos;
		msg->FindPoint("where",&pos);
		if (topLeft.Contains(pos)) 
			Cornered(TOPL);
		else if (topRight.Contains(pos)) 
			Cornered(TOPR);
		else if (bottomLeft.Contains(pos)) 
			Cornered(BOTL);
		else if (bottomRight.Contains(pos)) 
			Cornered(BOTR);
		else
			Cornered(MIDDLE);
		}
	if (real_time_clock()>first+4) // Only check every 5 seconds or so
		{
		if (!enabled)
			{
			delete ssApp;
			ssApp=new BMessenger("application/x-vnd.OBOS-ScreenSaverApp",-1,&err);
			if (err==B_OK)
				{
				enabled=true;
				UpdateRectangles();
				}
			}
		
		if (enabled)
			enabled=(B_OK == ssApp->SendMessage('INPT')); 
		first=real_time_clock();
		}
}

SSISFilter::~SSISFilter()
{
	;
}
