/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

// TODO: While debugging only. Remove when implementation is done.
#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include <new>

#include <signal.h>
#include <unistd.h>

#include <AppFileInfo.h>
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
#include "MessageDeliverer.h"
#include "MessageEvent.h"
#include "Registrar.h"
#include "RosterAppInfo.h"
#include "ShutdownProcess.h"
#include "TRoster.h"

using namespace BPrivate;

// The time span user applications have after the quit message has been
// delivered (more precisely: has been handed over to the MessageDeliverer).
static const bigtime_t USER_APPS_QUIT_TIMEOUT = 5000000; // 5 s

// The time span system applications have after the quit message has been
// delivered (more precisely: has been handed over to the MessageDeliverer).
static const bigtime_t SYSTEM_APPS_QUIT_TIMEOUT = 3000000; // 3 s

// The time span non-app processes have after the HUP signal has been send
// to them before they get a KILL signal.
static const bigtime_t NON_APPS_QUIT_TIMEOUT = 500000; // 0.5 s


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
	USER_APP_QUIT_EVENT,
	SYSTEM_APP_QUIT_EVENT,
	KILL_APP_EVENT,
	REBOOT_SYSTEM_EVENT,
};

// phases
enum {
	INVALID_PHASE						= -1,
	USER_APP_TERMINATION_PHASE			= 0,
	SYSTEM_APP_TERMINATION_PHASE		= 1,
	OTHER_PROCESSES_TERMINATION_PHASE	= 2,
	DONE_PHASE							= 3,
	ABORTED_PHASE						= 4,
};

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
		  fKillAppMessage(NULL)
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

		// app icons view
		fAppIconsView = new(nothrow) AppIconsView(fAppInfos);
		if (!fAppIconsView)
			return B_NO_MEMORY;
		rootView->AddChild(fAppIconsView);

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
			"reboot", "Reboot System", NULL, B_FOLLOW_NONE);
		if (!fRebootSystemButton)
			return B_NO_MEMORY;
		fRebootSystemButton->Hide();
		rootView->AddChild(fRebootSystemButton);

		message = new BMessage(MSG_REBOOT_SYSTEM);
		if (!message)
			return B_NO_MEMORY;
		fRebootSystemButton->SetMessage(message);
		fRebootSystemButton->SetTarget(target);

		// compute the sizes
		static const int kHSpacing = 10;
		static const int kVSpacing = 10;
		static const int kInnerHSpacing = 5;
		static const int kInnerVSpacing = 5;

		// buttons
		fKillAppButton->ResizeToPreferred();
		fCancelShutdownButton->ResizeToPreferred();
		fRebootSystemButton->ResizeToPreferred();

		BRect rect(fKillAppButton->Frame());
		int buttonWidth = rect.IntegerWidth() + 1;
		int buttonHeight = rect.IntegerHeight() + 1;

		rect = fCancelShutdownButton->Frame();
		if (rect.IntegerWidth() >= buttonWidth)
			buttonWidth = rect.IntegerWidth() + 1;

		// text view
		fTextView->SetText("two\nlines");
		int textHeight = (int)fTextView->TextHeight(0, 2) + 1;

		// app icons view
		int appIconsHeight = fAppIconsView->Frame().IntegerHeight() + 1;

		// current app icon view
		int currentAppIconWidth = fCurrentAppIconView->Frame().IntegerWidth()
			+ 1;
		int currentAppIconHeight = fCurrentAppIconView->Frame().IntegerHeight()
			+ 1;

		int currentAppIconX = kHSpacing;
		int rightPartX = currentAppIconX + currentAppIconWidth + kInnerHSpacing;
		int appIconsY = kVSpacing;
		int textY = appIconsY + appIconsHeight + kInnerVSpacing;
		int buttonsY = textY + textHeight + kInnerVSpacing;
		int rightPartWidth = 2 * buttonWidth + kInnerHSpacing;
		int width = rightPartX + rightPartWidth + kHSpacing;
		int height = buttonsY + buttonHeight + kVSpacing;

		// now layout the views

		// current app icon view
		fCurrentAppIconView->MoveTo(currentAppIconX,
			textY + (textHeight - currentAppIconHeight) / 2);

		// app icons view
		fAppIconsView->MoveTo(rightPartX, kVSpacing);
		fAppIconsView->ResizeTo(rightPartWidth - 1, appIconsHeight - 1);

		// text view
		fTextView->MoveTo(rightPartX, textY);
		fTextView->ResizeTo(rightPartWidth - 1, textHeight - 1);
		fTextView->SetTextRect(fTextView->Bounds());

		// buttons
		fKillAppButton->MoveTo(rightPartX, buttonsY);
		fKillAppButton->ResizeTo(buttonWidth - 1, buttonHeight - 1);

		fCancelShutdownButton->MoveTo(
			rightPartX + buttonWidth + kInnerVSpacing - 1,
			buttonsY);
		fCancelShutdownButton->ResizeTo(buttonWidth - 1, buttonHeight - 1);

		fRebootSystemButton->MoveTo(
			(width - fRebootSystemButton->Frame().IntegerWidth()) / 2,
			buttonsY);

		// set the root view and window size
		rootView->ResizeTo(width - 1, height - 1);
		ResizeTo(width - 1, height - 1);

		// center the window on screen
		BScreen screen;
		if (screen.IsValid()) {
			BRect screenFrame = screen.Frame();
			int screenWidth = screenFrame.IntegerWidth() + 1;
			int screenHeight = screenFrame.IntegerHeight() + 1;
			MoveTo((screenWidth - width) / 2, (screenHeight - height) / 2);
		} else {
			MoveTo(20, 20);
		}

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

		fAppIconsView->Invalidate();

		return B_OK;
	}

	void RemoveApp(team_id team)
	{
		int32 index = _AppInfoIndexOf(team);
		if (index < 0)
			return;

		AppInfo *info = (AppInfo*)fAppInfos.RemoveItem(index);
		delete info;

		fAppIconsView->Invalidate();
	}

	void SetCurrentApp(team_id team)
	{
		AppInfo *info = (team >= 0 ? _AppInfoFor(team) : NULL);

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
		fAppIconsView->Hide();
		fCurrentAppIconView->Hide();
		fKillAppButton->Hide();
		fCancelShutdownButton->Hide();
		fRebootSystemButton->Show();
		// TODO: Temporary work-around for a Haiku bug.
		fRebootSystemButton->Invalidate();

		SetTitle("System is Shut Down");
		fTextView->SetText("It's now safe to turn off the computer.");
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

	class AppIconsView : public BView {
	  public:
		AppIconsView(const BList &appInfos)
			: BView(BRect(0, 0, 100,  15), "app icons", B_FOLLOW_NONE, 
				B_WILL_DRAW),
			  fAppInfos(appInfos),
			  fIconSize(16),
			  fInnerSpacing(4),
			  fEllipsisWidth(0)
		{
			SetViewColor(B_TRANSPARENT_32_BIT);
			fBackground = ui_color(B_PANEL_BACKGROUND_COLOR);
		}

		virtual void Draw(BRect updateRect)
		{
			// find out, how many icons we can draw
			BRect bounds = Bounds();
			int width = bounds.IntegerWidth() + 1;
			int maxIcons = (width + fInnerSpacing)
				/ (fIconSize + fInnerSpacing);
			int minIcons = (width - _EllipsisWidth())
				/ (fIconSize + fInnerSpacing);

			int iconsToDraw = fAppInfos.CountItems();
			bool drawEllipsis = false;
			if (iconsToDraw > maxIcons) {
				iconsToDraw = minIcons;
				drawEllipsis = true;
			}

			// set colors
			SetLowColor(fBackground);
			rgb_color black = { 0, 0, 0, 255 };
			SetHighColor(black);

			// draw the icons
			int lastX = 0;
			for (int i = 0; i < iconsToDraw; i++) {
				int x = lastX + (i > 0 ? fInnerSpacing : 0);
				int nextX = x + fIconSize;
				
				// clear background
				FillRect(BRect(lastX, 0, nextX - 1, bounds.bottom),
					B_SOLID_LOW);

				// draw the icon
				SetDrawingMode(B_OP_OVER);
				AppInfo *info = (AppInfo*)fAppInfos.ItemAt(i);
				if (info->miniIcon)
					DrawBitmap(info->miniIcon, BPoint(x, 0));

				lastX = nextX;
			}

			// clear remaining space
			FillRect(BRect(lastX, 0, bounds.right, bounds.bottom), B_SOLID_LOW);

			// draw ellipsis, if necessary
			if (drawEllipsis) {
				int x = lastX + fInnerSpacing;
				int y = (fIconSize + 1) / 2;
				DrawString(B_UTF8_ELLIPSIS, BPoint(x, y));
			}
		}

	  private:
		int _EllipsisWidth()
		{
			// init lazily
			if (fEllipsisWidth <= 0) {
				BFont font;
				GetFont(&font);

				fEllipsisWidth = (int)font.StringWidth(B_UTF8_ELLIPSIS);
			}

			return fEllipsisWidth;
		}

	  private:
		const BList	&fAppInfos;
		rgb_color	fBackground;
		int			fIconSize;
		int			fInnerSpacing;
		int			fEllipsisWidth;
	};

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
			if (fAppInfo && fAppInfo->largeIcon) {
				DrawBitmap(fAppInfo->largeIcon, BPoint(0, 0));
			} else {
				SetLowColor(fBackground);
				FillRect(Bounds(), B_SOLID_LOW);
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
	AppIconsView		*fAppIconsView;
	CurrentAppIconView	*fCurrentAppIconView;
	BTextView			*fTextView;
	BButton				*fKillAppButton;
	BButton				*fCancelShutdownButton;
	BButton				*fRebootSystemButton;
	BMessage			*fKillAppMessage;
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
	  fWindow(NULL)
{
}

// destructor
ShutdownProcess::~ShutdownProcess()
{
	// terminate the GUI
	if (fHasGUI) {
		fWindow->Lock();
		fWindow->Quit();
	}

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
	SendReply(fRequest, fShutdownError);
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
	error = fRoster->GetShutdownApps(fUserApps, fSystemApps, fVitalSystemApps);
	if (error != B_OK) {
		fRoster->RemoveWatcher(this);
		fRoster->SetShuttingDown(false);
		RETURN_ERROR(error);
	}

	// TODO: sort the system apps by descending registration time

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
			_LockAppLists();

			uint32 event;

			RosterAppInfo *info = fUserApps.InfoFor(team);
			if (info) {
				fUserApps.RemoveInfo(info);
				event = USER_APP_QUIT_EVENT;
			} else if ((info = fSystemApps.InfoFor(team))) {
				fSystemApps.RemoveInfo(info);
				event = SYSTEM_APP_QUIT_EVENT;
			} else	// not found
				return;

			int32 phase = fCurrentPhase;

			_UnlockAppLists();

			// post the event
			_PushEvent(event, team, phase);

			delete info;

			break;
		}

		case MSG_PHASE_TIMED_OUT:
		{
			// get the phase the event is intended for
			int32 phase = TimeoutEvent::GetMessagePhase(message);
			team_id team = TimeoutEvent::GetMessageTeam(message);;

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
			_PushEvent(TIMEOUT_EVENT, -1, fCurrentPhase);

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

// _LockAppLists
bool
ShutdownProcess::_LockAppLists()
{
	return fWorkerLock.Lock();
}

// _UnlockAppLists
void
ShutdownProcess::_UnlockAppLists()
{
	fWorkerLock.Unlock();
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

		// mini icon
		BBitmap *miniIcon = new(nothrow) BBitmap(BRect(0, 0, 15, 15), B_CMAP8);
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
		BBitmap *largeIcon = new(nothrow) BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
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
	// get the reboot flag
	bool reboot;
	if (fRequest->FindBool("reboot", &reboot) != B_OK)
		reboot = false;

	#ifdef __HAIKU__
		return _kern_shutdown(reboot);
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
	if (eventType == USER_APP_QUIT_EVENT || eventType == SYSTEM_APP_QUIT_EVENT)
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
		PRINT(("ShutdownProcess::_Worker(): caught exception: %s\n",
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

	// make the shutdown window ready and show it
	_InitShutdownWindow();
	_SetShutdownWindowCurrentApp(-1);
	_SetShutdownWindowText("Tidying things up a bit.");
	_SetShutdownWindowCancelButtonEnabled(true);
	_SetShutdownWindowKillButtonEnabled(false);
	_SetShowShutdownWindow(true);

	// sync
	sync();

	// phase 1: terminate the user apps
	_SetPhase(USER_APP_TERMINATION_PHASE);
	_QuitUserApps();
	_ScheduleTimeoutEvent(USER_APPS_QUIT_TIMEOUT);
	_WaitForUserApps();
	_KillUserApps();

	// phase 2: terminate the system apps
	_SetPhase(SYSTEM_APP_TERMINATION_PHASE);
	_QuitSystemApps();

	// phase 3: terminate the other processes
	_SetPhase(OTHER_PROCESSES_TERMINATION_PHASE);
	_QuitNonApps();

	// we're through: do the shutdown
	_SetPhase(DONE_PHASE);
	_ShutDown();

	PRINT(("  _kern_shutdown() failed\n"));

	// shutdown failed: This can happen for power off mode -- reboot should
	// always work.
	if (fHasGUI) {
		_SetShutdownWindowWaitForShutdown();

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

// _QuitUserApps
void
ShutdownProcess::_QuitUserApps()
{
	PRINT(("ShutdownProcess::_QuitUserApps()\n"));

	_SetShutdownWindowText("Asking user applications to quit.");

	// prepare the shutdown message
	BMessage message;
	_PrepareShutdownMessage(message);

	// send shutdown messages to user apps
	_LockAppLists();

	AppInfoListMessagingTargetSet targetSet(fUserApps);

	if (targetSet.HasNext()) {
		PRINT(("  sending shutdown message to %ld apps\n",
			fUserApps.CountInfos()));

		status_t error = MessageDeliverer::Default()->DeliverMessage(
			&message, targetSet);
		if (error != B_OK) {
			WARNING(("ShutdownProcess::_Worker(): Failed to deliver "
				"shutdown message to all applications: %s\n",
				strerror(error)));
		}
	}

	_UnlockAppLists();

	PRINT(("ShutdownProcess::_QuitUserApps() done\n"));
}

// _WaitForUserApps
void
ShutdownProcess::_WaitForUserApps()
{
	PRINT(("ShutdownProcess::_WaitForUserApps()\n"));

	// wait for user apps
	bool moreApps = true;
	while (moreApps) {
		_LockAppLists();
		moreApps = !fUserApps.IsEmpty();
		_UnlockAppLists();

		if (moreApps) {
			uint32 event;
			team_id team;
			int32 phase;
			status_t error = _GetNextEvent(event, team, phase, true);
			if (error != B_OK)
				throw error;

			if (event == ABORT_EVENT)
				throw B_SHUTDOWN_CANCELLED;

			if (event == TIMEOUT_EVENT)	
				return;
		}
	}

	PRINT(("ShutdownProcess::_WaitForUserApps() done\n"));
}

// _KillUserApps
void
ShutdownProcess::_KillUserApps()
{
	PRINT(("ShutdownProcess::_KillUserApps()\n"));

	while (true) {
		// eat events (we need to be responsive for an abort event)
		uint32 event;
		do {
			team_id team;
			int32 phase;
			status_t error = _GetNextEvent(event, team, phase, false);
			if (error != B_OK)
				throw error;

			if (event == ABORT_EVENT)
				throw B_SHUTDOWN_CANCELLED;

		} while (event != NO_EVENT);

		// get the first team to kill
		_LockAppLists();

		team_id team = -1;
		AppInfoList &list = fUserApps;
		if (!list.IsEmpty()) {
			RosterAppInfo *info = *list.It();
			team = info->team;
		}

		_UnlockAppLists();

		if (team < 0) {
			PRINT(("ShutdownProcess::_KillUserApps() done\n"));
			return;
		}

		// TODO: check whether the app blocks on a modal alert
		if (false) {
			// ...
		} else {
			// it does not: kill it
			PRINT(("  killing team %ld\n", team));

			kill_team(team);

			// remove the app (the roster will note eventually and send us
			// a notification, but we want to be sure)
			_LockAppLists();
	
			if (RosterAppInfo *info = list.InfoFor(team)) {
				list.RemoveInfo(info);
				delete info;
			}
	
			_UnlockAppLists();
		}
	}
}

// _QuitSystemApps
void
ShutdownProcess::_QuitSystemApps()
{
	PRINT(("ShutdownProcess::_QuitSystemApps()\n"));

	_SetShutdownWindowCancelButtonEnabled(false);

	// check one last time for abort events
	uint32 event;
	do {
		team_id team;
		int32 phase;
		status_t error = _GetNextEvent(event, team, phase, false);
		if (error != B_OK)
			throw error;

		if (event == ABORT_EVENT)
			throw B_SHUTDOWN_CANCELLED;

	} while (event != NO_EVENT);

	// prepare the shutdown message
	BMessage message;
	_PrepareShutdownMessage(message);

	// now iterate through the list of system apps
	while (true) {
		// eat events
		uint32 event;
		do {
			team_id team;
			int32 phase;
			status_t error = _GetNextEvent(event, team, phase, false);
			if (error != B_OK)
				throw error;
		} while (event != NO_EVENT);

		// get the first app to quit
		_LockAppLists();

		AppInfoList &list = fSystemApps;
		team_id team = -1;
		port_id port = -1;
		char appName[B_FILE_NAME_LENGTH];
		if (!list.IsEmpty()) {
			RosterAppInfo *info = *list.It();
			team = info->team;
			port = info->port;
			strcpy(appName, info->ref.name);
		}


		_UnlockAppLists();

		if (team < 0) {
			PRINT(("ShutdownProcess::_QuitSystemApps() done\n"));
			return;
		}

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
		_ScheduleTimeoutEvent(SYSTEM_APPS_QUIT_TIMEOUT, team);

		// wait for the app to die or for the timeout to occur
		do {
			team_id eventTeam;
			int32 phase;
			status_t error = _GetNextEvent(event, eventTeam, phase, true);
			if (error != B_OK)
				throw error;

			if (event == TIMEOUT_EVENT && eventTeam == team)
				break;

			// If the app requests aborting the shutdown, we don't need to
			// wait any longer. It has processed the request and won't quit by
			// itself. We'll have to kill it.
			if (event == ABORT_EVENT && eventTeam == team)
				break;

			BAutolock _(fWorkerLock);
			if (!list.InfoFor(team))
				break;

		} while (event != NO_EVENT);

		// TODO: check whether the app blocks on a modal alert
		if (false) {
			// ...
		} else {
			// it does not: kill it
			PRINT(("  killing team %ld\n", team));

			kill_team(team);

			// remove the app (the roster will note eventually and send us
			// a notification, but we want to be sure)
			_LockAppLists();
	
			if (RosterAppInfo *info = list.InfoFor(team)) {
				list.RemoveInfo(info);
				delete info;
			}
	
			_UnlockAppLists();
		}
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
	snooze(NON_APPS_QUIT_TIMEOUT);

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

