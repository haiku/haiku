/*
 * Copyright 2005-2006, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <new>

#include <signal.h>
#include <unistd.h>

#include <Alert.h>
#include <AppFileInfo.h>
#include <AppMisc.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Button.h>
#include <File.h>
#include <Message.h>
#include <MessagePrivate.h>
#include <RegistrarDefs.h>
#include <Roster.h>		// for B_REQUEST_QUIT
#include <Screen.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>

#include <TokenSpace.h>
#include <util/DoublyLinkedList.h>

#ifdef __HAIKU__
#include <syscalls.h>
#endif

#include "AppInfoListMessagingTargetSet.h"
#include "Debug.h"
#include "EventQueue.h"
#include "InputServerTypes.h"
#include "MessageDeliverer.h"
#include "MessageEvent.h"
#include "Registrar.h"
#include "RosterAppInfo.h"
#include "ShutdownProcess.h"
#include "TRoster.h"

using std::nothrow;
using namespace BPrivate;

// The time span a non-background application has after the quit message has
// been delivered (more precisely: has been handed over to the
// MessageDeliverer).
static const bigtime_t kAppQuitTimeout = 3000000; // 3 s

// The time span a background application has after the quit message has been
// delivered (more precisely: has been handed over to the MessageDeliverer).
static const bigtime_t kBackgroundAppQuitTimeout = 3000000; // 3 s

// The time span non-app processes have after the HUP signal has been send
// to them before they get a KILL signal.
static const bigtime_t kNonAppQuitTimeout = 500000; // 0.5 s

// The time span the app that has aborted the shutdown shall be displayed in
// the shutdown window before closing it automatically.
static const bigtime_t kDisplayAbortingAppTimeout = 3000000; // 3 s


// message what fields
enum {
	MSG_PHASE_TIMED_OUT		= 'phto',
	MSG_DONE				= 'done',
	MSG_KILL_APPLICATION	= 'kill',
	MSG_CANCEL_SHUTDOWN		= 'cncl',
	MSG_REBOOT_SYSTEM		= 'rbot',
};

// internal events
enum {
	NO_EVENT,
	ABORT_EVENT,
	TIMEOUT_EVENT,
	APP_QUIT_EVENT,
	KILL_APP_EVENT,
	REBOOT_SYSTEM_EVENT,
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


// inverse_compare_by_registration_time
static
int
inverse_compare_by_registration_time(const RosterAppInfo *info1,
	const RosterAppInfo *info2)
{
	bigtime_t cmp = info1->registration_time - info2->registration_time;
	if (cmp < 0)
		return 1;
	return (cmp > 0 ? -1 : 0);
}

// throw_error
/*!	\brief Used to avoid type matching problems when throwing a constant.
*/
static inline
void
throw_error(status_t error)
{
	throw error;
}

// TimeoutEvent
class ShutdownProcess::TimeoutEvent : public MessageEvent {
public:
	TimeoutEvent(BHandler *target)
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

	static int32 GetMessagePhase(BMessage *message)
	{
		int32 phase;
		if (message->FindInt32("phase", &phase) != B_OK)
			phase = INVALID_PHASE;

		return phase;
	}

	static int32 GetMessageTeam(BMessage *message)
	{
		team_id team;
		if (message->FindInt32("team", &team) != B_OK)
			team = -1;

		return team;
	}
};


