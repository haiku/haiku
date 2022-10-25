/*
 * Copyright 2005-2008, Ingo Weinhold, bonefish@users.sf.net.
 * Copyright 2006-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2006-2008, Stephan Aßmus.
 * Copyright 2006, Ryan Leavengood.
 * Copyright 2021, Jacob Secunda.
 *
 * Distributed under the terms of the MIT License.
 */

#include "ShutdownProcess.h"

#include <new>
#include <string.h>

#include <signal.h>
#include <unistd.h>

#include <Alert.h>
#include <AppFileInfo.h>
#include <AppMisc.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Button.h>
#include <ControlLook.h>
#include <Catalog.h>
#include <File.h>
#include <Message.h>
#include <MessagePrivate.h>
#include <RegistrarDefs.h>
#include <Roster.h>		// for B_REQUEST_QUIT
#include <Screen.h>
#include <String.h>
#include <TextView.h>
#include <Window.h>

#include <TokenSpace.h>
#include <util/DoublyLinkedList.h>

#include <syscalls.h>

#include "AppInfoListMessagingTargetSet.h"
#include "Debug.h"
#include "EventQueue.h"
#include "MessageDeliverer.h"
#include "MessageEvent.h"
#include "Registrar.h"
#include "RosterAppInfo.h"
#include "TRoster.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ShutdownProcess"


using std::nothrow;
using namespace BPrivate;

// The time span a non-background application has after the quit message has
// been delivered (more precisely: has been handed over to the
// MessageDeliverer).
static const bigtime_t kAppQuitTimeout = 3000000; // 3 s

// The time span a background application has after the quit message has been
// delivered (more precisely: has been handed over to the MessageDeliverer).
static const bigtime_t kBackgroundAppQuitTimeout = 3000000; // 3 s

// The time span non-app processes have after the TERM signal has been send
// to them before they get a KILL signal.
static const bigtime_t kNonAppQuitTimeout = 500000; // 0.5 s

// The time span the app that has aborted the shutdown shall be displayed in
// the shutdown window before closing it automatically.
static const bigtime_t kDisplayAbortingAppTimeout = 3000000; // 3 s

// The speed of the animation played when an application is blocked on a modal
// panel.
static const bigtime_t kIconAnimateInterval = 50000 * B_LARGE_ICON; // 0.05 s

// message what fields (must not clobber the registrar's message namespace)
enum {
	MSG_PHASE_TIMED_OUT		= 'phto',
	MSG_DONE				= 'done',
	MSG_KILL_APPLICATION	= 'kill',
	MSG_CANCEL_SHUTDOWN		= 'cncl',
	MSG_REBOOT_SYSTEM		= 'lbot',
};

// internal events
enum {
	NO_EVENT,
	ABORT_EVENT,
	TIMEOUT_EVENT,
	APP_QUIT_EVENT,
	KILL_APP_EVENT,
	REBOOT_SYSTEM_EVENT,
	DEBUG_EVENT
};

// phases
enum {
	INVALID_PHASE						= -1,
	USER_APP_TERMINATION_PHASE			= 0,
	SYSTEM_APP_TERMINATION_PHASE		= 1,
	BACKGROUND_APP_TERMINATION_PHASE	= 2,
	OTHER_PROCESSES_TERMINATION_PHASE	= 3,
	ABORTED_PHASE						= 4,
	DONE_PHASE							= 5,
};


static bool
inverse_compare_by_registration_time(const RosterAppInfo* info1,
	const RosterAppInfo* info2)
{
	return (info2->registration_time < info1->registration_time);
}


/*!	\brief Used to avoid type matching problems when throwing a constant.
*/
static inline
void
throw_error(status_t error)
{
	throw error;
}


class ShutdownProcess::TimeoutEvent : public MessageEvent {
public:
	TimeoutEvent(BHandler* target)
		: MessageEvent(0, target, MSG_PHASE_TIMED_OUT)
	{
		SetAutoDelete(false);

		fMessage.AddInt32("phase", INVALID_PHASE);
		fMessage.AddInt32("team", -1);
	}

	void SetPhase(int32 phase)
	{
		fMessage.ReplaceInt32("phase", phase);
	}

	void SetTeam(team_id team)
	{
		fMessage.ReplaceInt32("team", team);
	}

	static int32 GetMessagePhase(BMessage* message)
	{
		int32 phase;
		if (message->FindInt32("phase", &phase) != B_OK)
			phase = INVALID_PHASE;

		return phase;
	}

	static int32 GetMessageTeam(BMessage* message)
	{
		team_id team;
		if (message->FindInt32("team", &team) != B_OK)
			team = -1;

		return team;
	}
};


class ShutdownProcess::InternalEvent
	: public DoublyLinkedListLinkImpl<InternalEvent> {
public:
	InternalEvent(uint32 type, team_id team, int32 phase)
		:
		fType(type),
		fTeam(team),
		fPhase(phase)
	{
	}

	uint32 Type() const			{ return fType; }
	team_id Team() const		{ return fTeam; }
	int32 Phase() const			{ return fPhase; }

private:
	uint32	fType;
	int32	fTeam;
	int32	fPhase;
};


struct ShutdownProcess::InternalEventList : DoublyLinkedList<InternalEvent> {
};


class ShutdownProcess::QuitRequestReplyHandler : public BHandler {
public:
	QuitRequestReplyHandler(ShutdownProcess* shutdownProcess)
		: BHandler("shutdown quit reply handler"),
		fShutdownProcess(shutdownProcess)
	{
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case B_REPLY:
			{
				bool result;
				thread_id thread;
				if (message->FindBool("result", &result) == B_OK
					&& message->FindInt32("thread", &thread) == B_OK) {
					if (!result)
						fShutdownProcess->_NegativeQuitRequestReply(thread);
				}

				break;
			}

			default:
				BHandler::MessageReceived(message);
				break;
		}
	}

private:
	ShutdownProcess	*fShutdownProcess;
};


class ShutdownProcess::ShutdownWindow : public BAlert {
public:
	ShutdownWindow()
		:
		BAlert(),
		fKillAppMessage(NULL),
		fCurrentApp(-1),

		fCurrentIconBitmap(NULL),
		fNormalIconBitmap(NULL),
		fTintedIconBitmap(NULL),
		fAnimationActive(false),
		fAnimationWorker(-1),
		fCurrentAnimationRow(-1),
		fAnimationLightenPhase(true)
	{
		SetTitle(B_TRANSLATE("Shutdown status"));
		SetWorkspaces(B_ALL_WORKSPACES);
		SetLook(B_TITLED_WINDOW_LOOK);
		SetFlags(Flags() | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE);

		SetButtonWidth(B_WIDTH_AS_USUAL);
		SetType(B_EMPTY_ALERT);
	}

