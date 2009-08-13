// license: public domain
// authors: jonas.sundstrom@kirilla.com


#include "ZipOMatic.h"

#include <Alert.h>
#include <Roster.h>
#include <Screen.h>
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
	fSettings(),
	fGotRefs(false),
	fInvoker(new BInvoker(new BMessage(ZIPPO_QUIT_OR_CONTINUE), NULL, this)),
	fWindowFrame(200, 200, 430, 310)
{
	status_t status = _ReadSettings();
	
	if (status != B_OK)
		ErrorMessage("_ReadSettings()", status);	
}


ZipOMatic::~ZipOMatic()
{
	status_t status = _WriteSettings();
	
	if (status != B_OK)
		ErrorMessage("_WriteSettings()", status);
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
		{
			BRect frame;
			if (message->FindRect("frame", &frame) == B_OK)
				fWindowFrame = frame;
			snooze(200000);
			if (CountWindows() == 0)
				Quit();
			break;
		}
		case B_SILENT_RELAUNCH:
			_SilentRelaunch();
			break;

		case ZIPPO_QUIT_OR_CONTINUE:
		{
			int32 button;
			if (message->FindInt32("which", &button) == B_OK)
				if (button == 0) {
					_StopZipping();
				} else  {
					if (CountWindows() == 0)
						Quit();
				}
			break;
		}

		default:
			BApplication::MessageReceived(message);
			break;			
	}
}


bool
ZipOMatic::QuitRequested  (void)
{
	if (CountWindows() <= 0)
		return true;
	
	BWindow* window;
	ZippoWindow* zippo;
	ZippoWindow* lastFoundZippo = NULL;
	int32 zippoCount = 0;
	
	for (int32 i = 0;; i++) {
		window = WindowAt(i);
		if (window == NULL)
			break;
		
		zippo = dynamic_cast<ZippoWindow*>(window);
		if (zippo == NULL)
			continue;
		
		lastFoundZippo = zippo;
		
		if (zippo->Lock()) {
			if (zippo->IsZipping())
				zippoCount++;
			else
				zippo->PostMessage(B_QUIT_REQUESTED);
				
			zippo->Unlock();
		}
	}
	
	if (zippoCount == 1) {
		// This is likely the most frequent case - a single zipper.
		// We post a message to the window so it can put up its own
		// BAlert instead of the app-wide BAlert. This avoids making
		// a difference between having pressed Commmand-W or Command-Q.
		// Closing or quitting, it doesn't matter for a single window.

		if (lastFoundZippo->Lock()) {
			lastFoundZippo->Activate();
			lastFoundZippo->PostMessage(B_QUIT_REQUESTED);
			lastFoundZippo->Unlock();
		}
		return false;
	}

	if (zippoCount > 0) {
		// The multi-zipper case differs from the single-zipper case
		// in that zippers are not paused while the BAlert is up.

		BString question;
		question << "You have " << zippoCount;
		question << " Zip-O-Matic running.\n\nDo you want to stop them?";

		BAlert* alert = new BAlert("Stop or Continue", question.String(),
			"Stop them", "Let them continue", NULL, B_WIDTH_AS_USUAL,
			B_WARNING_ALERT);
		alert->Go(fInvoker);
		alert->Activate();
			// BAlert, being modal, does not show on the current workspace
			// if the application has no window there. Activate() triggers
			// a switch to a workspace where it does have a window.
			
			// TODO: See if AS_ACTIVATE_WINDOW should be handled differently
			// in src/servers/app/Desktop.cpp Desktop::ActivateWindow()
			// or if maybe BAlert should (and does not?) activate itself.
			
		return false;
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
				window->SetWorkspaces(B_CURRENT_WORKSPACE);
				window->Activate();
				window->Unlock();
				break;
			}
			window->Unlock();
		}
	}
	
	if (!foundNonBusyWindow)
	{
		BScreen screen;
		fWindowFrame.OffsetBy(screen.Frame().LeftTop());
		
		_CascadeOnFrameCollision(&fWindowFrame);
		if(!screen.Frame().Contains(fWindowFrame)) {
			fWindowFrame.OffsetTo(screen.Frame().LeftTop());
			fWindowFrame.OffsetBy(20,45);
				// TODO: replace with CenterOnScreen()
		}
		
		ZippoWindow * window = new ZippoWindow(fWindowFrame, message);
		window->Show();
	}
}


void
ZipOMatic::_StopZipping()
{
	BWindow* window;
	ZippoWindow* zippo;
	BList list;
	
	for (int32 i = 0;; i++) {
		window = WindowAt(i);
		if (window == NULL)
			break;
		
		zippo = dynamic_cast<ZippoWindow*>(window);
		if (zippo == NULL)
			continue;
		
		list.AddItem(zippo);
	}

	for (int32 i = 0;; i++) {
		zippo = static_cast<ZippoWindow*>(list.ItemAt(i));
		if (zippo == NULL)
			break;

		if (zippo->Lock()) {
			if (zippo->IsZipping())
				zippo->StopZipping();

			zippo->PostMessage(B_QUIT_REQUESTED);
			zippo->Unlock();
		}
	}	
}


status_t
ZipOMatic::_ReadSettings()
{
	status_t status = B_OK;
	
	status = fSettings.SetTo("zipomatic.msg");
	if (status != B_OK)
		return status;

	status = fSettings.InitCheck();
	if (status != B_OK)
		return status;

	status = fSettings.InitCheck();
	if (status != B_OK)
		return status;

	status = fSettings.ReadSettings();
	if (status != B_OK)
		return status;

	BRect frame;
	status = fSettings.FindRect("frame", &frame);
	if (status != B_OK)
		return status;
	
	fWindowFrame = frame;
	
	return B_OK;
}


status_t
ZipOMatic::_WriteSettings()
{
	status_t status = B_OK;

	status = fSettings.InitCheck();
	if (status != B_OK)
		return status;

	status = fSettings.MakeEmpty();
	if (status != B_OK)
		return status;

	status = fSettings.AddRect("frame", fWindowFrame);
	if (status != B_OK)
		return status;
	
	status = fSettings.WriteSettings();
	if (status != B_OK)
		return status;
	
	return B_OK;
}


void
ZipOMatic::_CascadeOnFrameCollision(BRect* frame)
{
	BWindow* window;
	ZippoWindow* zippo;
	BList list;
	
	for (int32 i = 0;; i++) {
		window = WindowAt(i);
		if (window == NULL)
			break;
		
		zippo = dynamic_cast<ZippoWindow*>(window);
		if (zippo == NULL)
			continue;
		
		list.AddItem(zippo);
	}

	for (int32 i = 0;; i++) {
		zippo = static_cast<ZippoWindow*>(list.ItemAt(i));
		if (zippo == NULL)
			break;

		if (zippo->Lock()) {
			if (frame->LeftTop() == zippo->Frame().LeftTop())
				frame->OffsetBy(20, 20);
			zippo->Unlock();
		}
	}	
}