// InternalEvent
class ShutdownProcess::InternalEvent
	: public DoublyLinkedListLinkImpl<InternalEvent> {
public:
	InternalEvent(uint32 type, team_id team, int32 phase)
		: fType(type),
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


// InternalEventList
struct ShutdownProcess::InternalEventList : DoublyLinkedList<InternalEvent> {
};


// QuitRequestReplyHandler
class ShutdownProcess::QuitRequestReplyHandler : public BHandler {
public:
	QuitRequestReplyHandler(ShutdownProcess *shutdownProcess)
		: BHandler("shutdown quit reply handler"),
		  fShutdownProcess(shutdownProcess)
	{
	}

	virtual void MessageReceived(BMessage *message)
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


// ShutdownWindow
class ShutdownProcess::ShutdownWindow : public BWindow {
public:
	ShutdownWindow()
		: BWindow(BRect(0, 0, 200, 100), "Shutdown Status",
			B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_MINIMIZABLE
				| B_NOT_ZOOMABLE | B_NOT_CLOSABLE, B_ALL_WORKSPACES),
		  fKillAppMessage(NULL),
		  fCurrentApp(-1)
	{
	}

	~ShutdownWindow()
	{
		for (int32 i = 0; AppInfo *info = (AppInfo*)fAppInfos.ItemAt(i); i++)
			delete info;
	}

	status_t Init(BMessenger target)
	{
		// create the views

		// root view
		BView *rootView = new(nothrow) BView(BRect(0, 0, 100,  15), "app icons",
			B_FOLLOW_NONE, 0);
		if (!rootView)
			return B_NO_MEMORY;
		rootView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		AddChild(rootView);

		// current app icon view
		fCurrentAppIconView = new(nothrow) CurrentAppIconView;
		if (!fCurrentAppIconView)
			return B_NO_MEMORY;
		rootView->AddChild(fCurrentAppIconView);

		// text view
		fTextView = new(nothrow) BTextView(BRect(0, 0, 10, 10), "text",
			BRect(0, 0, 10, 10), B_FOLLOW_NONE);
		if (!fTextView)
			return B_NO_MEMORY;
		fTextView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		fTextView->MakeEditable(false);
		fTextView->MakeSelectable(false);
		fTextView->SetWordWrap(true);
		rootView->AddChild(fTextView);

		// kill app button
		fKillAppButton = new(nothrow) BButton(BRect(0, 0, 10, 10), "kill app",
			"Kill Application", NULL, B_FOLLOW_NONE);
		if (!fKillAppButton)
			return B_NO_MEMORY;
		rootView->AddChild(fKillAppButton);

		BMessage *message = new BMessage(MSG_KILL_APPLICATION);
		if (!message)
			return B_NO_MEMORY;
		message->AddInt32("team", -1);
		fKillAppMessage = message;
		fKillAppButton->SetMessage(message);
		fKillAppButton->SetTarget(target);

		// cancel shutdown button
		fCancelShutdownButton = new(nothrow) BButton(BRect(0, 0, 10, 10),
			"cancel shutdown", "Cancel Shutdown", NULL, B_FOLLOW_NONE);
		if (!fCancelShutdownButton)
			return B_NO_MEMORY;
		rootView->AddChild(fCancelShutdownButton);

		message = new BMessage(MSG_CANCEL_SHUTDOWN);
		if (!message)
			return B_NO_MEMORY;
		fCancelShutdownButton->SetMessage(message);
		fCancelShutdownButton->SetTarget(target);

		// reboot system button
		fRebootSystemButton = new(nothrow) BButton(BRect(0, 0, 10, 10),
			"reboot", "Restart System", NULL, B_FOLLOW_NONE);
		if (!fRebootSystemButton)
			return B_NO_MEMORY;
		fRebootSystemButton->Hide();
		rootView->AddChild(fRebootSystemButton);

		message = new BMessage(MSG_REBOOT_SYSTEM);
		if (!message)
			return B_NO_MEMORY;
		fRebootSystemButton->SetMessage(message);
		fRebootSystemButton->SetTarget(target);

		// aborted OK button
		fAbortedOKButton = new(nothrow) BButton(BRect(0, 0, 10, 10),
			"ok", "OK", NULL, B_FOLLOW_NONE);
		if (!fAbortedOKButton)
			return B_NO_MEMORY;
		fAbortedOKButton->Hide();
		rootView->AddChild(fAbortedOKButton);

		message = new BMessage(MSG_CANCEL_SHUTDOWN);
		if (!message)
			return B_NO_MEMORY;
		fAbortedOKButton->SetMessage(message);
		fAbortedOKButton->SetTarget(target);

		// compute the sizes
		static const int kHSpacing = 10;
		static const int kVSpacing = 10;
		static const int kInnerHSpacing = 5;
		static const int kInnerVSpacing = 8;

		// buttons
		fKillAppButton->ResizeToPreferred();
		fCancelShutdownButton->ResizeToPreferred();
		fRebootSystemButton->MakeDefault(true);
		fRebootSystemButton->ResizeToPreferred();
		fAbortedOKButton->MakeDefault(true);
		fAbortedOKButton->ResizeToPreferred();

		BRect rect(fKillAppButton->Frame());
		int buttonWidth = rect.IntegerWidth() + 1;
		int buttonHeight = rect.IntegerHeight() + 1;

		rect = fCancelShutdownButton->Frame();
		if (rect.IntegerWidth() >= buttonWidth)
			buttonWidth = rect.IntegerWidth() + 1;

		int defaultButtonHeight
			= fRebootSystemButton->Frame().IntegerHeight() + 1;

		// text view
		fTextView->SetText("two\nlines");
		int textHeight = (int)fTextView->TextHeight(0, 1) + 1;

		// current app icon view
		int currentAppIconWidth = fCurrentAppIconView->Frame().IntegerWidth()
			+ 1;
		int currentAppIconHeight = fCurrentAppIconView->Frame().IntegerHeight()
			+ 1;

		int currentAppIconX = kHSpacing;
		int rightPartX = currentAppIconX + currentAppIconWidth;
		int textX = rightPartX + kInnerHSpacing;
		int textY = kVSpacing;
		int buttonsY = textY + textHeight + kInnerVSpacing;
		int nonDefaultButtonsY = buttonsY
			+ (defaultButtonHeight - buttonHeight) / 2;
		int rightPartWidth = 2 * buttonWidth + kInnerHSpacing;
		int width = rightPartX + rightPartWidth + kHSpacing;
		int height = buttonsY + defaultButtonHeight + kVSpacing;

		// now layout the views

		// current app icon view
		fCurrentAppIconView->MoveTo(currentAppIconX,
			textY + (textHeight - currentAppIconHeight) / 2);

		// text view
		fTextView->MoveTo(textX, textY);
		fTextView->ResizeTo(rightPartWidth + rightPartX - textX - 1,
			textHeight - 1);
		fTextView->SetTextRect(fTextView->Bounds());

		// buttons
		fKillAppButton->MoveTo(rightPartX, nonDefaultButtonsY);
		fKillAppButton->ResizeTo(buttonWidth - 1, buttonHeight - 1);

		fCancelShutdownButton->MoveTo(
			rightPartX + buttonWidth + kInnerVSpacing - 1,
			nonDefaultButtonsY);
		fCancelShutdownButton->ResizeTo(buttonWidth - 1, buttonHeight - 1);

		fRebootSystemButton->MoveTo(
			(width - fRebootSystemButton->Frame().IntegerWidth()) / 2,
			buttonsY);

		fAbortedOKButton->MoveTo(
			(width - fAbortedOKButton->Frame().IntegerWidth()) / 2,
			buttonsY);

		// set the root view and window size
		rootView->ResizeTo(width - 1, height - 1);
		ResizeTo(width - 1, height - 1);

		// move the window to the same position as BAlerts
		BScreen screen(this);
	 	BRect screenFrame = screen.Frame();

		MoveTo(screenFrame.left + (screenFrame.Width() - width) / 2.0,
			screenFrame.top + screenFrame.Height() / 4.0 - ceilf(height / 3.0));

		return B_OK;
	}

	status_t AddApp(team_id team, BBitmap *miniIcon, BBitmap *largeIcon)
	{
		AppInfo *info = new(nothrow) AppInfo;
		if (!info) {
			delete miniIcon;
			delete largeIcon;
			return B_NO_MEMORY;
		}

		info->team = team;
		info->miniIcon = miniIcon;
		info->largeIcon = largeIcon;

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

		AppInfo *info = (AppInfo*)fAppInfos.RemoveItem(index);
		delete info;
	}

	void SetCurrentApp(team_id team)
	{
		AppInfo *info = (team >= 0 ? _AppInfoFor(team) : NULL);

		fCurrentApp = team;
		fCurrentAppIconView->SetAppInfo(info);

		fKillAppMessage->ReplaceInt32("team", team);
	}

	void SetText(const char *text)
	{
		fTextView->SetText(text);
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
		}
	}

	void SetWaitForShutdown()
	{
		fCurrentAppIconView->Hide();
		fKillAppButton->Hide();
		fCancelShutdownButton->Hide();
		fRebootSystemButton->MakeDefault(true);
		fRebootSystemButton->Show();

		SetTitle("System is Shut Down");
		fTextView->SetText("It's now safe to turn off the computer.");
	}

	void SetWaitForAbortedOK()
	{
		fCurrentAppIconView->Hide();
		fKillAppButton->Hide();
		fCancelShutdownButton->Hide();
		fAbortedOKButton->MakeDefault(true);
		fAbortedOKButton->Show();
		// TODO: Temporary work-around for a Haiku bug.
		fAbortedOKButton->Invalidate();

		SetTitle("Shutdown Aborted");
	}

private:
	struct AppInfo {
		team_id		team;
		BBitmap		*miniIcon;
		BBitmap		*largeIcon;

		~AppInfo()
		{
			delete miniIcon;
			delete largeIcon;
		}
	};

	int32 _AppInfoIndexOf(team_id team)
	{
		if (team < 0)
			return -1;

		for (int32 i = 0; AppInfo *info = (AppInfo*)fAppInfos.ItemAt(i); i++) {
			if (info->team == team)
				return i;
		}

		return -1;
	}

	AppInfo *_AppInfoFor(team_id team)
	{
		int32 index = _AppInfoIndexOf(team);
		return (index >= 0 ? (AppInfo*)fAppInfos.ItemAt(index) : NULL);
	}

	class CurrentAppIconView : public BView {
	  public:
		CurrentAppIconView()
			: BView(BRect(0, 0, 31,  31), "current app icon", B_FOLLOW_NONE, 
				B_WILL_DRAW),
			  fAppInfo(NULL)
		{
			SetViewColor(B_TRANSPARENT_32_BIT);
			fBackground = ui_color(B_PANEL_BACKGROUND_COLOR);
		}

		virtual void Draw(BRect updateRect)
		{
			SetDrawingMode(B_OP_COPY);
			SetLowColor(fBackground);
			FillRect(Bounds(), B_SOLID_LOW);

			if (fAppInfo && fAppInfo->largeIcon) {
				if (fAppInfo->largeIcon->ColorSpace() == B_RGBA32) {
					SetDrawingMode(B_OP_ALPHA);
					SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
				} else
					SetDrawingMode(B_OP_OVER);
			
				DrawBitmap(fAppInfo->largeIcon, BPoint(0, 0));
			}
		}

		void SetAppInfo(AppInfo *info)
		{
			fAppInfo = info;
			Invalidate();
		}

	  private:
		const AppInfo	*fAppInfo;
		rgb_color		fBackground;
	};

private:
	BList				fAppInfos;
	CurrentAppIconView	*fCurrentAppIconView;
	BTextView			*fTextView;
	BButton				*fKillAppButton;
	BButton				*fCancelShutdownButton;
	BButton				*fRebootSystemButton;
	BButton				*fAbortedOKButton;
	BMessage			*fKillAppMessage;
	team_id				fCurrentApp;
};


// #pragma mark -

// constructor
ShutdownProcess::ShutdownProcess(TRoster *roster, EventQueue *eventQueue)
	: BLooper("shutdown process"),
	  EventMaskWatcher(BMessenger(this), B_REQUEST_QUIT),
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

// destructor
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
		while (InternalEvent *event = fInternalEvents->First()) {
			fInternalEvents->Remove(event);
			delete event;
		}

		delete fInternalEvents;
	}

	// send a reply to the request and delete it
	_SendReply(fShutdownError);
	delete fRequest;
}