	~ShutdownWindow()
	{
		atomic_set(&fAnimationActive, false);
		wait_for_thread(fAnimationWorker, NULL);

		delete fNormalIconBitmap;
		delete fTintedIconBitmap;

		for (int32 i = 0; AppInfo* info = (AppInfo*)fAppInfos.ItemAt(i); i++) {
			delete info;
		}
	}

	virtual bool QuitRequested()
	{
		return false;
	}

	status_t Init(BMessenger target)
	{
		// kill app button
		AddButton(B_TRANSLATE("Kill application"));
		fKillAppButton = ButtonAt(CountButtons() - 1);

		BMessage* message = new BMessage(MSG_KILL_APPLICATION);
		if (!message)
			return B_NO_MEMORY;
		message->AddInt32("team", -1);
		fKillAppMessage = message;
		fKillAppButton->SetMessage(message);
		fKillAppButton->SetTarget(target);

		// cancel shutdown button
		AddButton(B_TRANSLATE("Cancel shutdown"));
		fCancelShutdownButton = ButtonAt(CountButtons() - 1);

		message = new BMessage(MSG_CANCEL_SHUTDOWN);
		if (!message)
			return B_NO_MEMORY;
		fCancelShutdownButton->SetMessage(message);
		fCancelShutdownButton->SetTarget(target);

		// reboot system button
		AddButton(B_TRANSLATE("Restart system"));
		fRebootSystemButton = ButtonAt(CountButtons() - 1);
		fRebootSystemButton->Hide();

		message = new BMessage(MSG_REBOOT_SYSTEM);
		if (!message)
			return B_NO_MEMORY;
		fRebootSystemButton->SetMessage(message);
		fRebootSystemButton->SetTarget(target);

		// aborted OK button
		AddButton(B_TRANSLATE("OK"));
		fAbortedOKButton = ButtonAt(CountButtons() - 1);
		fAbortedOKButton->Hide();

		message = new BMessage(MSG_CANCEL_SHUTDOWN);
		if (!message)
			return B_NO_MEMORY;
		fAbortedOKButton->SetMessage(message);
		fAbortedOKButton->SetTarget(target);

		return B_OK;
	}

	status_t AddApp(team_id team, BBitmap* appIcon)
	{
		AppInfo* info = new(nothrow) AppInfo;
		if (!info) {
			delete appIcon;
			return B_NO_MEMORY;
		}

		info->team = team;
		info->appIcon = appIcon;

		if (!fAppInfos.AddItem(info)) {
			delete info;
			return B_NO_MEMORY;
		}

		return B_OK;
	}

	void RemoveApp(team_id team)
	{
		int32 index = _AppInfoIndexOf(team);
		if (index < 0)
			return;

		if (team == fCurrentApp)
			SetCurrentApp(-1);

		AppInfo* info = (AppInfo*)fAppInfos.RemoveItem(index);
		delete info;
	}

	void SetCurrentApp(team_id team)
	{
		AppInfo* info = (team >= 0 ? _AppInfoFor(team) : NULL);

		fCurrentApp = team;
		SetAppInfo(info);

		fKillAppMessage->ReplaceInt32("team", team);
	}

	void SetText(const char* text)
	{
		const int32 initialLength = TextView()->TextLength(),
			initialLines = TextView()->CountLines();

		BAlert::SetText(text);

		if (TextView()->CountLines() > initialLines
				|| TextView()->CountLines() > (initialLength * 2))
			ResizeToPreferred();
	}

	void SetCancelShutdownButtonEnabled(bool enable)
	{
		fCancelShutdownButton->SetEnabled(enable);
	}

	void SetKillAppButtonEnabled(bool enable)
	{
		if (enable != fKillAppButton->IsEnabled()) {
			fKillAppButton->SetEnabled(enable);

			if (enable)
				fKillAppButton->Show();
			else
				fKillAppButton->Hide();
			ResizeToPreferred();
		}
	}

	void SetWaitAnimationEnabled(bool enable)
	{
		IconWaitAnimationEnabled(enable);
	}

	void SetWaitForShutdown()
	{
		fKillAppButton->Hide();
		fCancelShutdownButton->Hide();
		fRebootSystemButton->MakeDefault(true);
		fRebootSystemButton->Show();

		SetTitle(B_TRANSLATE("System is shut down"));
		SetText(B_TRANSLATE("It's now safe to turn off the computer."));
	}

	void SetWaitForAbortedOK()
	{
		fKillAppButton->Hide();
		fCancelShutdownButton->Hide();
		fAbortedOKButton->MakeDefault(true);
		fAbortedOKButton->Show();
		ResizeToPreferred();

		SetTitle(B_TRANSLATE("Shutdown aborted"));
	}

private:
	struct AppInfo {
		team_id		team;
		BBitmap*	appIcon;

		~AppInfo()
		{
			delete appIcon;
		}
	};

	int32 _AppInfoIndexOf(team_id team)
	{
		if (team < 0)
			return -1;

		for (int32 i = 0; AppInfo* info = (AppInfo*)fAppInfos.ItemAt(i); i++) {
			if (info->team == team)
				return i;
		}

		return -1;
	}

	AppInfo* _AppInfoFor(team_id team)
	{
		int32 index = _AppInfoIndexOf(team);
		return (index >= 0 ? (AppInfo*)fAppInfos.ItemAt(index) : NULL);
	}

private:
	void SetAppInfo(AppInfo* info)
	{
		IconWaitAnimationEnabled(false);

		BAutolock lock(this);
		if (!lock.IsLocked())
			return;

		delete fNormalIconBitmap;
		fNormalIconBitmap = NULL;

		delete fTintedIconBitmap;
		fTintedIconBitmap = NULL;

		// We do not delete the present fCurrentIconBitmap as the BAlert owns it.
		fCurrentIconBitmap = NULL;

		if (info != NULL && info->appIcon != NULL
			&& info->appIcon->IsValid()) {
			fCurrentIconBitmap = new BBitmap(info->appIcon->Bounds(), B_RGBA32);

			if (fCurrentIconBitmap == NULL
				|| fCurrentIconBitmap->ImportBits(info->appIcon) != B_OK) {
				delete fCurrentIconBitmap;
				fCurrentIconBitmap = NULL;
			} else
				SetIcon(fCurrentIconBitmap);
		} else
			SetIcon(NULL);
	}

