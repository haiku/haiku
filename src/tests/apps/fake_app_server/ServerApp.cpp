/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <AppDefs.h>
#include <List.h>
#include <String.h>
#include <stdio.h>
#include <string.h>
#include <ServerProtocol.h>

#include "AppServer.h"

#include "ServerApp.h"

//#define DEBUG_SERVERAPP

#ifdef DEBUG_SERVERAPP
#	include <stdio.h>
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

//#define DEBUG_SERVERAPP_FONT

#ifdef DEBUG_SERVERAPP_FONT
#	include <stdio.h>
#	define FTRACE(x) printf x
#else
#	define FTRACE(x) ;
#endif

/*!
	\brief Constructor
	\param sendport port ID for the BApplication which will receive the ServerApp's messages
	\param rcvport port by which the ServerApp will receive messages from its BApplication.
	\param fSignature NULL-terminated string which contains the BApplication's
	MIME fSignature.
*/
ServerApp::ServerApp(port_id sendport, port_id rcvport, port_id clientLooperPort,
	team_id clientTeamID, int32 handlerID, const char* signature)
	:
	fClientAppPort(sendport),
	fMessagePort(rcvport),
	fClientLooperPort(clientLooperPort),
	fSignature(signature),
	fMonitorThreadID(-1),
	fClientTeamID(clientTeamID),
	fLink(fClientAppPort, fMessagePort),
	fLockSem(create_sem(1, "ServerApp sem")),
	fCursorHidden(false),
	fIsActive(false),
	fQuitting(false)
{
	if (fSignature == "")
		fSignature = "application/x-vnd.NULL-application-signature";

	Run();

	STRACE(("ServerApp %s:\n", fSignature.String()));
	STRACE(("\tBApp port: %ld\n", fClientAppPort));
	STRACE(("\tReceiver port: %ld\n", fMessagePort));
}


//! Does all necessary teardown for application
ServerApp::~ServerApp(void)
{
	STRACE(("*ServerApp %s:~ServerApp()\n",fSignature.String()));
	
	fQuitting = true;

	// This shouldn't be necessary -- all cursors owned by the app
	// should be cleaned up by RemoveAppCursors	
//	if(fAppCursor)
//		delete fAppCursor;

	delete_sem(fLockSem);
	
	STRACE(("#ServerApp %s:~ServerApp()\n", fSignature.String()));

	// TODO: Is this the right place for this ?
	// From what I've understood, this is the port created by
	// the BApplication (?), but if I delete it in there, GetNextMessage()
	// in the MonitorApp thread never returns. Cleanup.
	delete_port(fMessagePort);
	
	status_t dummyStatus;
	wait_for_thread(fMonitorThreadID, &dummyStatus);
	
	STRACE(("ServerApp %s::~ServerApp(): Exiting\n", fSignature.String()));
}

/*!
	\brief Starts the ServerApp monitoring for messages
	\return false if the application couldn't start, true if everything went OK.
*/
bool
ServerApp::Run(void)
{
	// Unlike a BApplication, a ServerApp is *supposed* to return immediately
	// when its Run() function is called.	
	fMonitorThreadID = spawn_thread(MonitorApp, fSignature.String(), B_NORMAL_PRIORITY, this);
	if (fMonitorThreadID < B_OK)
		return false;

	return resume_thread(fMonitorThreadID) == B_OK;
}

/*!
	\brief Pings the target app to make sure it's still working
	\return true if target is still "alive" and false if "He's dead, Jim." 
	"But that's impossible..."
	
	This function is called by the app_server thread to ensure that
	the target app still exists. We do this not by sending a message
	but by calling get_port_info. We don't want to send ping messages
	just because the app might simply be hung. If this is the case, it
	should be up to the user to kill it. If the app has been killed, its
	ports will be invalid. Thus, if get_port_info returns an error, we
	tell the app_server to delete the respective ServerApp.
*/
bool
ServerApp::PingTarget(void)
{
	team_info tinfo;
	if (get_team_info(fClientTeamID, &tinfo) == B_BAD_TEAM_ID) {
		BPrivate::LinkSender link(gAppServerPort);
		link.StartMessage(AS_DELETE_APP);
		link.Attach(&fMonitorThreadID, sizeof(thread_id));
		link.Flush();
		return false;
	}
	return true;
}

/*!
	\brief Send a message to the ServerApp with no attachments
	\param code ID code of the message to post
*/
void
ServerApp::PostMessage(int32 code)
{
	BPrivate::LinkSender link(fMessagePort);
	link.StartMessage(code);
	link.Flush();
}

