// Registrar.cpp

#include "Debug.h"

#include <Application.h>
#include <Message.h>
#include <OS.h>
#include <RegistrarDefs.h>

#include "ClipboardHandler.h"
#include "MIMEManager.h"
#include "Registrar.h"
#include "TRoster.h"

/*!
	\class Registrar
	\brief The application class of the registrar.

	Glues the registrar services together and dispatches the roster messages.
*/

// constructor
/*!	\brief Creates the registrar application class.
*/
Registrar::Registrar()
		 : BApplication(kRegistrarSignature),
		   fRoster(NULL),
		   fClipboardHandler(NULL),
		   fMIMEManager(NULL)
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
	fMIMEManager->Lock();
	fMIMEManager->Quit();
	RemoveHandler(fClipboardHandler);
	delete fClipboardHandler;
	delete fRoster;
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
	FUNCTION_START();
	switch (message->what) {
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
		case B_REG_GET_APP_INFO:
			fRoster->HandleGetAppInfo(message);
			break;
		case B_REG_GET_APP_LIST:
			fRoster->HandleGetAppList(message);
			break;
		case B_REG_ACTIVATE_APP:
			fRoster->HandleActivateApp(message);
			break;
		default:
			BApplication::MessageReceived(message);
			break;
	}
	FUNCTION_END();
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
	// init the global be_roster
	BPrivate::init_registrar_roster(be_app_messenger,
									BMessenger(NULL, fMIMEManager));
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