	void IconWaitAnimationEnabled(bool enable)
	{
		if (atomic_get(&fAnimationActive) == enable)
			return;

		BAutolock lock(this);
		if (!lock.IsLocked())
			return;

		if (enable) {
			if (fCurrentIconBitmap == NULL
				|| !fCurrentIconBitmap->IsValid())
				return;

			if (fNormalIconBitmap == NULL
				|| !fNormalIconBitmap->IsValid()) {
				delete fNormalIconBitmap;
				fNormalIconBitmap = NULL;

				fNormalIconBitmap = new BBitmap(fCurrentIconBitmap->Bounds(),
					B_BITMAP_NO_SERVER_LINK, B_RGBA32);

				if (fNormalIconBitmap == NULL
					|| fNormalIconBitmap->ImportBits(fCurrentIconBitmap)
						!= B_OK) {
					delete fNormalIconBitmap;
					fNormalIconBitmap = NULL;

					return;
				}
			}

			if (fTintedIconBitmap == NULL
				|| !fTintedIconBitmap->IsValid()) {
				delete fTintedIconBitmap;
				fTintedIconBitmap = NULL;

				fTintedIconBitmap = new BBitmap(fNormalIconBitmap->Bounds(),
					B_BITMAP_NO_SERVER_LINK, B_RGBA32);

				if (fTintedIconBitmap == NULL
					|| fTintedIconBitmap->ImportBits(fNormalIconBitmap)
						!= B_OK) {
					delete fTintedIconBitmap;
					fTintedIconBitmap = NULL;

					return;
				}

				int32 width =
					fTintedIconBitmap->Bounds().IntegerWidth() + 1;
				int32 height =
					fTintedIconBitmap->Bounds().IntegerHeight() + 1;
				int32 rowLength = fTintedIconBitmap->BytesPerRow();

				uint8* iconBits = (uint8*)fTintedIconBitmap->Bits();

				for (int32 y = 0; y < height; y++) {
					for (int32 x = 0; x < width; x++) {
						int32 offset = (y * rowLength) + (x * 4);

						rgb_color pixelColor = make_color(iconBits[offset],
							iconBits[offset + 1], iconBits[offset + 2],
							iconBits[offset + 3]);

						pixelColor = tint_color(pixelColor,
							B_DARKEN_2_TINT);

						iconBits[offset] = pixelColor.red;
						iconBits[offset + 1] = pixelColor.green;
						iconBits[offset + 2] = pixelColor.blue;
						iconBits[offset + 3] = pixelColor.alpha;
					}
				}
			}

			fAnimationWorker = spawn_thread(&_AnimateWaitIconWorker,
				"thumb twiddling", B_DISPLAY_PRIORITY, this);

			if (fAnimationWorker < B_NO_ERROR)
				return;

			atomic_set(&fAnimationActive, true);
			if (resume_thread(fAnimationWorker) != B_OK)
				atomic_set(&fAnimationActive, false);
		} else {
			atomic_set(&fAnimationActive, false);
			wait_for_thread(fAnimationWorker, NULL);

			fCurrentAnimationRow = -1;
			fAnimationLightenPhase = true;

			if (fCurrentIconBitmap != NULL && fNormalIconBitmap != NULL)
				fCurrentIconBitmap->ImportBits(fNormalIconBitmap);
		}
	}

private:
	status_t _AnimateWaitIcon()
	{
		int32 lastHeight = 1;
		while (atomic_get(&fAnimationActive)) {
			if (LockWithTimeout(kIconAnimateInterval / lastHeight) != B_OK)
				continue;

			lastHeight = fCurrentIconBitmap->Bounds().IntegerHeight();
			if (fCurrentAnimationRow < 0) {
				fCurrentAnimationRow = lastHeight;
				fAnimationLightenPhase = !fAnimationLightenPhase;
			}

			BBitmap* sourceBitmap = fAnimationLightenPhase ?
				fNormalIconBitmap : fTintedIconBitmap;

			fCurrentIconBitmap->ImportBits(sourceBitmap,
				BPoint(0, fCurrentAnimationRow),
				BPoint(0, fCurrentAnimationRow),
				BSize(sourceBitmap->Bounds().IntegerWidth() - 1, 0));

			fCurrentAnimationRow--;

			ChildAt(0)->Invalidate();

			Unlock();
			snooze(kIconAnimateInterval / lastHeight);
		}

		return B_OK;
	}

	static status_t _AnimateWaitIconWorker(void* cookie)
	{
		ShutdownWindow* ourView = (ShutdownWindow*)cookie;
		return ourView->_AnimateWaitIcon();
	}

private:
	BList				fAppInfos;
	BButton*			fKillAppButton;
	BButton*			fCancelShutdownButton;
	BButton*			fRebootSystemButton;
	BButton*			fAbortedOKButton;
	BMessage*			fKillAppMessage;
	team_id				fCurrentApp;

private:
	BBitmap*		fCurrentIconBitmap;
	BBitmap*		fNormalIconBitmap;
	BBitmap*		fTintedIconBitmap;
	int32			fAnimationActive;
	thread_id		fAnimationWorker;
	int32			fCurrentAnimationRow;
	bool			fAnimationLightenPhase;
};


// #pragma mark -


ShutdownProcess::ShutdownProcess(TRoster* roster, EventQueue* eventQueue)
	:
	BLooper("shutdown process"),
	EventMaskWatcher(BMessenger(this), B_REQUEST_QUIT | B_REQUEST_LAUNCHED),
	fWorkerLock("worker lock"),
	fRequest(NULL),
	fRoster(roster),
	fEventQueue(eventQueue),
	fTimeoutEvent(NULL),
	fInternalEvents(NULL),
	fInternalEventSemaphore(-1),
	fQuitRequestReplyHandler(NULL),
	fWorker(-1),
	fCurrentPhase(INVALID_PHASE),
	fShutdownError(B_ERROR),
	fHasGUI(false),
	fReboot(false),
	fRequestReplySent(false),
	fWindow(NULL)
{
}


ShutdownProcess::~ShutdownProcess()
{
	// terminate the GUI
	if (fHasGUI && fWindow && fWindow->Lock())
		fWindow->Quit();

	// remove and delete the quit request reply handler
	if (fQuitRequestReplyHandler) {
		BAutolock _(this);
		RemoveHandler(fQuitRequestReplyHandler);
		delete fQuitRequestReplyHandler;
	}

	// remove and delete the timeout event
	if (fTimeoutEvent) {
		fEventQueue->RemoveEvent(fTimeoutEvent);

		delete fTimeoutEvent;
	}

	// remove the application quit watcher
	fRoster->RemoveWatcher(this);

	// If an error occurred (e.g. the shutdown process was cancelled), the
	// roster should accept applications again.
	if (fShutdownError != B_OK)
		fRoster->SetShuttingDown(false);

	// delete the internal event semaphore
	if (fInternalEventSemaphore >= 0)
		delete_sem(fInternalEventSemaphore);

	// wait for the worker thread to terminate
	if (fWorker >= 0) {
		int32 result;
		wait_for_thread(fWorker, &result);
	}

	// delete all internal events and the queue
	if (fInternalEvents) {
		while (InternalEvent* event = fInternalEvents->First()) {
			fInternalEvents->Remove(event);
			delete event;
		}

		delete fInternalEvents;
	}

	// send a reply to the request and delete it
	_SendReply(fShutdownError);
	delete fRequest;
}