// Init
status_t
ShutdownProcess::Init(BMessage *request)
{
	PRINT(("ShutdownProcess::Init()\n"));

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
	fHasGUI = (Registrar::App()->InitGUIContext() == B_OK);

	// tell TRoster not to accept new applications anymore
	fRoster->SetShuttingDown(true);

	// start watching application quits
	status_t error = fRoster->AddWatcher(this);
	if (error != B_OK) {
		fRoster->SetShuttingDown(false);
		RETURN_ERROR(error);
	}

	// get a list of all applications to shut down and sort them
	error = fRoster->GetShutdownApps(fUserApps, fSystemApps, fBackgroundApps,
		fVitalSystemApps);//, fInputServer);
	if (error != B_OK) {
		fRoster->RemoveWatcher(this);
		fRoster->SetShuttingDown(false);
		RETURN_ERROR(error);
	}

	fUserApps.Sort(&inverse_compare_by_registration_time);
	fSystemApps.Sort(&inverse_compare_by_registration_time);

	// start the worker thread
	fWorker = spawn_thread(_WorkerEntry, "shutdown worker", B_NORMAL_PRIORITY,
		this);
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

	PRINT(("ShutdownProcess::Init() done\n"));

	return B_OK;
}

// MessageReceived
void
ShutdownProcess::MessageReceived(BMessage *message)
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

			PRINT(("ShutdownProcess::MessageReceived(): B_SOME_APP_QUIT: %ld\n",
				team));

			// remove the app info from the respective list
			int32 phase;
			RosterAppInfo *info;
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

		case MSG_PHASE_TIMED_OUT:
		{
			// get the phase the event is intended for
			int32 phase = TimeoutEvent::GetMessagePhase(message);
			team_id team = TimeoutEvent::GetMessageTeam(message);;
PRINT(("MSG_PHASE_TIMED_OUT: phase: %ld, team: %ld\n", phase, team));

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

			BAutolock _(fWorkerLock);

			// post the event
			_PushEvent(KILL_APP_EVENT, team, fCurrentPhase);

			break;
		}

		case MSG_CANCEL_SHUTDOWN:
		{
			BAutolock _(fWorkerLock);

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

		default:
			BLooper::MessageReceived(message);
			break;
	}
}

