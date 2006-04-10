/*
 * Copyright 2003-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */


#include <NodeMonitor.h>
#include <OS.h>
#include <Roster.h>
#include <Screen.h>
#include <Application.h>
#include "ScreenSaverFilter.h"

#include <Debug.h>
#define CALLED() SERIAL_PRINT(("%s\n", __PRETTY_FUNCTION__))


static const int kCornerSize = 10;
static const int32 kMsgCheckTime = 'SSCT';


extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();


/** required C func to build the IS Filter */

BInputServerFilter*
instantiate_input_filter()
{
	return new ScreenSaverFilter();
}


//	#pragma mark -


ScreenSaverController::ScreenSaverController(ScreenSaverFilter *filter)
	: BLooper("screensaver controller", B_LOW_PRIORITY),
	fFilter(filter)
{
	CALLED();
}


void
ScreenSaverController::MessageReceived(BMessage *msg)
{
	CALLED();
	SERIAL_PRINT(("what %lx\n", msg->what));

	switch (msg->what) {
		case B_NODE_MONITOR:
			fFilter->ReloadSettings();
			break;
		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
		{
			const char *signature;
			if (msg->FindString("be:signature", &signature) == B_OK 
				&& strcasecmp(signature, SCREEN_BLANKER_SIG) == 0) {
				fFilter->SetEnabled(msg->what == B_SOME_APP_LAUNCHED);
			}
			SERIAL_PRINT(("mime_sig %s\n", signature));
			break;
		}

		case kMsgCheckTime:
			fFilter->CheckTime();
			break;

		default:
			BLooper::MessageReceived(msg);
	}
}


//	#pragma mark -


ScreenSaverFilter::ScreenSaverFilter() 
	: fLastEventTime(0),
		fBlankTime(0),
		fSnoozeTime(0),
		fCurrent(NONE),
		fEnabled(false),
		fFrameNum(0),
		fRunner(NULL),
		fWatchingDirectory(false), 
		fWatchingFile(false)
{
	CALLED();
	fController = new ScreenSaverController(this);
	fController->Run();

	ReloadSettings();
	be_roster->StartWatching(fController);
}


ScreenSaverFilter::~ScreenSaverFilter()
{
	delete fRunner;

	if (fWatchingFile)
		watch_node(&fPrefsNodeRef, B_STOP_WATCHING, NULL);
	if (fWatchingDirectory)
		watch_node(&fPrefsDirNodeRef, B_STOP_WATCHING, NULL);

	be_roster->StopWatching(fController);
	delete fController;
}


void
ScreenSaverFilter::WatchPreferences()
{
	BEntry entry(fPrefs.GetPath().Path());
	if (entry.Exists()) {
		if (fWatchingFile)
			return;
		if (fWatchingDirectory) {
			watch_node(&fPrefsDirNodeRef, B_STOP_WATCHING, NULL);
			fWatchingDirectory = false;
		}
		entry.GetNodeRef(&fPrefsNodeRef);
		watch_node(&fPrefsNodeRef, B_WATCH_ALL, NULL, fController);
		fWatchingFile = true;
	} else {
		if (fWatchingDirectory)
			return;
		if (fWatchingFile) {
			watch_node(&fPrefsNodeRef, B_STOP_WATCHING, NULL);
			fWatchingFile = false;
		}
		BEntry dir;
		entry.GetParent(&dir);
		dir.GetNodeRef(&fPrefsDirNodeRef);
		watch_node(&fPrefsDirNodeRef, B_WATCH_ALL, NULL, fController);
		fWatchingDirectory = true;
	}
}


void 
ScreenSaverFilter::Invoke() 
{
	CALLED();
	if ((fKeep != NONE && fCurrent == fKeep)
		|| fEnabled
		|| fPrefs.TimeFlags() != 1
		|| be_roster->IsRunning(SCREEN_BLANKER_SIG)) {
		// If mouse is in this corner, never invoke.
		return;
	}

	SERIAL_PRINT(("we run screenblanker\n"));
	be_roster->Launch(SCREEN_BLANKER_SIG);
}


void 
ScreenSaverFilter::ReloadSettings()
{
	CALLED();
	if (!fPrefs.LoadSettings()) {
		SERIAL_PRINT(("preferences loading failed: going to defaults\n"));
	}

	fBlank = fPrefs.GetBlankCorner();
	fKeep = fPrefs.GetNeverBlankCorner();
	fBlankTime = fSnoozeTime = fPrefs.BlankTime();
	CheckTime();

	delete fRunner;
	fRunner = new BMessageRunner(BMessenger(NULL, fController),
		new BMessage(kMsgCheckTime), fSnoozeTime, -1);
	if (fRunner->InitCheck() != B_OK) {
		SERIAL_PRINT(("fRunner init failed\n"));
	}

	WatchPreferences();
}


void 
ScreenSaverFilter::Banish() 
{
	CALLED();
	if (!fEnabled)
		return;

	SERIAL_PRINT(("we quit screenblanker\n"));

	// Don't care if it fails
	BMessenger blankerMessenger(SCREEN_BLANKER_SIG, -1, NULL);
	blankerMessenger.SendMessage(B_QUIT_REQUESTED);
}


void 
ScreenSaverFilter::CheckTime()
{
	CALLED();
	bigtime_t now = system_time();
	if (now >= fLastEventTime + fBlankTime)  
		Invoke();

	// TODO: this doesn't work correctly - since the BMessageRunner is not
	// restarted, the next check will be too far away

	// If the screen saver is on OR it was time to come on but it didn't (corner),
	// snooze for blankTime.
	// Otherwise, there was an event in the middle of the last snooze, so snooze
	// for the remainder.
	if (fEnabled || fLastEventTime + fBlankTime <= now)
		fSnoozeTime = fBlankTime;
	else
		fSnoozeTime = fLastEventTime + fBlankTime - now;

	if (fRunner)
		fRunner->SetInterval(fSnoozeTime);
}


void 
ScreenSaverFilter::UpdateRectangles()
{
	// TODO: make this better if possible at all (in a clean way)
	CALLED();
	BRect frame = BScreen().Frame();

	fTopLeft.Set(frame.left, frame.top,
		frame.left + kCornerSize, frame.top + kCornerSize);
	fTopRight.Set(frame.right - kCornerSize, frame.top,
		frame.right, frame.top + kCornerSize);
	fBottomLeft.Set(frame.left, frame.bottom - kCornerSize,
		frame.left + kCornerSize, frame.bottom);
	fBottomRight.Set(frame.right - kCornerSize, frame.bottom - kCornerSize,
		frame.right, frame.bottom);
}


void 
ScreenSaverFilter::Cornered(arrowDirection pos)
{
	//CALLED();
	fCurrent = pos;
	if (fBlank != NONE && pos == fBlank)
		Invoke();
}


filter_result 
ScreenSaverFilter::Filter(BMessage *msg, BList *outList)
{
	fLastEventTime = system_time();

	if (msg->what == B_MOUSE_MOVED) {
		BPoint pos;
		msg->FindPoint("where",&pos);
		if ((fFrameNum++ % 32) == 0) // Every so many frames, update
			UpdateRectangles();

		if (fTopLeft.Contains(pos)) 
			Cornered(UPLEFT);
		else if (fTopRight.Contains(pos)) 
			Cornered(UPRIGHT);
		else if (fBottomLeft.Contains(pos)) 
			Cornered(DOWNLEFT);
		else if (fBottomRight.Contains(pos)) 
			Cornered(DOWNRIGHT);
		else {
			Cornered(NONE);
			Banish();
		}
	} else 
		Banish();

	return B_DISPATCH_MESSAGE;
}