status_t
ShutdownProcess::Init(BMessage* request)
{
	PRINT("ShutdownProcess::Init()\n");

	// create and add the quit request reply handler
	fQuitRequestReplyHandler = new(nothrow) QuitRequestReplyHandler(this);
	if (!fQuitRequestReplyHandler)
		RETURN_ERROR(B_NO_MEMORY);
	AddHandler(fQuitRequestReplyHandler);

	// create the timeout event
	fTimeoutEvent = new(nothrow) TimeoutEvent(this);
	if (!fTimeoutEvent)
		RETURN_ERROR(B_NO_MEMORY);

	// create the event list
	fInternalEvents = new(nothrow) InternalEventList;
	if (!fInternalEvents)
		RETURN_ERROR(B_NO_MEMORY);

	// create the event sempahore
	fInternalEventSemaphore = create_sem(0, "shutdown events");
	if (fInternalEventSemaphore < 0)
		RETURN_ERROR(fInternalEventSemaphore);

	// init the app server connection
	fHasGUI = Registrar::App()->InitGUIContext() == B_OK;

	// start watching application quits
	status_t error = fRoster->AddWatcher(this);
	if (error != B_OK) {
		fRoster->SetShuttingDown(false);
		RETURN_ERROR(error);
	}

	// start the worker thread
	fWorker = spawn_thread(_WorkerEntry, "shutdown worker",
		B_NORMAL_PRIORITY + 1, this);
	if (fWorker < 0) {
		fRoster->RemoveWatcher(this);
		fRoster->SetShuttingDown(false);
		RETURN_ERROR(fWorker);
	}

	// everything went fine: now we own the request
	fRequest = request;

	if (fRequest->FindBool("reboot", &fReboot) != B_OK)
		fReboot = false;

	resume_thread(fWorker);

	PRINT("ShutdownProcess::Init() done\n");

	return B_OK;
}


void
ShutdownProcess::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SOME_APP_QUIT:
		{
			// get the team
			team_id team;
			if (message->FindInt32("be:team", &team) != B_OK) {
				// should not happen
				return;
			}

			PRINT("ShutdownProcess::MessageReceived(): B_SOME_APP_QUIT: %"
				B_PRId32 "\n", team);

			// remove the app info from the respective list
			int32 phase;
			RosterAppInfo* info;
			{
				BAutolock _(fWorkerLock);

				info = fUserApps.InfoFor(team);
				if (info)
					fUserApps.RemoveInfo(info);
				else if ((info = fSystemApps.InfoFor(team)))
					fSystemApps.RemoveInfo(info);
				else if ((info = fBackgroundApps.InfoFor(team)))
					fBackgroundApps.RemoveInfo(info);
				else	// not found
					return;

				phase = fCurrentPhase;
			}

			// post the event
			_PushEvent(APP_QUIT_EVENT, team, phase);

			delete info;

			break;
		}

		case B_SOME_APP_LAUNCHED:
		{
			// get the team
			team_id team;
			if (message->FindInt32("be:team", &team) != B_OK) {
				// should not happen
				return;
			}

			PRINT("ShutdownProcess::MessageReceived(): B_SOME_APP_LAUNCHED: %"
				B_PRId32 "\n", team);

			// add the user app info to the respective list
			{
				BAutolock _(fWorkerLock);
				fRoster->AddAppInfo(fUserApps, team);
			}
			break;
		}

		case MSG_PHASE_TIMED_OUT:
		{
			// get the phase the event is intended for
			int32 phase = TimeoutEvent::GetMessagePhase(message);
			team_id team = TimeoutEvent::GetMessageTeam(message);;
			PRINT("MSG_PHASE_TIMED_OUT: phase: %" B_PRId32 ", team: %" B_PRId32
				"\n", phase, team);

			BAutolock _(fWorkerLock);

			if (phase == INVALID_PHASE || phase != fCurrentPhase)
				return;

			// post the event
			_PushEvent(TIMEOUT_EVENT, team, phase);

			break;
		}

		case MSG_KILL_APPLICATION:
		{
			team_id team;
			if (message->FindInt32("team", &team) != B_OK)
				break;

			// post the event
			_PushEvent(KILL_APP_EVENT, team, fCurrentPhase);
			break;
		}

		case MSG_CANCEL_SHUTDOWN:
		{
			// post the event
			_PushEvent(ABORT_EVENT, -1, fCurrentPhase);
			break;
		}

		case MSG_REBOOT_SYSTEM:
		{
			// post the event
			_PushEvent(REBOOT_SYSTEM_EVENT, -1, INVALID_PHASE);
			break;
		}

		case MSG_DONE:
		{
			// notify the registrar that we're done
			be_app->PostMessage(B_REG_SHUTDOWN_FINISHED, be_app);
			break;
		}

		case B_REG_TEAM_DEBUGGER_ALERT:
		{
			bool stopShutdown;
			if (message->FindBool("stop shutdown", &stopShutdown) == B_OK
				&& stopShutdown) {
				// post abort event to the worker
				_PushEvent(ABORT_EVENT, -1, fCurrentPhase);
				break;
			}

			bool open;
			team_id team;
			if (message->FindInt32("team", &team) != B_OK
				|| message->FindBool("open", &open) != B_OK)
				break;

			BAutolock _(fWorkerLock);
			if (open) {
				PRINT("B_REG_TEAM_DEBUGGER_ALERT: insert %" B_PRId32 "\n",
					team);
				fDebuggedTeams.Add(team);
			} else {
				PRINT("B_REG_TEAM_DEBUGGER_ALERT: remove %" B_PRId32 "\n",
					team);
				fDebuggedTeams.Remove(team);
				_PushEvent(DEBUG_EVENT, -1, fCurrentPhase);
			}
			break;
		}

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


void
ShutdownProcess::SendReply(BMessage* request, status_t error)
{
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}
}


void
ShutdownProcess::_SendReply(status_t error)
{
	if (!fRequestReplySent) {
		SendReply(fRequest, error);
		fRequestReplySent = true;
	}
}


void
ShutdownProcess::_SetPhase(int32 phase)
{
	BAutolock _(fWorkerLock);

	if (phase == fCurrentPhase)
		return;

	fCurrentPhase = phase;

	// remove the timeout event scheduled for the previous phase
	fEventQueue->RemoveEvent(fTimeoutEvent);
}