// SendReply
void
ShutdownProcess::SendReply(BMessage *request, status_t error)
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

// _SendReply
void
ShutdownProcess::_SendReply(status_t error)
{
	if (!fRequestReplySent) {
		SendReply(fRequest, error);
		fRequestReplySent = true;
	}
}

// _SetPhase
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

// _ScheduleTimeoutEvent
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

// _SetShowShutdownWindow
void
ShutdownProcess::_SetShowShutdownWindow(bool show)
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		if (show == fWindow->IsHidden()) {
			if (show)
				fWindow->Show();
			else
				fWindow->Hide();
		}
	}
}

// _InitShutdownWindow
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
			WARNING(("ShutdownProcess::Init(): Failed to create or init "
				"shutdown window."));

			fHasGUI = false;
		}
	}
}

// _AddShutdownWindowApps
void
ShutdownProcess::_AddShutdownWindowApps(AppInfoList &infos)
{
	if (!fHasGUI)
		return;

	for (AppInfoList::Iterator it = infos.It(); it.IsValid(); ++it) {
		RosterAppInfo *info = *it;

		// init an app file info
		BFile file;
		status_t error = file.SetTo(&info->ref, B_READ_ONLY);
		if (error != B_OK) {
			WARNING(("ShutdownProcess::_AddShutdownWindowApps(): Failed to "
				"open file for app %s: %s\n", info->signature,
				strerror(error)));
			continue;
		}

		BAppFileInfo appFileInfo;
		error = appFileInfo.SetTo(&file);
		if (error != B_OK) {
			WARNING(("ShutdownProcess::_AddShutdownWindowApps(): Failed to "
				"init app file info for app %s: %s\n", info->signature,
				strerror(error)));
		}

		// get the application icons
#ifdef __HAIKU__
		color_space format = B_RGBA32;
#else
		color_space format = B_CMAP8;
#endif

		// mini icon
		BBitmap *miniIcon = new(nothrow) BBitmap(BRect(0, 0, 15, 15), format);
		if (miniIcon != NULL) {
			error = miniIcon->InitCheck();
			if (error == B_OK)
				error = appFileInfo.GetTrackerIcon(miniIcon, B_MINI_ICON);
			if (error != B_OK) {
				delete miniIcon;
				miniIcon = NULL;
			}
		}

		// mini icon
		BBitmap *largeIcon = new(nothrow) BBitmap(BRect(0, 0, 31, 31), format);
		if (largeIcon != NULL) {
			error = largeIcon->InitCheck();
			if (error == B_OK)
				error = appFileInfo.GetTrackerIcon(largeIcon, B_LARGE_ICON);
			if (error != B_OK) {
				delete largeIcon;
				largeIcon = NULL;
			}
		}

		// add the app
		error = fWindow->AddApp(info->team, miniIcon, largeIcon);
		if (error != B_OK) {
			WARNING(("ShutdownProcess::_AddShutdownWindowApps(): Failed to "
				"add app to the shutdown window: %s\n", strerror(error)));
		}
	}
}

