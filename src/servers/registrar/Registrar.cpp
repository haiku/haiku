// Registrar.cpp

#ifndef DEBUG
#	define DEBUG 1
#endif
#include "Debug.h"

#include <Application.h>
#include <Message.h>
#include <OS.h>
#include <RegistrarDefs.h>

#include "ClipboardHandler.h"
#include "MIMEManager.h"
#include "Registrar.h"
#include "TRoster.h"

// constructor
Registrar::Registrar()
		 : BApplication(kRegistrarSignature),
		   fRoster(NULL),
		   fClipboardHandler(NULL),
		   fMIMEManager(NULL)
{
FUNCTION_START();
	// move the following code to ReadyToRun() once it works.
	fRoster = new TRoster;
	fClipboardHandler = new ClipboardHandler;
	AddHandler(fClipboardHandler);
	fMIMEManager = new MIMEManager;
	fMIMEManager->Run();
}

// destructor
Registrar::~Registrar()
{
FUNCTION_START();
	fMIMEManager->Lock();
	fMIMEManager->Quit();
	RemoveHandler(fClipboardHandler);
	delete fClipboardHandler;
	delete fRoster;
}

// MessageReceived
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
		case B_REG_IS_PRE_REGISTERED:
			fRoster->HandleIsAppPreRegistered(message);
			break;
		case B_REG_REMOVE_PRE_REGISTERED_APP:
			fRoster->HandleRemovePreRegApp(message);
			break;
		case B_REG_REMOVE_APP:
			fRoster->HandleRemoveApp(message);
			break;
		default:
			BApplication::MessageReceived(message);
			break;
	}
FUNCTION_END();
}

// ReadyToRun
void
Registrar::ReadyToRun()
{
FUNCTION_START();
}

// QuitRequested
bool
Registrar::QuitRequested()
{
FUNCTION_START();
	// The final registrar must not quit. At least not that easily. ;-)
	return BApplication::QuitRequested();
}


// main
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