void
ShutdownProcess::_ScheduleTimeoutEvent(bigtime_t timeout, team_id team)
{
	BAutolock _(fWorkerLock);

	// remove the timeout event
	fEventQueue->RemoveEvent(fTimeoutEvent);

	// set the event's phase, team and time
	fTimeoutEvent->SetPhase(fCurrentPhase);
	fTimeoutEvent->SetTeam(team);
	fTimeoutEvent->SetTime(system_time() + timeout);

	// add the event
	fEventQueue->AddEvent(fTimeoutEvent);
}


void
ShutdownProcess::_SetShowShutdownWindow(bool show)
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		if (show == fWindow->IsHidden()) {
			if (show)
				fWindow->Go(NULL);
			else
				fWindow->Hide();
		}
	}
}


void
ShutdownProcess::_InitShutdownWindow()
{
	// prepare the window
	if (fHasGUI) {
		fWindow = new(nothrow) ShutdownWindow;
		if (fWindow != NULL) {
			status_t error = fWindow->Init(BMessenger(this));
			if (error != B_OK) {
				delete fWindow;
				fWindow = NULL;
			}
		}

		// add the applications
		if (fWindow) {
			BAutolock _(fWorkerLock);
			_AddShutdownWindowApps(fUserApps);
			_AddShutdownWindowApps(fSystemApps);
		} else {
			WARNING("ShutdownProcess::Init(): Failed to create or init "
				"shutdown window.");

			fHasGUI = false;
		}
	}
}


void
ShutdownProcess::_AddShutdownWindowApps(AppInfoList& infos)
{
	if (!fHasGUI)
		return;

	for (AppInfoList::Iterator it = infos.It(); it.IsValid(); ++it) {
		RosterAppInfo* info = *it;

		// init an app file info
		BFile file;
		status_t error = file.SetTo(&info->ref, B_READ_ONLY);
		if (error != B_OK) {
			WARNING("ShutdownProcess::_AddShutdownWindowApps(): Failed to "
				"open file for app %s: %s\n", info->signature,
				strerror(error));
			continue;
		}

		BAppFileInfo appFileInfo;
		error = appFileInfo.SetTo(&file);
		if (error != B_OK) {
			WARNING("ShutdownProcess::_AddShutdownWindowApps(): Failed to "
				"init app file info for app %s: %s\n", info->signature,
				strerror(error));
		}

		// get the application icon
		BBitmap* appIcon = new(nothrow) BBitmap(
			BRect(BPoint(0, 0), be_control_look->ComposeIconSize(B_LARGE_ICON)),
			B_BITMAP_NO_SERVER_LINK, B_RGBA32);
		if (appIcon != NULL) {
			error = appIcon->InitCheck();
			if (error == B_OK) {
				error = appFileInfo.GetTrackerIcon(appIcon,
					(icon_size)(appIcon->Bounds().IntegerWidth() + 1));
			}
			if (error != B_OK) {
				delete appIcon;
				appIcon = NULL;
			}
		}

		// add the app
		error = fWindow->AddApp(info->team, appIcon);
		if (error != B_OK) {
			WARNING("ShutdownProcess::_AddShutdownWindowApps(): Failed to "
				"add app to the shutdown window: %s\n", strerror(error));
		}
	}
}


void
ShutdownProcess::_RemoveShutdownWindowApp(team_id team)
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->RemoveApp(team);
	}
}


void
ShutdownProcess::_SetShutdownWindowCurrentApp(team_id team)
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->SetCurrentApp(team);
	}
}


void
ShutdownProcess::_SetShutdownWindowText(const char* text)
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->SetText(text);
	}
}


void
ShutdownProcess::_SetShutdownWindowCancelButtonEnabled(bool enabled)
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->SetCancelShutdownButtonEnabled(enabled);
	}
}


void
ShutdownProcess::_SetShutdownWindowKillButtonEnabled(bool enabled)
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->SetKillAppButtonEnabled(enabled);
	}
}


void
ShutdownProcess::_SetShutdownWindowWaitAnimationEnabled(bool enabled)
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->SetWaitAnimationEnabled(enabled);
	}
}


void
ShutdownProcess::_SetShutdownWindowWaitForShutdown()
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->SetWaitForShutdown();
	}
}


void
ShutdownProcess::_SetShutdownWindowWaitForAbortedOK()
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->SetWaitForAbortedOK();
	}
}


void
ShutdownProcess::_NegativeQuitRequestReply(thread_id thread)
{
	BAutolock _(fWorkerLock);

	// Note: team ID == team main thread ID under Haiku. When testing under R5
	// using the team ID in case of an ABORT_EVENT won't work correctly. But
	// this is done only for system apps.
	_PushEvent(ABORT_EVENT, thread, fCurrentPhase);
}


void
ShutdownProcess::_PrepareShutdownMessage(BMessage& message) const
{
	message.what = B_QUIT_REQUESTED;
	message.AddBool("_shutdown_", true);

	BMessage::Private(message).SetReply(BMessenger(fQuitRequestReplyHandler));
}


status_t
ShutdownProcess::_ShutDown()
{
	PRINT("Invoking _kern_shutdown(%d)\n", fReboot);
	RETURN_ERROR(_kern_shutdown(fReboot));
}


status_t
ShutdownProcess::_PushEvent(uint32 eventType, team_id team, int32 phase)
{
	InternalEvent* event = new(nothrow) InternalEvent(eventType, team, phase);
	if (!event) {
		ERROR("ShutdownProcess::_PushEvent(): Failed to create event!\n");

		return B_NO_MEMORY;
	}

	BAutolock _(fWorkerLock);

	fInternalEvents->Add(event);
	release_sem(fInternalEventSemaphore);

	return B_OK;
}


status_t
ShutdownProcess::_GetNextEvent(uint32& eventType, thread_id& team, int32& phase,
	bool block)
{
	while (true) {
		// acquire the semaphore
		if (block) {
			status_t error;
			do {
				error = acquire_sem(fInternalEventSemaphore);
			} while (error == B_INTERRUPTED);

			if (error != B_OK)
				return error;
		} else {
			status_t error = acquire_sem_etc(fInternalEventSemaphore, 1,
				B_RELATIVE_TIMEOUT, 0);
			if (error != B_OK) {
				eventType = NO_EVENT;
				return B_OK;
			}
		}

		// get the event
		BAutolock _(fWorkerLock);

		InternalEvent* event = fInternalEvents->Head();
		fInternalEvents->Remove(event);

		eventType = event->Type();
		team = event->Team();
		phase = event->Phase();

		delete event;

		// if the event is an obsolete timeout event, we drop it right here
		if (eventType == TIMEOUT_EVENT && phase != fCurrentPhase)
			continue;

		break;
	}

	// notify the window, if an app has been removed
	if (eventType == APP_QUIT_EVENT)
		_RemoveShutdownWindowApp(team);

	return B_OK;
}