// _RemoveShutdownWindowApp
void
ShutdownProcess::_RemoveShutdownWindowApp(team_id team)
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->RemoveApp(team);
	}
}

// _SetShutdownWindowCurrentApp
void
ShutdownProcess::_SetShutdownWindowCurrentApp(team_id team)
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->SetCurrentApp(team);
	}
}

// _SetShutdownWindowText
void
ShutdownProcess::_SetShutdownWindowText(const char *text)
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->SetText(text);
	}
}

// _SetShutdownWindowCancelButtonEnabled
void
ShutdownProcess::_SetShutdownWindowCancelButtonEnabled(bool enabled)
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->SetCancelShutdownButtonEnabled(enabled);
	}
}

// _SetShutdownWindowKillButtonEnabled
void
ShutdownProcess::_SetShutdownWindowKillButtonEnabled(bool enabled)
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->SetKillAppButtonEnabled(enabled);
	}
}

// _SetShutdownWindowWaitForShutdown
void
ShutdownProcess::_SetShutdownWindowWaitForShutdown()
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->SetWaitForShutdown();
	}
}

// _SetShutdownWindowWaitForAbortedOK
void
ShutdownProcess::_SetShutdownWindowWaitForAbortedOK()
{
	if (fHasGUI) {
		BAutolock _(fWindow);

		fWindow->SetWaitForAbortedOK();
	}
}

// _NegativeQuitRequestReply
void
ShutdownProcess::_NegativeQuitRequestReply(thread_id thread)
{
	BAutolock _(fWorkerLock);

	// Note: team ID == team main thread ID under Haiku. When testing under R5
	// using the team ID in case of an ABORT_EVENT won't work correctly. But
	// this is done only for system apps.
	_PushEvent(ABORT_EVENT, thread, fCurrentPhase);
}

// _PrepareShutdownMessage
void
ShutdownProcess::_PrepareShutdownMessage(BMessage &message) const
{
	message.what = B_QUIT_REQUESTED;
	message.AddBool("_shutdown_", true);

	BMessage::Private(message).SetReply(BMessenger(fQuitRequestReplyHandler));
}

