#include "SSInputFilter.h"
#include "OS.h"
#include "Roster.h"

extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

// TODO:
// Read SS preferences for timing, corner info
// If corners get confused (l/r), check size of enums

SSISFilter *fltr;

BInputServerFilter* instantiate_input_filter() 
{ 
	return (fltr=new SSISFilter()); 
}

filter_result timeUpFunc (BMessage *message,BHandler **target, BMessageFilter *messageFilter) {
	fltr->CheckTime();
	return B_SKIP_MESSAGE;
}

SSISFilter::SSISFilter() : enabled(false) {
	messenger=new BMessenger(NULL,-1); // Get the current teams default handler
	timeMsg=new BMessage(timeUp);
	runner=new BMessageRunner(*messenger, timeMsg, 3000000,-1);  // CHANGE ME!!!
	filter=new BMessageFilter(timeUp,timeUpFunc);	
}

void SSISFilter::Invoke(void) {
	status_t err;
	if (current==keep)
		return; // If mouse is in this corner, never invoke.
	be_roster->Launch("application/x-vnd.OBOS-ScreenSaverApp");
	ssApp=new BMessenger("application/x-vnd.OBOS-ScreenSaverApp",-1,&err);
	enabled=true;
}

void SSISFilter::Banish(void) {
	ssApp->SendMessage('QUIT');
	delete ssApp;
	enabled=true;
}

void SSISFilter::CheckTime(void) {
	uint32 rtc;
	if ((rtc=real_time_clock())>=first+blankTime) 
		Invoke();
	else
		runner->SetInterval(first+blankTime-rtc); // Check back at the time when, if nothing is pressed/moved between now and then, we should blank...	
}

void SSISFilter::UpdateRectangles(void) {
	BRegion region;
	BRect frame;
	GetScreenRegion(&region);
	frame=region.Frame();

	topLeft.Set(frame.left,frame.top,frame.left+CORNER_SIZE,frame.top+CORNER_SIZE);
	topRight.Set(frame.right-CORNER_SIZE,frame.top,frame.right,frame.top+CORNER_SIZE);
	bottomLeft.Set(frame.left,frame.bottom-CORNER_SIZE,frame.left+CORNER_SIZE,frame.bottom);
	bottomRight.Set(frame.right-CORNER_SIZE,frame.bottom-CORNER_SIZE,frame.right,frame.bottom);
}

void SSISFilter::Cornered(cornerPos pos) {
	current=pos;
	if (pos==blank)
		Invoke();
}

filter_result SSISFilter::Filter(BMessage *msg,BList *outList) {
	if (msg->what==B_SCREEN_CHANGED)
		UpdateRectangles();
	else if (enabled && msg->what==B_MOUSE_MOVED) {
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
	first=real_time_clock();
}

SSISFilter::~SSISFilter()
{
	;
}