status_t
ShutdownProcess::_WorkerEntry(void* data)
{
	return ((ShutdownProcess*)data)->_Worker();
}


status_t
ShutdownProcess::_Worker()
{
	try {
		_WorkerDoShutdown();
		fShutdownError = B_OK;
	} catch (status_t error) {
		PRINT("ShutdownProcess::_Worker(): error while shutting down: %s\n",
			strerror(error));

		fShutdownError = error;
	}

	// this can happen only, if the shutdown process failed or was aborted:
	// notify the looper
	_SetPhase(DONE_PHASE);
	PostMessage(MSG_DONE);

	return B_OK;
}


void
ShutdownProcess::_WorkerDoShutdown()
{
	PRINT("ShutdownProcess::_WorkerDoShutdown()\n");

	// If we are here, the shutdown process has been initiated successfully,
	// that is, if an asynchronous BRoster::Shutdown() was requested, we
	// notify the caller at this point.
	bool synchronous;
	if (fRequest->FindBool("synchronous", &synchronous) == B_OK && !synchronous)
		_SendReply(B_OK);

	// ask the user to confirm the shutdown, if desired
	bool askUser;
	if (fHasGUI && fRequest->FindBool("confirm", &askUser) == B_OK && askUser) {
		const char* restart = B_TRANSLATE("Restart");
		const char* shutdown = B_TRANSLATE("Shut down");
		BString title = B_TRANSLATE("%action%?");
		title.ReplaceFirst("%action%", fReboot ? restart : shutdown);
		const char* text = fReboot
			? B_TRANSLATE("Do you really want to restart the system?")
			: B_TRANSLATE("Do you really want to shut down the system?");
		const char* defaultText = fReboot ? restart : shutdown;
		const char* otherText = fReboot ? shutdown : restart;
		BAlert* alert = new BAlert(title.String(), text,
			B_TRANSLATE("Cancel"), otherText, defaultText,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		// We want the alert to behave more like a regular window...
		alert->SetFeel(B_NORMAL_WINDOW_FEEL);
		// ...but not quit. Minimizing the alert would prevent the user from
		// finding it again, since registrar does not have an entry in the
		// Deskbar.
		alert->SetFlags(alert->Flags() | B_NOT_MINIMIZABLE | B_CLOSE_ON_ESCAPE);
		alert->SetWorkspaces(B_ALL_WORKSPACES);
		int32 result = alert->Go();

		if (result == 1) {
			// Toggle shutdown method
			fReboot = !fReboot;
		} else if (result < 1)
			throw_error(B_SHUTDOWN_CANCELLED);
	}

	fWorkerLock.Lock();
	// get a list of all applications to shut down and sort them
	status_t status = fRoster->GetShutdownApps(fUserApps, fSystemApps,
		fBackgroundApps, fVitalSystemApps);
	if (status  != B_OK) {
		fWorkerLock.Unlock();
		fRoster->RemoveWatcher(this);
		return;
	}

	fUserApps.Sort(&inverse_compare_by_registration_time);
	fSystemApps.Sort(&inverse_compare_by_registration_time);

	fWorkerLock.Unlock();

	// make the shutdown window ready and show it
	_InitShutdownWindow();
	_SetShutdownWindowCurrentApp(-1);
	_SetShutdownWindowText(B_TRANSLATE("Tidying things up a bit."));
	_SetShutdownWindowCancelButtonEnabled(true);
	_SetShutdownWindowKillButtonEnabled(false);
	_SetShowShutdownWindow(true);

	// sync
	sync();

	// phase 1: terminate the user apps
	_SetPhase(USER_APP_TERMINATION_PHASE);

	// since, new apps can still be launched, loop until all are gone
	if (!fUserApps.IsEmpty()) {
		_QuitApps(fUserApps, false);
		_WaitForDebuggedTeams();
	}

	// tell TRoster not to accept new applications anymore
	fRoster->SetShuttingDown(true);

	// phase 2: terminate the system apps
	_SetPhase(SYSTEM_APP_TERMINATION_PHASE);
	_QuitApps(fSystemApps, true);
	_WaitForDebuggedTeams();

	// phase 3: terminate the background apps
	_SetPhase(BACKGROUND_APP_TERMINATION_PHASE);
	_QuitBackgroundApps();
	_WaitForDebuggedTeams();

	// phase 4: terminate the other processes
	_SetPhase(OTHER_PROCESSES_TERMINATION_PHASE);
	_QuitNonApps();
	_ScheduleTimeoutEvent(kBackgroundAppQuitTimeout, -1);
	_WaitForBackgroundApps();
	_KillBackgroundApps();
	_WaitForDebuggedTeams();

	// we're through: do the shutdown
	_SetPhase(DONE_PHASE);
	if (fReboot)
		_SetShutdownWindowText(B_TRANSLATE("Restarting" B_UTF8_ELLIPSIS));
	else
		_SetShutdownWindowText(B_TRANSLATE("Shutting down" B_UTF8_ELLIPSIS));
	_ShutDown();
	_SetShutdownWindowWaitForShutdown();

	PRINT("  _kern_shutdown() failed\n");

	// shutdown failed: This can happen for power off mode -- reboot should
	// always work.
	if (fHasGUI) {
		// wait for the reboot event
		uint32 event;
		do {
			team_id team;
			int32 phase;
			status = _GetNextEvent(event, team, phase, true);
			if (status != B_OK)
				break;
		} while (event != REBOOT_SYSTEM_EVENT);

		_kern_shutdown(true);
	}

	// either there's no GUI or reboot failed: we enter the kernel debugger
	// instead
	while (true) {
		_kern_kernel_debugger("The system is shut down. It's now safe to turn "
			"off the computer.");
	}
}


bool
ShutdownProcess::_WaitForApp(team_id team, AppInfoList* list, bool systemApps)
{
	uint32 event;
	do {
		team_id eventTeam;
		int32 phase;
		status_t error = _GetNextEvent(event, eventTeam, phase, true);
		if (error != B_OK)
			throw_error(error);

		if (event == APP_QUIT_EVENT && eventTeam == team)
			return true;

		if (event == TIMEOUT_EVENT && eventTeam == team)
			return false;

		if (event == ABORT_EVENT) {
			if (eventTeam == -1) {
				// The user canceled the shutdown process by pressing the
				// Cancel button.
				throw_error(B_SHUTDOWN_CANCELLED);
			}
			if (systemApps) {
				// If the app requests aborting the shutdown, we don't need
				// to wait any longer. It has processed the request and
				// won't quit by itself. We ignore this for system apps.
				if (eventTeam == team)
					return false;
			} else {
				// The app returned false in QuitRequested().
				PRINT("ShutdownProcess::_WaitForApp(): shutdown cancelled "
					"by team %" B_PRId32 " (-1 => user)\n", eventTeam);

				_DisplayAbortingApp(team);
				throw_error(B_SHUTDOWN_CANCELLED);
			}
		}

		BAutolock _(fWorkerLock);
		if (list != NULL && !list->InfoFor(team))
			return true;
	} while (event != NO_EVENT);

	return false;
}


void
ShutdownProcess::_QuitApps(AppInfoList& list, bool systemApps)
{
	PRINT("ShutdownProcess::_QuitApps(%s)\n",
		(systemApps ? "system" : "user"));

	if (systemApps) {
		_SetShutdownWindowCancelButtonEnabled(false);

		// check one last time for abort events
		uint32 event;
		do {
			team_id team;
			int32 phase;
			status_t error = _GetNextEvent(event, team, phase, false);
			if (error != B_OK)
				throw_error(error);

			if (event == ABORT_EVENT) {
				PRINT("ShutdownProcess::_QuitApps(): shutdown cancelled by "
					"team %" B_PRId32 " (-1 => user)\n", team);

				_DisplayAbortingApp(team);
				throw_error(B_SHUTDOWN_CANCELLED);
			}

		} while (event != NO_EVENT);
	}

	// prepare the shutdown message
	BMessage message;
	_PrepareShutdownMessage(message);

	// now iterate through the list of apps
	while (true) {
		// eat events
		uint32 event;
		do {
			team_id team;
			int32 phase;
			status_t error = _GetNextEvent(event, team, phase, false);
			if (error != B_OK)
				throw_error(error);

			if (!systemApps && event == ABORT_EVENT) {
				PRINT("ShutdownProcess::_QuitApps(): shutdown cancelled by "
					"team %" B_PRId32 " (-1 => user)\n", team);

				_DisplayAbortingApp(team);
				throw_error(B_SHUTDOWN_CANCELLED);
			}

		} while (event != NO_EVENT);

		// get the first app to quit
		team_id team = -1;
		port_id port = -1;
		char appName[B_FILE_NAME_LENGTH];
		{
			BAutolock _(fWorkerLock);
			while (!list.IsEmpty()) {
				RosterAppInfo* info = *list.It();
				team = info->team;
				port = info->port;
				strcpy(appName, info->ref.name);

				if (info->IsRunning())
					break;
				list.RemoveInfo(info);
				delete info;
			}
		}

		if (team < 0) {
			PRINT("ShutdownProcess::_QuitApps() done\n");
			return;
		}

		// set window text
		BString buffer = B_TRANSLATE("Asking \"%appName%\" to quit.");
		buffer.ReplaceFirst("%appName%", appName);
		_SetShutdownWindowText(buffer.String());
		_SetShutdownWindowCurrentApp(team);

		// send the shutdown message to the app
		PRINT("  sending team %" B_PRId32 " (port: %" B_PRId32 ") a shutdown "
			"message\n", team, port);
		SingleMessagingTargetSet target(port, B_PREFERRED_TOKEN);
		MessageDeliverer::Default()->DeliverMessage(&message, target);

		// schedule a timeout event
		_ScheduleTimeoutEvent(kAppQuitTimeout, team);

		// wait for the app to die or for the timeout to occur
		bool appGone = _WaitForApp(team, &list, systemApps);
		if (appGone) {
			// fine: the app finished in an orderly manner
		} else {
			// the app is either blocking on a model alert or blocks for another
			// reason
			if (!systemApps)
				_QuitBlockingApp(list, team, appName, true);
			else {
				// This is a system app: remove it from the list
				BAutolock _(fWorkerLock);

				if (RosterAppInfo* info = list.InfoFor(team)) {
					list.RemoveInfo(info);
					delete info;
				}
			}
		}
	}
}


void
ShutdownProcess::_QuitBackgroundApps()
{
	PRINT("ShutdownProcess::_QuitBackgroundApps()\n");

	_SetShutdownWindowText(
		B_TRANSLATE("Asking background applications to quit."));

	// prepare the shutdown message
	BMessage message;
	_PrepareShutdownMessage(message);

	// send shutdown messages to user apps
	BAutolock _(fWorkerLock);

	AppInfoListMessagingTargetSet targetSet(fBackgroundApps);

	if (targetSet.HasNext()) {
		PRINT("  sending shutdown message to %" B_PRId32 " apps\n",
			fBackgroundApps.CountInfos());

		status_t error = MessageDeliverer::Default()->DeliverMessage(
			&message, targetSet);
		if (error != B_OK) {
			WARNING("_QuitBackgroundApps::_Worker(): Failed to deliver "
				"shutdown message to all applications: %s\n",
				strerror(error));
		}
	}

	PRINT("ShutdownProcess::_QuitBackgroundApps() done\n");
}


void
ShutdownProcess::_WaitForBackgroundApps()
{
	PRINT("ShutdownProcess::_WaitForBackgroundApps()\n");

	// wait for user apps
	bool moreApps = true;
	while (moreApps) {
		{
			BAutolock _(fWorkerLock);
			moreApps = !fBackgroundApps.IsEmpty();
		}

		if (moreApps) {
			uint32 event;
			team_id team;
			int32 phase;
			status_t error = _GetNextEvent(event, team, phase, true);
			if (error != B_OK)
				throw_error(error);

			if (event == ABORT_EVENT) {
				// ignore: it's too late to abort the shutdown
			}

			if (event == TIMEOUT_EVENT)
				return;
		}
	}

	PRINT("ShutdownProcess::_WaitForBackgroundApps() done\n");
}


void
ShutdownProcess::_KillBackgroundApps()
{
	PRINT("ShutdownProcess::_KillBackgroundApps()\n");

	while (true) {
		// eat events (we need to be responsive for an abort event)
		uint32 event;
		do {
			team_id team;
			int32 phase;
			status_t error = _GetNextEvent(event, team, phase, false);
			if (error != B_OK)
				throw_error(error);

		} while (event != NO_EVENT);

		// get the first team to kill
		team_id team = -1;
		char appName[B_FILE_NAME_LENGTH];
		AppInfoList& list = fBackgroundApps;
		{
			BAutolock _(fWorkerLock);

			if (!list.IsEmpty()) {
				RosterAppInfo* info = *list.It();
				team = info->team;
				strcpy(appName, info->ref.name);
			}
		}


		if (team < 0) {
			PRINT("ShutdownProcess::_KillBackgroundApps() done\n");
			return;
		}

		// the app is either blocking on a model alert or blocks for another
		// reason
		_QuitBlockingApp(list, team, appName, false);
	}
}


void
ShutdownProcess::_QuitNonApps()
{
	PRINT("ShutdownProcess::_QuitNonApps()\n");

	_SetShutdownWindowText(B_TRANSLATE("Asking other processes to quit."));

	// iterate through the remaining teams and send them the TERM signal
	int32 cookie = 0;
	team_info teamInfo;
	while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
		if (!fVitalSystemApps.Contains(teamInfo.team)) {
			PRINT("  sending team %" B_PRId32 " TERM signal\n", teamInfo.team);

			// Note: team ID == team main thread ID under Haiku
			send_signal(teamInfo.team, SIGTERM);
		}
	}

	// give them a bit of time to terminate
	// TODO: Instead of just waiting we could periodically check whether the
	// processes are already gone to shorten the process.
	snooze(kNonAppQuitTimeout);

	// iterate through the remaining teams and kill them
	cookie = 0;
	while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
		if (!fVitalSystemApps.Contains(teamInfo.team)) {
			PRINT("  killing team %" B_PRId32 "\n", teamInfo.team);

			kill_team(teamInfo.team);
		}
	}

	PRINT("ShutdownProcess::_QuitNonApps() done\n");
}