// _ShutDown
status_t
ShutdownProcess::_ShutDown()
{
	PRINT(("Invoking _kern_shutdown(%d)\n", fReboot));

	#ifdef __HAIKU__
		RETURN_ERROR(_kern_shutdown(fReboot));
	#else
		// we can't do anything on R5
		return B_ERROR;
	#endif
}


// _PushEvent
status_t
ShutdownProcess::_PushEvent(uint32 eventType, team_id team, int32 phase)
{
	InternalEvent *event = new(nothrow) InternalEvent(eventType, team, phase);
	if (!event) {
		ERROR(("ShutdownProcess::_PushEvent(): Failed to create event!\n"));

		return B_NO_MEMORY;
	}

	BAutolock _(fWorkerLock);

	fInternalEvents->Add(event);
	release_sem(fInternalEventSemaphore);

	return B_OK;
}

// _GetNextEvent
status_t
ShutdownProcess::_GetNextEvent(uint32 &eventType, thread_id &team, int32 &phase,
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
	
		InternalEvent *event = fInternalEvents->Head();
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

// _WorkerEntry
status_t
ShutdownProcess::_WorkerEntry(void *data)
{
	return ((ShutdownProcess*)data)->_Worker();
}

// _Worker
status_t
ShutdownProcess::_Worker()
{
	try {
		_WorkerDoShutdown();
		fShutdownError = B_OK;
	} catch (status_t error) {
		PRINT(("ShutdownProcess::_Worker(): error while shutting down: %s\n",
			strerror(error)));

		fShutdownError = error;
	}

	// this can happen only, if the shutdown process failed or was aborted:
	// notify the looper
	_SetPhase(DONE_PHASE);
	PostMessage(MSG_DONE);

	return B_OK;
}

// _WorkerDoShutdown
void
ShutdownProcess::_WorkerDoShutdown()
{
	PRINT(("ShutdownProcess::_WorkerDoShutdown()\n"));

	// If we are here, the shutdown process has been initiated successfully,
	// that is, if an asynchronous BRoster::Shutdown() was requested, we
	// notify the caller at this point.
	bool synchronous;
	if (fRequest->FindBool("synchronous", &synchronous) == B_OK
		&& !synchronous) {
		_SendReply(B_OK);
	}

	// ask the user to confirm the shutdown, if desired
	bool askUser;
	if (fHasGUI && fRequest->FindBool("confirm", &askUser) == B_OK && askUser) {
		const char *title = (fReboot ? "Restart?" : "Shut Down?");
		const char *text = (fReboot
			? "Do you really want to restart the system?"
			: "Do you really want to shut down the system?");
		const char *buttonText = (fReboot ? "Restart" : "Shut Down");
		BAlert *alert = new BAlert(title, text, "Cancel", buttonText, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		int32 result = alert->Go();

		if (result != 1)
			throw_error(B_SHUTDOWN_CANCELLED);
	}

	// make the shutdown window ready and show it
	_InitShutdownWindow();
	_SetShutdownWindowCurrentApp(-1);
	_SetShutdownWindowText("Tidying things up a bit.");
	_SetShutdownWindowCancelButtonEnabled(true);
	_SetShutdownWindowKillButtonEnabled(false);
	_SetShowShutdownWindow(true);

	// sync
	sync();

	// notify the input server we are shutting down
	/*if (fInputServer.registration_time != 0) {
		// The SYSTEM_SHUTTING_DOWN message is defined in InputServerTypes.h
		BMessage message(SYSTEM_SHUTTING_DOWN);
		SingleMessagingTargetSet target(fInputServer.port, B_PREFERRED_TOKEN);
		MessageDeliverer::Default()->DeliverMessage(&message, target);
	}*/
	
	// phase 1: terminate the user apps
	_SetPhase(USER_APP_TERMINATION_PHASE);
	_QuitApps(fUserApps, false);

	// phase 2: terminate the system apps
	_SetPhase(SYSTEM_APP_TERMINATION_PHASE);
	_QuitApps(fSystemApps, true);

	// phase 3: terminate the background apps
	_SetPhase(BACKGROUND_APP_TERMINATION_PHASE);
// TODO: _QuitNonApps() and _QuitBackgroundApps() are called in reverse?
// and don't match the phase
	_QuitNonApps();

	// phase 4: terminate the other processes
	_SetPhase(OTHER_PROCESSES_TERMINATION_PHASE);
	_QuitBackgroundApps();
	_ScheduleTimeoutEvent(kBackgroundAppQuitTimeout, -1);
	_WaitForBackgroundApps();
	_KillBackgroundApps();

	// we're through: do the shutdown
	_SetPhase(DONE_PHASE);
	_ShutDown();
	_SetShutdownWindowWaitForShutdown();

	PRINT(("  _kern_shutdown() failed\n"));

	// shutdown failed: This can happen for power off mode -- reboot should
	// always work.
	if (fHasGUI) {
		// wait for the reboot event
		uint32 event;
		do {
			team_id team;
			int32 phase;
			status_t error = _GetNextEvent(event, team, phase, true);
			if (error != B_OK)
				break;
		} while (event != REBOOT_SYSTEM_EVENT);

		#ifdef __HAIKU__
		_kern_shutdown(true);
		#endif
	}

	// either there's no GUI or reboot failed: we enter the kernel debugger
	// instead
	#ifdef __HAIKU__
// TODO: Introduce the syscall.
//	while (true) {
//		_kern_kernel_debugger("The system is shut down. It's now safe to turn "
//			"off the computer.");
//	}
	#endif
}

// _QuitApps
void
ShutdownProcess::_QuitApps(AppInfoList &list, bool disableCancel)
{
	PRINT(("ShutdownProcess::_QuitApps(%s)\n",
		(disableCancel ? "system" : "user")));

	if (disableCancel) {
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
				PRINT(("ShutdownProcess::_QuitApps(): shutdown cancelled by "
					"team %ld (-1 => user)\n", team));

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

			if (!disableCancel && event == ABORT_EVENT) {
				PRINT(("ShutdownProcess::_QuitApps(): shutdown cancelled by "
					"team %ld (-1 => user)\n", team));

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
			if (!list.IsEmpty()) {
				RosterAppInfo *info = *list.It();
				team = info->team;
				port = info->port;
				strcpy(appName, info->ref.name);
			}
		}

		if (team < 0) {
			PRINT(("ShutdownProcess::_QuitApps() done\n"));
			return;
		}

		PRINT(("****RJL: Asking %s to quit\n", appName));

		// set window text
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "Asking \"%s\" to quit.", appName);
		_SetShutdownWindowText(buffer);
		_SetShutdownWindowCurrentApp(team);

		// send the shutdown message to the app
		PRINT(("  sending team %ld (port: %ld) a shutdown message\n", team,
			port));
		SingleMessagingTargetSet target(port, B_PREFERRED_TOKEN);
		MessageDeliverer::Default()->DeliverMessage(&message, target);

		// schedule a timeout event
		_ScheduleTimeoutEvent(kAppQuitTimeout, team);

		// wait for the app to die or for the timeout to occur
		bool appGone = false;
		do {
			team_id eventTeam;
			int32 phase;
			status_t error = _GetNextEvent(event, eventTeam, phase, true);
			if (error != B_OK)
				throw_error(error);

			if ((event == APP_QUIT_EVENT)
				&& eventTeam == team) {
				appGone = true;
			}

			if (event == TIMEOUT_EVENT && eventTeam == team)
				break;

			if (event == ABORT_EVENT) {
				if (disableCancel) {
					// If the app requests aborting the shutdown, we don't need
					// to wait any longer. It has processed the request and
					// won't quit by itself. We'll have to kill it.
					if (eventTeam == team)
						break;
				} else {
					PRINT(("ShutdownProcess::_QuitApps(): shutdown cancelled "
						"by team %ld (-1 => user)\n", eventTeam));

					_DisplayAbortingApp(team);
					throw_error(B_SHUTDOWN_CANCELLED);
				}
			}

			BAutolock _(fWorkerLock);
			if (!list.InfoFor(team))
				break;

		} while (event != NO_EVENT);

		if (appGone) {
			// fine: the app finished in an orderly manner
		} else {
			// the app is either blocking on a model alert or blocks for another
			// reason
			_QuitBlockingApp(list, team, appName, !disableCancel);
		}
	}
}

// _QuitBackgroundApps
void
ShutdownProcess::_QuitBackgroundApps()
{
	PRINT(("ShutdownProcess::_QuitBackgroundApps()\n"));

	_SetShutdownWindowText("Asking background applications to quit.");

	// prepare the shutdown message
	BMessage message;
	_PrepareShutdownMessage(message);

	// send shutdown messages to user apps
	BAutolock _(fWorkerLock);

	AppInfoListMessagingTargetSet targetSet(fBackgroundApps);

	if (targetSet.HasNext()) {
		PRINT(("  sending shutdown message to %ld apps\n",
			fBackgroundApps.CountInfos()));

		status_t error = MessageDeliverer::Default()->DeliverMessage(
			&message, targetSet);
		if (error != B_OK) {
			WARNING(("_QuitBackgroundApps::_Worker(): Failed to deliver "
				"shutdown message to all applications: %s\n",
				strerror(error)));
		}
	}

	PRINT(("ShutdownProcess::_QuitBackgroundApps() done\n"));
}

// _WaitForBackgroundApps
void
ShutdownProcess::_WaitForBackgroundApps()
{
	PRINT(("ShutdownProcess::_WaitForBackgroundApps()\n"));

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

	PRINT(("ShutdownProcess::_WaitForBackgroundApps() done\n"));
}

// _KillBackgroundApps
void
ShutdownProcess::_KillBackgroundApps()
{
	PRINT(("ShutdownProcess::_KillBackgroundApps()\n"));

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
		AppInfoList &list = fBackgroundApps;
		{
			BAutolock _(fWorkerLock);

			if (!list.IsEmpty()) {
				RosterAppInfo *info = *list.It();
				team = info->team;
				strcpy(appName, info->ref.name);
			}
		}


		if (team < 0) {
			PRINT(("ShutdownProcess::_KillBackgroundApps() done\n"));
			return;
		}

		// the app is either blocking on a model alert or blocks for another
		// reason
		_QuitBlockingApp(list, team, appName, false);
	}
}

