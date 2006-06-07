/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ScreenSaverFilter.h"

#include <Application.h>
#include <MessageRunner.h>
#include <NodeMonitor.h>
#include <OS.h>
#include <Roster.h>
#include <Screen.h>

#include <Debug.h>

#define CALLED() SERIAL_PRINT(("%s\n", __PRETTY_FUNCTION__))


static const int32 kNeverBlankCornerSize = 10;
static const int32 kBlankCornerSize = 5;

static const int32 kMsgCheckTime = 'SSCT';
static const int32 kMsgCornerInvoke = 'Scin';


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
ScreenSaverController::MessageReceived(BMessage *message)
{
	CALLED();
	SERIAL_PRINT(("what %lx\n", message->what));

	switch (message->what) {
		case B_NODE_MONITOR:
			fFilter->ReloadSettings();
			break;
		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
		{
			const char *signature;
			if (message->FindString("be:signature", &signature) == B_OK 
				&& strcasecmp(signature, SCREEN_BLANKER_SIG) == 0) {
				fFilter->SetEnabled(message->what == B_SOME_APP_LAUNCHED);
			}
			SERIAL_PRINT(("mime_sig %s\n", signature));
			break;
		}

		case kMsgSuspendScreenSaver:
			//fFilter->Suspend(msg);
			break;

		case kMsgCheckTime:
			fFilter->CheckTime();
			break;

		case kMsgCornerInvoke:
			fFilter->CheckCornerInvoke();
			break;

		default:
			BLooper::MessageReceived(message);
	}
}


//	#pragma mark -


ScreenSaverFilter::ScreenSaverFilter() 
	:
	fLastEventTime(0),
	fBlankTime(0),
	fSnoozeTime(0),
	fCurrentCorner(NONE),
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
ScreenSaverFilter::_WatchPreferences()
{
	BEntry entry(fSettings.Path().Path());
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
ScreenSaverFilter::_Invoke() 
{
	CALLED();
	if (fCurrentCorner == fNeverBlankCorner || fEnabled
		|| fSettings.TimeFlags() != 1
		|| be_roster->IsRunning(SCREEN_BLANKER_SIG))
		return;

	SERIAL_PRINT(("we run screenblanker\n"));
	be_roster->Launch(SCREEN_BLANKER_SIG);
}


void
ScreenSaverFilter::ReloadSettings()
{
	CALLED();
	if (!fSettings.LoadSettings()) {
		SERIAL_PRINT(("preferences loading failed: going to defaults\n"));
	}

	fBlankCorner = fSettings.GetBlankCorner();
	fNeverBlankCorner = fSettings.GetNeverBlankCorner();
	fBlankTime = fSnoozeTime = fSettings.BlankTime();
	CheckTime();

	delete fRunner;
	fRunner = new BMessageRunner(BMessenger(NULL, fController),
		new BMessage(kMsgCheckTime), fSnoozeTime);
	if (fRunner->InitCheck() != B_OK) {
		SERIAL_PRINT(("fRunner init failed\n"));
	}

	_WatchPreferences();
}


void
ScreenSaverFilter::_Banish() 
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
		_Invoke();

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
ScreenSaverFilter::CheckCornerInvoke()
{
	if (fCurrentCorner == fBlankCorner)
		_Invoke();
}


void
ScreenSaverFilter::_UpdateRectangles()
{
	// TODO: make this better if possible at all (in a clean way)
	CALLED();

	fBlankRect = _ScreenCorner(fBlankCorner, kBlankCornerSize);
	fNeverBlankRect = _ScreenCorner(fNeverBlankCorner, kNeverBlankCornerSize);
}


BRect
ScreenSaverFilter::_ScreenCorner(screen_corner corner, uint32 cornerSize)
{
	BRect frame = BScreen().Frame();

	switch (corner) {
		case UPLEFT:
			return BRect(frame.left, frame.top, frame.left + cornerSize - 1,
				frame.top + cornerSize - 1);

		case UPRIGHT:
			return BRect(frame.right, frame.top, frame.right - cornerSize + 1,
				frame.top + cornerSize - 1);

		case DOWNRIGHT:
			return BRect(frame.right, frame.bottom, frame.right - cornerSize + 1,
				frame.bottom - cornerSize + 1);

		case DOWNLEFT:
			return BRect(frame.left, frame.bottom, frame.left + cornerSize - 1,
				frame.bottom - cornerSize + 1);

		default:
			// return an invalid rectangle
			return BRect(-1, -1, -2, -2);
	}
}


filter_result
ScreenSaverFilter::Filter(BMessage *message, BList *outList)
{
	fLastEventTime = system_time();

	switch (message->what) {
		case B_MOUSE_MOVED:
		{
			BPoint pos;
			message->FindPoint("where", &pos);
			if ((fFrameNum++ % 32) == 0) {
				// Every so many frames, update
				// Used so that we don't update the screen coord's so often
				// Ideally, we would get a message when the screen changes.
				// R5 doesn't do this.
				_UpdateRectangles();
			}

			if (fBlankRect.Contains(pos)) {
				fCurrentCorner = fBlankCorner;

				// start screen blanker after one second
				BMessage invoke(kMsgCornerInvoke);
				if (BMessageRunner::StartSending(fController, &invoke, 1000000ULL, 1) != B_OK)
					_Invoke();
				break;
			}

			if (fNeverBlankRect.Contains(pos))
				fCurrentCorner = fNeverBlankCorner;
			else
				fCurrentCorner = NONE;
			break;
		}

		case B_KEY_UP:
		case B_KEY_DOWN:
		{
			// we ignore the Print-Screen key to make screen shots of
			// screen savers possible
			int32 key;
			if (fEnabled && message->FindInt32("key", &key) == B_OK && key == 0xe)
				return B_DISPATCH_MESSAGE;
		}
	}

	_Banish();
	return B_DISPATCH_MESSAGE;
}