void
ShutdownProcess::_QuitBlockingApp(AppInfoList& list, team_id team,
	const char* appName, bool cancelAllowed)
{
	bool debugged = false;
	bool modal = false;
	{
		BAutolock _(fWorkerLock);
		if (fDebuggedTeams.Contains(team))
			debugged = true;
	}
	if (!debugged)
		modal = BPrivate::is_app_showing_modal_window(team);

	if (modal) {
		// app blocks on a modal window
		BString buffer = B_TRANSLATE("The application \"%appName%\" might be "
			"blocked on a modal panel.");
		buffer.ReplaceFirst("%appName%", appName);
		_SetShutdownWindowText(buffer.String());
		_SetShutdownWindowCurrentApp(team);
		_SetShutdownWindowKillButtonEnabled(true);
		_SetShutdownWindowWaitAnimationEnabled(true);
	}

	if (modal || debugged) {
		// wait for something to happen
		bool appGone = false;
		while (true) {
			uint32 event;
			team_id eventTeam;
			int32 phase;
			status_t error = _GetNextEvent(event, eventTeam, phase, true);
			if (error != B_OK)
				throw_error(error);

			if ((event == APP_QUIT_EVENT) && eventTeam == team) {
				appGone = true;
				break;
			}

			if (event == KILL_APP_EVENT && eventTeam == team)
				break;

			if (event == ABORT_EVENT) {
				if (cancelAllowed || debugged) {
					PRINT("ShutdownProcess::_QuitBlockingApp(): shutdown "
						"cancelled by team %" B_PRId32 " (-1 => user)\n",
						eventTeam);

					if (!debugged)
						_DisplayAbortingApp(eventTeam);
					throw_error(B_SHUTDOWN_CANCELLED);
				}

				// If the app requests aborting the shutdown, we don't need
				// to wait any longer. It has processed the request and
				// won't quit by itself. We'll have to kill it.
				if (eventTeam == team)
					break;
			}
		}

		_SetShutdownWindowKillButtonEnabled(false);
		_SetShutdownWindowWaitAnimationEnabled(false);

		if (appGone)
			return;
	}

	// kill the app
	PRINT("  killing team %" B_PRId32 "\n", team);

	kill_team(team);

	// remove the app (the roster will note eventually and send us
	// a notification, but we want to be sure)
	{
		BAutolock _(fWorkerLock);

		if (RosterAppInfo* info = list.InfoFor(team)) {
			list.RemoveInfo(info);
			delete info;
		}
	}
}