// _QuitNonApps
void
ShutdownProcess::_QuitNonApps()
{
	PRINT(("ShutdownProcess::_QuitNonApps()\n"));

	_SetShutdownWindowText("Asking other processes to quit.");

	// iterate through the remaining teams and send them the HUP signal
	int32 cookie = 0;
	team_info teamInfo;
	while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
		if (fVitalSystemApps.find(teamInfo.team) == fVitalSystemApps.end()) {
			PRINT(("  sending team %ld HUP signal\n", teamInfo.team));

			#ifdef __HAIKU__
				// Note: team ID == team main thread ID under Haiku
				send_signal(teamInfo.team, SIGHUP);
			#else
				// We don't want to do this when testing under R5, since it
				// would kill all teams besides our app server and registrar.
			#endif
		}
	}

	// give them a bit of time to terminate
	// TODO: Instead of just waiting we could periodically check whether the
	// processes are already gone to shorten the process.
	snooze(kNonAppQuitTimeout);

	// iterate through the remaining teams and kill them
	cookie = 0;
	while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
		if (fVitalSystemApps.find(teamInfo.team) == fVitalSystemApps.end()) {
			PRINT(("  killing team %ld\n", teamInfo.team));

			#ifdef __HAIKU__
				kill_team(teamInfo.team);
			#else
				// We don't want to do this when testing under R5, since it
				// would kill all teams besides our app server and registrar.
			#endif
		}
	}

	PRINT(("ShutdownProcess::_QuitNonApps() done\n"));
}