/*!
	\brief Send a message to the ServerApp's BApplication
	\param msg The message to send
*/
void
ServerApp::SendMessageToClient(const BMessage *msg) const
{
	ssize_t size = msg->FlattenedSize();
	char *buffer = new char[size];

	if (msg->Flatten(buffer, size) == B_OK)
		write_port(fClientLooperPort, msg->what, buffer, size);
	else
		printf("PANIC: ServerApp: '%s': can't flatten message in 'SendMessageToClient()'\n", fSignature.String());

	delete [] buffer;
}

/*!
	\brief Sets the ServerApp's active status
	\param value The new status of the ServerApp.
	
	This changes an internal flag and also sets the current cursor to the one specified by
	the application
*/
void
ServerApp::Activate(bool value)
{
	fIsActive = value;
}


/*!
	\brief The thread function ServerApps use to monitor messages
	\param data Pointer to the thread's ServerApp object
	\return Throwaway value - always 0
*/
int32
ServerApp::MonitorApp(void *data)
{
	// Message-dispatching loop for the ServerApp

	ServerApp *app = (ServerApp *)data;
	BPrivate::LinkReceiver &reader = app->fLink.Receiver();

	int32 code;
	status_t err = B_OK;

	while (!app->fQuitting) {
		STRACE(("info: ServerApp::MonitorApp listening on port %ld.\n", app->fMessagePort));
		err = reader.GetNextMessage(code, B_INFINITE_TIMEOUT);
		if (err < B_OK) {
			STRACE(("ServerApp::MonitorApp(): GetNextMessage returned %s\n", strerror(err)));

			// ToDo: this should kill the app, but it doesn't work
			BPrivate::LinkSender link(gAppServerPort);
			link.StartMessage(AS_DELETE_APP);
			link.Attach(&app->fMonitorThreadID, sizeof(thread_id));
			link.Flush();
			break;
		}

		switch (code) {
			case AS_QUIT_APP:
			{
				// This message is received only when the app_server is asked to shut down in
				// test/debug mode. Of course, if we are testing while using AccelerantDriver, we do
				// NOT want to shut down client applications. The server can be quit o in this fashion
				// through the driver's interface, such as closing the ViewDriver's window.

				STRACE(("ServerApp %s:Server shutdown notification received\n",
					app->fSignature.String()));

				// If we are using the real, accelerated version of the
				// DisplayDriver, we do NOT want the user to be able shut down
				// the server. The results would NOT be pretty
				break;
			}
			
			case B_QUIT_REQUESTED:
			{
				STRACE(("ServerApp %s: B_QUIT_REQUESTED\n",app->fSignature.String()));
				// Our BApplication sent us this message when it quit.
				// We need to ask the app_server to delete ourself.
				BPrivate::LinkSender sender(gAppServerPort);
				sender.StartMessage(AS_DELETE_APP);
				sender.Attach(&app->fMonitorThreadID, sizeof(thread_id));
				sender.Flush();
				break;
			}

			default:
				STRACE(("ServerApp %s: Got a Message to dispatch\n", app->fSignature.String()));
				app->DispatchMessage(code, reader);
				break;
		}
	}

	return 0;
}


/*!
	\brief Handler function for BApplication API messages
	\param code Identifier code for the message. Equivalent to BMessage::what
	\param buffer Any attachments
	
	Note that the buffer's exact format is determined by the particular message. 
	All attachments are placed in the buffer via a PortLink, so it will be a 
	matter of casting and incrementing an index variable to access them.
*/
void
ServerApp::DispatchMessage(int32 code, BPrivate::LinkReceiver &link)
{
	switch (code) {
		case AS_CURRENT_WORKSPACE:
		{
			STRACE(("ServerApp %s: get current workspace\n", fSignature.String()));

			// TODO: Locking this way is not nice
			fLink.StartMessage(B_OK);
			fLink.Attach<int32>(0);
			fLink.Flush();
			break;
		}
		
		case AS_ACTIVATE_WORKSPACE:
		{
			STRACE(("ServerApp %s: activate workspace\n", fSignature.String()));

			// TODO: See above
			int32 index;
			link.Read<int32>(&index);
			// no reply
		
			break;
		}
		
		case AS_QUERY_CURSOR_HIDDEN:
		{
			STRACE(("ServerApp %s: Received IsCursorHidden request\n", fSignature.String()));
			// Attached data
			// 1) int32 port to reply to
			fLink.StartMessage(fCursorHidden ? B_OK : B_ERROR);
			fLink.Flush();
			break;
		}

		default:
			printf("ServerApp %s received unhandled message code offset %lx\n",
				fSignature.String(), code);

			if (link.NeedsReply()) {
				// the client is now blocking and waiting for a reply!
				fLink.StartMessage(B_ERROR);
				fLink.Flush();
			} else
				puts("message doesn't need a reply!");
			break;
	}
}


team_id
ServerApp::ClientTeamID() const
{
	return fClientTeamID;
}


thread_id
ServerApp::MonitorThreadID() const
{
	return fMonitorThreadID;
}