void
ShutdownProcess::_DisplayAbortingApp(team_id team)
{
	// find the app that cancelled the shutdown
	char appName[B_FILE_NAME_LENGTH];
	bool foundApp = false;
	{
		BAutolock _(fWorkerLock);

		RosterAppInfo* info = fUserApps.InfoFor(team);
		if (!info)
			info = fSystemApps.InfoFor(team);
		if (!info)
			fBackgroundApps.InfoFor(team);

		if (info) {
			foundApp = true;
			strcpy(appName, info->ref.name);
		}
	}

	if (!foundApp) {
		PRINT("ShutdownProcess::_DisplayAbortingApp(): Didn't find the app "
			"that has cancelled the shutdown.\n");
		return;
	}

	// compose the text to be displayed
	BString buffer = B_TRANSLATE("Application \"%appName%\" has aborted the "
		"shutdown process.");
	buffer.ReplaceFirst("%appName%", appName);

	// set up the window
	_SetShutdownWindowWaitAnimationEnabled(false);
	_SetShutdownWindowCurrentApp(team);
	_SetShutdownWindowText(buffer.String());
	_SetShutdownWindowWaitForAbortedOK();

	// schedule the timeout event
	_SetPhase(ABORTED_PHASE);
	_ScheduleTimeoutEvent(kDisplayAbortingAppTimeout);

	// wait for the timeout or the user to press the cancel button
	while (true) {
		uint32 event;
		team_id eventTeam;
		int32 phase;
		status_t error = _GetNextEvent(event, eventTeam, phase, true);
		if (error != B_OK)
			break;

		// stop waiting when the timeout occurs
		if (event == TIMEOUT_EVENT)
			break;

		// stop waiting when the user hit the cancel button
		if (event == ABORT_EVENT && phase == ABORTED_PHASE && eventTeam < 0)
			break;
	}
}


/*!	Waits until the debugged team list is empty, ie. when there is no one
	left to debug.
*/
void
ShutdownProcess::_WaitForDebuggedTeams()
{
	PRINT("ShutdownProcess::_WaitForDebuggedTeams()\n");
	{
		BAutolock _(fWorkerLock);
		if (fDebuggedTeams.Size() == 0)
			return;
	}

	PRINT("  not empty!\n");

	// wait for something to happen
	while (true) {
		uint32 event;
		team_id eventTeam;
		int32 phase;
		status_t error = _GetNextEvent(event, eventTeam, phase, true);
		if (error != B_OK)
			throw_error(error);

		if (event == ABORT_EVENT)
			throw_error(B_SHUTDOWN_CANCELLED);

		BAutolock _(fWorkerLock);
		if (fDebuggedTeams.Size() == 0) {
			PRINT("  out empty");
			return;
		}
	}
}

