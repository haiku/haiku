// Registrar.cpp

#include "Debug.h"

#include <Application.h>
#include <Message.h>
#include <OS.h>
#include <RegistrarDefs.h>

#include "ClipboardHandler.h"
#include "EventQueue.h"
#include "MessageEvent.h"
#include "MessageRunnerManager.h"
#include "MIMEManager.h"
#include "Registrar.h"
#include "TRoster.h"

/*!
	\class Registrar
	\brief The application class of the registrar.

	Glues the registrar services together and dispatches the roster messages.
*/

//! Name of the event queue.
static const char *kEventQueueName = "timer_thread";

//! Time interval between two roster sanity checks (1 s).
static const bigtime_t kRosterSanityEventInterval = 1000000LL;

// constructor
/*!	\brief Creates the registrar application class.
*/
Registrar::Registrar()
		 : BApplication(kRegistrarSignature),
		   fRoster(NULL),
		   fClipboardHandler(NULL),
		   fMIMEManager(NULL),
		   fEventQueue(NULL),
		   fMessageRunnerManager(NULL),
		   fSanityEvent(NULL)
{
	FUNCTION_START();
}

// destructor
/*!	\brief Frees all resources associated with the registrar.

	All registrar services, that haven't been shut down earlier, are
	terminated.
*/
Registrar::~Registrar()
{
	FUNCTION_START();
	Lock();
	fEventQueue->Die();
	delete fMessageRunnerManager;
	delete fEventQueue;
	delete fSanityEvent;
	fMIMEManager->Lock();
	fMIMEManager->Quit();
	RemoveHandler(fClipboardHandler);
	delete fClipboardHandler;
	delete fRoster;
	// Invalidate the global be_roster, so that the BApplication destructor
	// won't dead-lock when sending a message to itself.
	BPrivate::init_registrar_roster(BMessenger(), BMessenger());
	FUNCTION_END();
}

// MessageReceived
/*!	\brief Overrides the super class version to dispatch roster specific
		   messages.
	\param message The message to be handled
*/
void
Registrar::MessageReceived(BMessage *message)
{
//	FUNCTION_START();
	switch (message->what) {
		// general requests
		case B_REG_GET_MIME_MESSENGER:
		{
			PRINT(("B_REG_GET_MIME_MESSENGER\n"));
			BMessenger messenger(NULL, fMIMEManager);
			BMessage reply(B_REG_SUCCESS);
			reply.AddMessenger("messenger", messenger);
			message->SendReply(&reply);
			break;
		}
		case B_REG_GET_CLIPBOARD_MESSENGER:
		{
			PRINT(("B_REG_GET_CLIPBOARD_MESSENGER\n"));
			BMessenger messenger(fClipboardHandler);
			BMessage reply(B_REG_SUCCESS);
			reply.AddMessenger("messenger", messenger);
			message->SendReply(&reply);
			break;
		}
		// roster requests
		case B_REG_ADD_APP:
			fRoster->HandleAddApplication(message);
			break;
		case B_REG_COMPLETE_REGISTRATION:
			fRoster->HandleCompleteRegistration(message);
			break;
		case B_REG_IS_APP_PRE_REGISTERED:
			fRoster->HandleIsAppPreRegistered(message);
			break;
		case B_REG_REMOVE_PRE_REGISTERED_APP:
			fRoster->HandleRemovePreRegApp(message);
			break;
		case B_REG_REMOVE_APP:
			fRoster->HandleRemoveApp(message);
			break;
		case B_REG_SET_THREAD_AND_TEAM:
			fRoster->HandleSetThreadAndTeam(message);
			break;
		case B_REG_SET_SIGNATURE:
			fRoster->HandleSetSignature(message);
			break;
		case B_REG_GET_APP_INFO:
			fRoster->HandleGetAppInfo(message);
			break;
		case B_REG_GET_APP_LIST:
			fRoster->HandleGetAppList(message);
			break;
		case B_REG_ACTIVATE_APP:
			fRoster->HandleActivateApp(message);
			break;
		case B_REG_BROADCAST:
			fRoster->HandleBroadcast(message);
			break;
		case B_REG_START_WATCHING:
			fRoster->HandleStartWatching(message);
			break;
		case B_REG_STOP_WATCHING:
			fRoster->HandleStopWatching(message);
			break;	
		case B_REG_GET_RECENT_DOCUMENTS:
			fRoster->HandleGetRecentDocuments(message);
			break;
		case B_REG_GET_RECENT_FOLDERS:
			fRoster->HandleGetRecentFolders(message);
			break;
		case B_REG_GET_RECENT_APPS:
			fRoster->HandleGetRecentApps(message);
			break;
		case B_REG_ADD_TO_RECENT_DOCUMENTS:
			fRoster->HandleAddToRecentDocuments(message);
			break;
		case B_REG_ADD_TO_RECENT_FOLDERS:
			fRoster->HandleAddToRecentFolders(message);
			break;
		case B_REG_ADD_TO_RECENT_APPS:
			fRoster->HandleAddToRecentApps(message);
			break;
		case B_REG_CLEAR_RECENT_DOCUMENTS:
			fRoster->HandleClearRecentDocuments(message);
			break;
		case B_REG_CLEAR_RECENT_FOLDERS:
			fRoster->HandleClearRecentFolders(message);
			break;
		case B_REG_CLEAR_RECENT_APPS:
			fRoster->HandleClearRecentApps(message);
			break;
		// message runner requests
		case B_REG_REGISTER_MESSAGE_RUNNER:
			fMessageRunnerManager->HandleRegisterRunner(message);
			break;
		case B_REG_UNREGISTER_MESSAGE_RUNNER:
			fMessageRunnerManager->HandleUnregisterRunner(message);
			break;
		case B_REG_SET_MESSAGE_RUNNER_PARAMS:
			fMessageRunnerManager->HandleSetRunnerParams(message);
			break;
		case B_REG_GET_MESSAGE_RUNNER_INFO:
			fMessageRunnerManager->HandleGetRunnerInfo(message);
			break;
		// internal messages
		case B_REG_ROSTER_SANITY_EVENT:
			fRoster->CheckSanity();
			fSanityEvent->SetTime(system_time() + kRosterSanityEventInterval);
			fEventQueue->AddEvent(fSanityEvent);
			break;
		default:
			BApplication::MessageReceived(message);
			break;
	}
//	FUNCTION_END();
}

