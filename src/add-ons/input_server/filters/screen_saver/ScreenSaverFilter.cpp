/*
 * Copyright 2003-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ryan Leavengood, leavengood@gmail.com
 */


#include "ScreenSaverFilter.h"

#include <Application.h>
#include <Autolock.h>
#include <FindDirectory.h>
#include <MessageRunner.h>
#include <NodeMonitor.h>
#include <OS.h>
#include <Roster.h>
#include <Screen.h>

#include <new>
#include <syslog.h>


static const int32 kNeverBlankCornerSize = 10;
static const int32 kBlankCornerSize = 5;
static const bigtime_t kCornerDelay = 1000000LL;
	// one second

static const int32 kMsgCheckTime = 'SSCT';
static const int32 kMsgCornerInvoke = 'Scin';


extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();


/** Required C func to build the IS Filter */
BInputServerFilter*
instantiate_input_filter()
{
	return new (std::nothrow) ScreenSaverFilter();
}


//	#pragma mark -


ScreenSaverController::ScreenSaverController(ScreenSaverFilter *filter)
	: BLooper("screensaver controller", B_LOW_PRIORITY),
	fFilter(filter)
{
}


void
ScreenSaverController::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
			fFilter->ReloadSettings();
			break;
		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
		{
			const char *signature;
			if (message->FindString("be:signature", &signature) == B_OK
				&& strcasecmp(signature, SCREEN_BLANKER_SIG) == 0)
				fFilter->SetIsRunning(message->what == B_SOME_APP_LAUNCHED);
			break;
		}

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
	: BLocker("screen saver filter"),
	fLastEventTime(system_time()),
	fBlankTime(0),
	fSnoozeTime(0),
	fCurrentCorner(NO_CORNER),
	fFrameNum(0),
	fRunner(NULL),
	fCornerRunner(NULL),
	fWatchingDirectory(false),
	fWatchingFile(false),
	fIsRunning(false)
{
	fController = new (std::nothrow) ScreenSaverController(this);
	if (fController == NULL)
		return;

	BAutolock _(this);

	fController->Run();

	ReloadSettings();
	be_roster->StartWatching(fController);
}


ScreenSaverFilter::~ScreenSaverFilter()
{
	be_roster->StopWatching(fController);

	// We must quit our controller without being locked, or else we might
	// deadlock; when the controller is gone, there is no reason to lock
	// anymore, anyway.
	if (fController->Lock())
		fController->Quit();

	delete fCornerRunner;
	delete fRunner;

	if (fWatchingFile || fWatchingDirectory)
		watch_node(&fNodeRef, B_STOP_WATCHING, fController);
}


/*!	Starts watching the settings file, or if that doesn't exist, the directory
	the settings file will be placed into.
*/
void
ScreenSaverFilter::_WatchSettings()
{
	BEntry entry(fSettings.Path().Path());
	if (entry.Exists()) {
		if (fWatchingFile)
			return;

		if (fWatchingDirectory) {
			watch_node(&fNodeRef, B_STOP_WATCHING, fController);
			fWatchingDirectory = false;
		}
		if (entry.GetNodeRef(&fNodeRef) == B_OK
			&& watch_node(&fNodeRef, B_WATCH_ALL, fController) == B_OK)
			fWatchingFile = true;
	} else {
		if (fWatchingDirectory)
			return;

		if (fWatchingFile) {
			watch_node(&fNodeRef, B_STOP_WATCHING, fController);
			fWatchingFile = false;
		}
		BEntry dir;
		if (entry.GetParent(&dir) == B_OK
			&& dir.GetNodeRef(&fNodeRef) == B_OK
			&& watch_node(&fNodeRef, B_WATCH_DIRECTORY, fController) == B_OK)
			fWatchingDirectory = true;
	}
}


/*!	Starts the screen saver if allowed */
void
ScreenSaverFilter::_Invoke()
{
	if ((fCurrentCorner == fNeverBlankCorner && fNeverBlankCorner != NO_CORNER)
		|| (fSettings.TimeFlags() & ENABLE_SAVER) == 0
		|| fIsRunning
		|| be_roster->IsRunning(SCREEN_BLANKER_SIG))
		return;

	if (be_roster->Launch(SCREEN_BLANKER_SIG) == B_OK) {
		// Already set the running state to avoid launching
		// the blanker twice in any case.
		fIsRunning = true;
		return;
	}

	// Try really hard to launch it. It's very likely that this fails,
	// when we run from the CD and there is only an incomplete mime
	// database for example...
	BPath path;
	if (find_directory(B_SYSTEM_BIN_DIRECTORY, &path) != B_OK
		|| path.Append("screen_blanker") != B_OK)
		path.SetTo("/bin/screen_blanker");
	BEntry entry(path.Path());
	entry_ref ref;
	if (entry.GetRef(&ref) == B_OK 
		&& be_roster->Launch(&ref) == B_OK)
		fIsRunning = true;
}


