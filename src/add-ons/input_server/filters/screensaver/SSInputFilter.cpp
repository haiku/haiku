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
#include "SSInputFilter.h"

#include <Debug.h>
#define CALLED() SERIAL_PRINT(("%s\n", __PRETTY_FUNCTION__))

extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();

BInputServerFilter* instantiate_input_filter() {  // required C func to build the IS Filter
	return (new SSInputFilter()); 
}


SSController::SSController(SSInputFilter *filter)
	: BLooper("screensaver controller", B_LOW_PRIORITY),
	fFilter(filter)
{
	CALLED();
}


void
SSController::MessageReceived(BMessage *msg)
{
	CALLED();
	SERIAL_PRINT(("what %lx\n", msg->what));
	switch (msg->what) {
		case B_NODE_MONITOR:
			fFilter->ReloadSettings();
			break;
		case SS_CHECK_TIME:
			fFilter->CheckTime();
			break;
		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
		{
			const char *signature;
			if (msg->FindString("be:signature", &signature)==B_OK 
				&& strcasecmp(signature, SCREEN_BLANKER_SIG)==0) {
				fFilter->SetEnabled(msg->what == B_SOME_APP_LAUNCHED);
			}
			SERIAL_PRINT(("mime_sig %s\n", signature));
			break;
		}
		default:
			BLooper::MessageReceived(msg);
	}
}


SSInputFilter::SSInputFilter() 
	: fLastEventTime(0),
		fBlankTime(0),
		fSnoozeTime(0),
		fRtc(0),
		fCurrent(NONE),
		fEnabled(false),
		fFrameNum(0),
		fRunner(NULL),
		fWatchingDirectory(false), 
		fWatchingFile(false) {
	CALLED();
	fSSController = new SSController(this);
	fSSController->Run();

	ReloadSettings();
	be_roster->StartWatching(fSSController);
}


SSInputFilter::~SSInputFilter() {
	delete fRunner;
	if (fWatchingFile)
		watch_node(&fPrefNodeRef, B_STOP_WATCHING, NULL);
	if (fWatchingDirectory)
		watch_node(&fPrefDirNodeRef, B_STOP_WATCHING, NULL);
	be_roster->StopWatching(fSSController);
	delete fSSController;
}


void
SSInputFilter::WatchPreferences() {
	BEntry entry(fPref.GetPath().Path());
	if (entry.Exists()) {
		if (fWatchingFile)
			return;
		if (fWatchingDirectory) {
			watch_node(&fPrefDirNodeRef, B_STOP_WATCHING, NULL);
			fWatchingDirectory = false;
		}
		entry.GetNodeRef(&fPrefNodeRef);
		watch_node(&fPrefNodeRef, B_WATCH_ALL, NULL, fSSController);
		fWatchingFile = true;
	} else {
		if (fWatchingDirectory)
			return;
		if (fWatchingFile) {
			watch_node(&fPrefNodeRef, B_STOP_WATCHING, NULL);
			fWatchingFile = false;
		}
		BEntry dir;
		entry.GetParent(&dir);
		dir.GetNodeRef(&fPrefDirNodeRef);
		watch_node(&fPrefDirNodeRef, B_WATCH_ALL, NULL, fSSController);
		fWatchingDirectory = true;
	}
	
}


void 
SSInputFilter::Invoke() 
{
	CALLED();
	if ((fKeep!=NONE && fCurrent == fKeep) || fEnabled || fPref.TimeFlags()!=1 || be_roster->IsRunning(SCREEN_BLANKER_SIG))
		return; // If mouse is in this corner, never invoke.
	SERIAL_PRINT(("we run screenblanker\n"));
	be_roster->Launch(SCREEN_BLANKER_SIG);
}


void 
SSInputFilter::ReloadSettings()
{
	CALLED();
	if (!fPref.LoadSettings()) {
		SERIAL_PRINT(("preferences loading failed: going to defaults\n"));
	}
	fBlank = fPref.GetBlankCorner();
	fKeep = fPref.GetNeverBlankCorner();
	fBlankTime = fSnoozeTime = fPref.BlankTime();
	CheckTime();
	delete fRunner;
	fRunner = new BMessageRunner(BMessenger(NULL, fSSController), new BMessage(SS_CHECK_TIME), fSnoozeTime, -1);
	if (fRunner->InitCheck() != B_OK) {
		SERIAL_PRINT(("fRunner init failed\n"));
	}
	WatchPreferences();
}


void 
SSInputFilter::Banish() 
{
	CALLED();
	if (!fEnabled)
		return;
	SERIAL_PRINT(("we quit screenblanker\n"));
	BMessenger ssApp(SCREEN_BLANKER_SIG,-1,NULL); // Don't care if it fails
	ssApp.SendMessage(B_QUIT_REQUESTED);
}


void 
SSInputFilter::CheckTime() {
	CALLED();
	fRtc = system_time();
	if (fRtc >= fLastEventTime + fBlankTime)  
		Invoke();
	// If the screen saver is on OR it was time to come on but it didn't (corner), snooze for blankTime
	// Otherwise, there was an event in the middle of the last snooze, so snooze for the remainder
	if (fEnabled || (fLastEventTime+fBlankTime<=fRtc))
		fSnoozeTime = fBlankTime;
	else
		fSnoozeTime = fLastEventTime + fBlankTime - fRtc;
}


void 
SSInputFilter::UpdateRectangles() {
	CALLED();
	BRect frame = BScreen().Frame();
	fTopLeft.Set(frame.left,frame.top,frame.left+CORNER_SIZE,frame.top+CORNER_SIZE);
	fTopRight.Set(frame.right-CORNER_SIZE,frame.top,frame.right,frame.top+CORNER_SIZE);
	fBottomLeft.Set(frame.left,frame.bottom-CORNER_SIZE,frame.left+CORNER_SIZE,frame.bottom);
	fBottomRight.Set(frame.right-CORNER_SIZE,frame.bottom-CORNER_SIZE,frame.right,frame.bottom);
}


void 
SSInputFilter::Cornered(arrowDirection pos) {
	//CALLED();
	fCurrent = pos;
	if (fBlank != NONE && pos == fBlank)
		Invoke();
}


filter_result 
SSInputFilter::Filter(BMessage *msg,BList *outList) {
	fLastEventTime = system_time();
	if (msg->what==B_MOUSE_MOVED) {
		BPoint pos;
		msg->FindPoint("where",&pos);
		if ((fFrameNum++ % 32)==0) // Every so many frames, update
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

