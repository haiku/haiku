/*

"Backgrounds" by Jerome Duval (jerome.duval@free.fr)

(C)2002 OpenBeOS under MIT license

*/

#include "BGMain.h"
#include <TrackerAddOn.h>

BGApplication::BGApplication()
  :BApplication(APP_SIGNATURE)
{
	fBGWin=new BGWindow(BRect(100,100,570,325));
	fBGWin->Show();
}


int main(int, char**)
{	
	BGApplication myApplication;

	// This function doesn't return until the application quits
	myApplication.Run();

	return(0);
}


void process_refs(entry_ref dir_ref, BMessage *msg, void* reserved)
{
	BGWindow *fBGWin=new BGWindow(BRect(100,100,570,325), false);
	fBGWin->ProcessRefs(dir_ref, msg);
	snooze(500);
	fBGWin->Show();
	
	status_t exit_value;
	wait_for_thread(fBGWin->Thread(), &exit_value);
}