void
ScreenSaverFilter::ReloadSettings()
{
	BAutolock _(this);
	bool isFirst = !fWatchingDirectory && !fWatchingFile;

	_WatchSettings();

	if (fWatchingDirectory && !isFirst) {
		// there is no settings file yet
		return;
	}

	delete fCornerRunner;
	delete fRunner;
	fRunner = fCornerRunner = NULL;

	fSettings.Load();

	fBlankCorner = fSettings.BlankCorner();
	fNeverBlankCorner = fSettings.NeverBlankCorner();
	fBlankTime = fSnoozeTime = fSettings.BlankTime();
	CheckTime();

	if (fBlankCorner != NO_CORNER || fNeverBlankCorner != NO_CORNER) {
		BMessage invoke(kMsgCornerInvoke);
		fCornerRunner = new (std::nothrow) BMessageRunner(fController,
			&invoke, B_INFINITE_TIMEOUT);
			// will be reset in Filter()
	}

	BMessage check(kMsgCheckTime);
	fRunner = new (std::nothrow) BMessageRunner(fController, &check,
		fSnoozeTime);
	if (fRunner == NULL || fRunner->InitCheck() != B_OK)
		syslog(LOG_ERR, "screen saver filter runner init failed\n");
}


void
ScreenSaverFilter::SetIsRunning(bool isRunning)
{
	// called from the controller BLooper
	BAutolock _(this);
	fIsRunning = isRunning;
}


void
ScreenSaverFilter::CheckTime()
{
	BAutolock _(this);

	bigtime_t now = system_time();
	if (now >= fLastEventTime + fBlankTime)
		_Invoke();

	// TODO: this doesn't work correctly - since the BMessageRunner is not
	// restarted, the next check will be too far away

	// If the screen saver is on OR it was time to come on but it didn't (corner),
	// snooze for blankTime.
	// Otherwise, there was an event in the middle of the last snooze, so snooze
	// for the remainder.
	if (fIsRunning || fLastEventTime + fBlankTime <= now)
		fSnoozeTime = fBlankTime;
	else
		fSnoozeTime = fLastEventTime + fBlankTime - now;

	if (fRunner != NULL)
		fRunner->SetInterval(fSnoozeTime);
}


void
ScreenSaverFilter::CheckCornerInvoke()
{
	BAutolock _(this);

	bigtime_t inactivity = system_time() - fLastEventTime;

	if (fCurrentCorner == fBlankCorner && fBlankCorner != NO_CORNER
		&& inactivity >= kCornerDelay)
		_Invoke();
}


void
ScreenSaverFilter::_UpdateRectangles()
{
	fBlankRect = _ScreenCorner(fBlankCorner, kBlankCornerSize);
	fNeverBlankRect = _ScreenCorner(fNeverBlankCorner, kNeverBlankCornerSize);
}


BRect
ScreenSaverFilter::_ScreenCorner(screen_corner corner, uint32 cornerSize)
{
	BRect frame = BScreen().Frame();

	switch (corner) {
		case UP_LEFT_CORNER:
			return BRect(frame.left, frame.top, frame.left + cornerSize - 1,
				frame.top + cornerSize - 1);

		case UP_RIGHT_CORNER:
			return BRect(frame.right - cornerSize + 1, frame.top, frame.right,
				frame.top + cornerSize - 1);

		case DOWN_RIGHT_CORNER:
			return BRect(frame.right - cornerSize + 1, frame.bottom - cornerSize + 1,
				frame.right, frame.bottom);

		case DOWN_LEFT_CORNER:
			return BRect(frame.left, frame.bottom - cornerSize + 1,
				frame.left + cornerSize - 1, frame.bottom);

		default:
			// return an invalid rectangle
			return BRect(-1, -1, -2, -2);
	}
}


filter_result
ScreenSaverFilter::Filter(BMessage *message, BList *outList)
{
	BAutolock _(this);

	fLastEventTime = system_time();

	switch (message->what) {
		case B_MOUSE_MOVED:
		{
			BPoint where;
			if (message->FindPoint("where", &where) != B_OK)
				break;

			if ((fFrameNum++ % 32) == 0) {
				// Every so many frames, update
				// Used so that we don't update the screen coord's so often
				// Ideally, we would get a message when the screen changes.
				// R5 doesn't do this.
				_UpdateRectangles();
			}

			if (fBlankRect.Contains(where)) {
				fCurrentCorner = fBlankCorner;

				// start screen blanker after one second of inactivity
				if (fCornerRunner != NULL
					&& fCornerRunner->SetInterval(kCornerDelay) != B_OK)
					_Invoke();
				break;
			}

			if (fNeverBlankRect.Contains(where))
				fCurrentCorner = fNeverBlankCorner;
			else
				fCurrentCorner = NO_CORNER;
			break;
		}
	}

	return B_DISPATCH_MESSAGE;
}

