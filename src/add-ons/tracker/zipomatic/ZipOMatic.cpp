// license: public domain
// authors: jonas.sundstrom@kirilla.com


#include "ZipOMatic.h"

#include <Roster.h>
#include <TrackerAddOn.h>

#include "ZipOMaticMisc.h"
#include "ZipOMaticWindow.h"


extern "C" void 
process_refs(entry_ref dirRef, BMessage* message, void*)
{
	status_t status = B_OK;
	type_code refType = B_REF_TYPE;
	int32 refCount = 0;
	
	status = message->GetInfo("refs", &refType, &refCount);
	if (status != B_OK || refCount < 1)
		be_roster->Launch(ZIPOMATIC_APP_SIG);
	else
		be_roster->Launch(ZIPOMATIC_APP_SIG, message);
}


int
main()
{
	ZipOMatic app;
	app.Run();

	return 0;
}


ZipOMatic::ZipOMatic()
	:
	BApplication(ZIPOMATIC_APP_SIG),
	fGotRefs(false)
{

}


ZipOMatic::~ZipOMatic()
{

}


void 
ZipOMatic::RefsReceived(BMessage* message)
{
	if (IsLaunching())
		fGotRefs = true;
	
	BMessage* msg = new BMessage(*message);

	_UseExistingOrCreateNewWindow(msg);
}


void
ZipOMatic::ReadyToRun()
{
	if (!fGotRefs)
		_UseExistingOrCreateNewWindow();
}


void 
ZipOMatic::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case ZIPPO_WINDOW_QUIT:
			snooze(200000);
			if (CountWindows() == 0)
				Quit();
			break;
					
		case B_SILENT_RELAUNCH:
			_SilentRelaunch();
			break;

		default:
			BApplication::MessageReceived(message);
			break;			
	}
}


bool
ZipOMatic::QuitRequested  (void)
{
	// Overriding BApplication's default behaviour on purpose
	// so we can have multiple zippers pause in unison.
	
	if (CountWindows() <= 0)
		return true;
	
	BList list(5);
	BWindow* window;
	
	for (int32 i = 0;; i++) {
		window =  WindowAt(i);
		if (window == NULL)
			break;
			
		list.AddItem(window);
	}
	
	while (true) {
		window = (BWindow*) list.RemoveItem(int32(0));
		if (window == NULL)
			break;
		
		if (window->Lock()) {
			window->PostMessage(B_QUIT_REQUESTED);
			window->Unlock();
		}
	}

	if (CountWindows() <= 0)
		return true;
	
	return false;
}


void
ZipOMatic::_SilentRelaunch()
{
	_UseExistingOrCreateNewWindow();
}


void
ZipOMatic::_UseExistingOrCreateNewWindow(BMessage* message)
{
	int32 windowCount = 0;
	ZippoWindow* window;
	bool foundNonBusyWindow = false;

	while (1) {
		window = dynamic_cast<ZippoWindow*>(WindowAt(windowCount++));
		if (window == NULL)
			break;
		
		if (window->Lock()) {
			if (!window->IsZipping()) {
				foundNonBusyWindow = true;
				if (message != NULL)
					window->PostMessage(message);
				window->Activate();
				window->Unlock();
				break;
			}
			window->Unlock();
		}
	}
	
	if (!foundNonBusyWindow)
	{
		ZippoWindow * window = new ZippoWindow(message);
		window->Show();
	}
}