// ReadyToRun
/*!	\brief Overrides the super class version to initialize the registrar
		   services.
*/
void
Registrar::ReadyToRun()
{
	FUNCTION_START();
	// create roster
	fRoster = new TRoster;
	fRoster->Init();
	// create clipboard handler
	fClipboardHandler = new ClipboardHandler;
	AddHandler(fClipboardHandler);
	// create MIME manager
	fMIMEManager = new MIMEManager;
	fMIMEManager->Run();
	// create message runner manager
	fEventQueue = new EventQueue(kEventQueueName);
	fMessageRunnerManager = new MessageRunnerManager(fEventQueue);
	// init the global be_roster
	BPrivate::init_registrar_roster(be_app_messenger,
									BMessenger(NULL, fMIMEManager));
	// create and schedule the sanity message event
	fSanityEvent = new MessageEvent(system_time() + kRosterSanityEventInterval,
									this, B_REG_ROSTER_SANITY_EVENT);
	fSanityEvent->SetAutoDelete(false);
	fEventQueue->AddEvent(fSanityEvent);
	FUNCTION_END();
}

// QuitRequested
/*!	\brief Overrides the super class version to avoid termination of the
		   registrar until the system shutdown.
*/
bool
Registrar::QuitRequested()
{
	FUNCTION_START();
	// The final registrar must not quit. At least not that easily. ;-)
	return BApplication::QuitRequested();
}

// GetEventQueue
/*!	\brief Returns the registrar's event queue.
	\return The registrar's event queue.
*/
EventQueue*
Registrar::GetEventQueue() const
{
	return fEventQueue;
}

// App
/*!	\brief Returns the Registrar application object.
	\return The Registrar application object.
*/
Registrar*
Registrar::App()
{
	return dynamic_cast<Registrar*>(be_app);
}


// init_registrar_roster
/*!	\brief Initializes the global \a be_roster.

	While this is done automagically for all other applications while libbe
	initialization, the registrar needs to help out a bit.

	\param mainMessenger A BMessenger targeting the registrar application.
	\param mimeMessenger A BMessenger targeting the MIME manager.
*/
void
BPrivate::init_registrar_roster(BMessenger mainMessenger,
								BMessenger mimeMessenger)
{
	BRoster *roster = const_cast<BRoster*>(be_roster);
	roster->fMess = mainMessenger;
	roster->fMimeMess = mimeMessenger;
}


// main
/*!	\brief Creates and runs the registrar application.

	The main thread is renamed.

	\return 0.
*/
int
main()
{
	FUNCTION_START();
	// rename the main thread
	rename_thread(find_thread(NULL), kRosterThreadName);
	// create and run the registrar application
	Registrar *app = new Registrar();
PRINT(("app->Run()...\n"));
	app->Run();
PRINT(("delete app...\n"));
	delete app;
	FUNCTION_END();
	return 0;
}