// _QuitBlockingApp
void
ShutdownProcess::_QuitBlockingApp(AppInfoList &list, team_id team,
	const char *appName, bool cancelAllowed)
{
	if (BPrivate::is_app_showing_modal_window(team)) {
		// app blocks on a modal window
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "The application \"%s\" might be "
			"blocked on a modal panel.", appName);
		_SetShutdownWindowText(buffer);
		_SetShutdownWindowCurrentApp(team);
		_SetShutdownWindowKillButtonEnabled(true);

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
				if (cancelAllowed) {
					PRINT(("ShutdownProcess::_QuitBlockingApp(): shutdown "
						"cancelled by team %ld (-1 => user)\n", eventTeam));

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

		if (appGone)
			return;
	}

	// kill the app
	PRINT(("  killing team %ld\n", team));

	kill_team(team);

	// remove the app (the roster will note eventually and send us
	// a notification, but we want to be sure)
	{
		BAutolock _(fWorkerLock);

		if (RosterAppInfo *info = list.InfoFor(team)) {
			list.RemoveInfo(info);
			delete info;
		}
	}
}

// _DisplayAbortingApp
void
ShutdownProcess::_DisplayAbortingApp(team_id team)
{
	// find the app that cancelled the shutdown
	char appName[B_FILE_NAME_LENGTH];
	bool foundApp = false;
	{
		BAutolock _(fWorkerLock);

		RosterAppInfo *info = fUserApps.InfoFor(team);
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
		PRINT(("ShutdownProcess::_DisplayAbortingApp(): Didn't find the app "
			"that has cancelled the shutdown.\n"));
		return;
	}

	// compose the text to be displayed
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "Application \"%s\" has aborted the shutdown "
		"process.", appName);

	// set up the window
	_SetShutdownWindowCurrentApp(team);
	_SetShutdownWindowText(buffer);
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

// This doesn't give us anything; it will just prevent us to see which
// app was responsible after all...
#if 0
		// also stop when the responsible app quit
		if ((event == APP_QUIT_EVENT) && eventTeam == team)
			break;
#endif
	}
}

