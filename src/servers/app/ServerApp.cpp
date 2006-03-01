/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jérôme Duval, jerome.duval@free.fr
 */

/*!
	\class ServerApp ServerApp.h
	\brief Counterpart to BApplication within the app_server
*/

#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <AppDefs.h>
#include <Autolock.h>
#include <List.h>
#include <ScrollBar.h>
#include <Shape.h>
#include <String.h>

#include <ColorSet.h>
#include <FontPrivate.h>
#include <MessengerPrivate.h>
#include <ServerProtocol.h>

#include "AppServer.h"
#include "BGet++.h"
#include "BitmapManager.h"
#include "CursorManager.h"
#include "CursorSet.h"
#include "Desktop.h"
#include "DebugInfoManager.h"
#include "DecorManager.h"
#include "DrawingEngine.h"
#include "EventStream.h"
#include "FontManager.h"
#include "HWInterface.h"
#include "InputManager.h"
#include "OffscreenServerWindow.h"
#include "RAMLinkMsgReader.h"
#include "ServerApp.h"
#include "ServerBitmap.h"
#include "ServerConfig.h"
#include "ServerCursor.h"
#include "ServerPicture.h"
#include "ServerScreen.h"
#include "ServerTokenSpace.h"
#include "ServerWindow.h"
#include "SystemPalette.h"
#include "WindowLayer.h"

//#define DEBUG_SERVERAPP

#ifdef DEBUG_SERVERAPP
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif

//#define DEBUG_SERVERAPP_FONT

#ifdef DEBUG_SERVERAPP_FONT
#	define FTRACE(x) printf x
#else
#	define FTRACE(x) ;
#endif

using std::nothrow;

static const uint32 kMsgAppQuit = 'appQ';


ServerApp::ServerApp(Desktop* desktop, port_id clientReplyPort,
		port_id clientLooperPort, team_id clientTeam,
		int32 clientToken, const char* signature)
	: MessageLooper("application"),

	fMessagePort(-1),
	fClientReplyPort(clientReplyPort),
	fDesktop(desktop),
	fSignature(signature),
	fClientTeam(clientTeam),
	fWindowListLock("window list"),
	fAppCursor(NULL),
	fCursorHideLevel(0),
	fIsActive(false),
	fSharedMem("shared memory", 32768)
{
	if (fSignature == "")
		fSignature = "application/no-signature";

	char name[B_OS_NAME_LENGTH];
	snprintf(name, sizeof(name), "a<%s", Signature());

	fMessagePort = create_port(DEFAULT_MONITOR_PORT_SIZE, name);
	if (fMessagePort < B_OK)
		return;

	fLink.SetSenderPort(fClientReplyPort);
	fLink.SetReceiverPort(fMessagePort);

	// we let the application own the port, so that we get aware when it's gone
	if (set_port_owner(fMessagePort, clientTeam) < B_OK) {
		delete_port(fMessagePort);
		fMessagePort = -1;
		return;
	}

	BMessenger::Private(fHandlerMessenger).SetTo(fClientTeam,
		clientLooperPort, clientToken);

	fInitialWorkspace = desktop->CurrentWorkspace();
		// TODO: this should probably be retrieved when the app is loaded!

	STRACE(("ServerApp %s:\n", Signature()));
	STRACE(("\tBApp port: %ld\n", fClientReplyPort));
	STRACE(("\tReceiver port: %ld\n", fMessagePort));
}


ServerApp::~ServerApp()
{
	STRACE(("*ServerApp %s:~ServerApp()\n", Signature()));

	if (!fQuitting)
		CRITICAL("ServerApp: destructor called after Run()!\n");

	// quit all server windows

	fWindowListLock.Lock();
	for (int32 i = fWindowList.CountItems(); i-- > 0;) {
		ServerWindow* window = fWindowList.ItemAt(i);
		window->Quit();
	}
	fWindowListLock.Unlock();

	// wait for the windows to quit

	snooze(20000);

	fWindowListLock.Lock();
	for (int32 i = fWindowList.CountItems(); i-- > 0;) {
		ServerWindow* window = fWindowList.ItemAt(i);

		// a window could have been remove in the mean time (if those 20 millisecs
		// from above weren't enough)
		if (window == NULL)
			continue;

		sem_id deathSemaphore = window->DeathSemaphore();
		fWindowListLock.Unlock();

		// wait 3 seconds for our window to quit - that's quite a long
		// time, but killing it might have desastrous effects
		if (MessageLooper::WaitForQuit(deathSemaphore, 3000000) != B_OK) {
			// This really shouldn't happen, as it shows we're buggy
#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
			syslog(LOG_ERR, "ServerApp %s: ServerWindow doesn't respond!\n",
				Signature());
#else
			debugger("ServerWindow doesn't respond!\n");
#endif
		}
		fWindowListLock.Lock();
	}

	// first, make sure our monitor thread doesn't 
	for (int32 i = 0; i < fBitmapList.CountItems(); i++) {
		gBitmapManager->DeleteBitmap(static_cast<ServerBitmap *>(fBitmapList.ItemAt(i)));
	}

	int32 pictureCount = fPictureList.CountItems();
	for (int32 i = 0; i < pictureCount; i++) {
		delete (ServerPicture*)fPictureList.ItemAtFast(i);
	}

	fDesktop->GetCursorManager().DeleteCursors(fClientTeam);

	STRACE(("ServerApp %s::~ServerApp(): Exiting\n", Signature()));
}


/*!
	\brief Checks if the application was initialized correctly
*/
status_t
ServerApp::InitCheck()
{
	if (fMessagePort < B_OK)
		return fMessagePort;

	if (fClientReplyPort < B_OK)
		return fClientReplyPort;

	if (fWindowListLock.Sem() < B_OK)
		return fWindowListLock.Sem();

	return B_OK;
}


/*!
	\brief Starts the ServerApp monitoring for messages
	\return false if the application couldn't start, true if everything went OK.
*/
bool
ServerApp::Run()
{
	if (!MessageLooper::Run())
		return false;

	// Let's tell the client how to talk with us
	fLink.StartMessage(B_OK);
	fLink.Attach<int32>(fMessagePort);
	fLink.Flush();

	return true;
}


/*!
	\brief This quits the application and deletes it. You're not supposed
		to call its destructor directly.

	At the point you're calling this method, the application should already
	be removed from the application list.
*/
void
ServerApp::Quit(sem_id shutdownSemaphore)
{
	if (fThread < B_OK) {
		delete this;
		return;
	}

	// execute application deletion in the message looper thread

	fQuitting = true;
	PostMessage(kMsgAppQuit);

	send_data(fThread, 'QUIT', &shutdownSemaphore, sizeof(sem_id));
}


/*!
	\brief Send a message to the ServerApp's BApplication
	\param msg The message to send
*/
void
ServerApp::SendMessageToClient(BMessage *msg) const
{
	status_t status = fHandlerMessenger.SendMessage(msg, (BHandler*)NULL, 100000);
	if (status != B_OK)
		printf("app %s send to client failed: %s\n", Signature(), strerror(status));
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
	if (fIsActive == value)
		return;

	// TODO: send some message to the client?!?

	fIsActive = value;

	if (fIsActive)
		SetCursor();
}


//! Sets the cursor to the application cursor, if any.
void
ServerApp::SetCursor()
{
	fDesktop->SetCursor(fAppCursor);
	fDesktop->HWInterface()->SetCursorVisible(fCursorHideLevel == 0);
}


void
ServerApp::_GetLooperName(char* name, size_t length)
{
#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
	strlcpy(name, Signature(), length);
#else
	strncpy(name, Signature(), length);
	name[length - 1] = '\0';
#endif
}


/*!
	\brief The thread function ServerApps use to monitor messages
*/
void
ServerApp::_MessageLooper()
{
	// Message-dispatching loop for the ServerApp

	BPrivate::LinkReceiver &receiver = fLink.Receiver();

	int32 code;
	status_t err = B_OK;

	while (!fQuitting) {
		STRACE(("info: ServerApp::_MessageLooper() listening on port %ld.\n", fMessagePort));
		err = receiver.GetNextMessage(code, B_INFINITE_TIMEOUT);
		if (err < B_OK || code == B_QUIT_REQUESTED) {
			STRACE(("ServerApp: application seems to be gone...\n"));

			// Tell desktop to quit us
			BPrivate::LinkSender link(fDesktop->MessagePort());
			link.StartMessage(AS_DELETE_APP);
			link.Attach<thread_id>(Thread());
			link.Flush();
			break;
		}

		switch (code) {
			case kMsgAppQuit:
				// we receive this from our destructor on quit
				fQuitting = true;
				break;

			case AS_QUIT_APP:
			{
				// This message is received only when the app_server is asked to shut down in
				// test/debug mode. Of course, if we are testing while using AccelerantDriver, we do
				// NOT want to shut down client applications. The server can be quit o in this fashion
				// through the driver's interface, such as closing the ViewDriver's window.

				STRACE(("ServerApp %s:Server shutdown notification received\n", Signature()));

				// If we are using the real, accelerated version of the
				// DrawingEngine, we do NOT want the user to be able shut down
				// the server. The results would NOT be pretty
#if TEST_MODE
				BMessage pleaseQuit(B_QUIT_REQUESTED);
				SendMessageToClient(&pleaseQuit);
#endif
				break;
			}

			default:
				STRACE(("ServerApp %s: Got a Message to dispatch\n", Signature()));
				_DispatchMessage(code, receiver);
				break;
		}
	}

	// Quit() will send us a message; we're handling the exiting procedure
	thread_id sender;
	sem_id shutdownSemaphore;
	receive_data(&sender, &shutdownSemaphore, sizeof(sem_id));

	delete this;

	if (shutdownSemaphore >= B_OK)
		release_sem(shutdownSemaphore);
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
ServerApp::_DispatchMessage(int32 code, BPrivate::LinkReceiver& link)
{
	switch (code) {
		case AS_REGISTER_INPUT_SERVER:
		{
			EventStream* stream = new (nothrow) InputServerStream(fHandlerMessenger);
			if (stream != NULL
				&& (!stream->IsValid() || !gInputManager->AddStream(stream))) {
				delete stream;
				break;
			}

			// TODO: this should be done using notifications (so that an abandoned
			//	stream will get noticed directly)
			if (fDesktop->EventDispatcher().InitCheck() != B_OK)
				fDesktop->EventDispatcher().SetTo(gInputManager->GetStream());
			break;
		}

		case AS_CREATE_WINDOW:
		case AS_CREATE_OFFSCREEN_WINDOW:
		{
			port_id clientReplyPort = -1;
			status_t status = _CreateWindow(code, link, clientReplyPort);

			// if sucessful, ServerWindow::Run() will already have replied
			if (status < B_OK) {
				// window creation failed, we need to notify the client
				BPrivate::LinkSender reply(clientReplyPort);
				reply.StartMessage(status);
				reply.Flush();
			}
			break;
		}

		case AS_GET_WINDOW_LIST:
		{
			team_id team;
			if (link.Read<team_id>(&team) == B_OK)
				fDesktop->WriteWindowList(team, fLink.Sender());
			break;
		}

		case AS_GET_WINDOW_INFO:
		{
			int32 serverToken;
			if (link.Read<int32>(&serverToken) == B_OK)
				fDesktop->WriteWindowInfo(serverToken, fLink.Sender());
			break;
		}

		case AS_MINIMIZE_TEAM:
		{
			team_id team;
			if (link.Read<team_id>(&team) == B_OK)
				fDesktop->MinimizeApplication(team);
			break;
		}

		case AS_BRING_TEAM_TO_FRONT:
		{
			team_id team;
			if (link.Read<team_id>(&team) == B_OK)
				fDesktop->BringApplicationToFront(team);
			break;
		}

		case AS_WINDOW_ACTION:
		{
			int32 token;
			int32 action;

			link.Read<int32>(&token);
			if (link.Read<int32>(&action) != B_OK)
				break;

			fDesktop->WindowAction(token, action);
			break;
		}

		case AS_UPDATE_COLORS:
		{
			// NOTE: R2: Eventually we will have windows which will notify their children of changes in 
			// system colors
			
/*			STRACE(("ServerApp %s: Received global UI color update notification\n", Signature()));
			ServerWindow *win;
			BMessage msg(_COLORS_UPDATED);
			
			for(int32 i = 0; i < fWindowList.CountItems(); i++) {
				win=(ServerWindow*)fWindowList.ItemAt(i);
				win->Window()->UpdateColors();
				win->SendMessageToClient(AS_UPDATE_COLORS, msg);
			}
*/			break;
		}
		case AS_UPDATE_FONTS:
		{
			// NOTE: R2: Eventually we will have windows which will notify their children of changes in 
			// system fonts
			
/*			STRACE(("ServerApp %s: Received global font update notification\n", Signature()));
			ServerWindow *win;
			BMessage msg(_FONTS_UPDATED);
			
			for(int32 i=0; i<fSWindowList->CountItems(); i++)
			{
				win=(ServerWindow*)fSWindowList->ItemAt(i);
				win->Window()->UpdateFont();
				win->SendMessageToClient(AS_UPDATE_FONTS, msg);
			}
*/			break;
		}
		case AS_AREA_MESSAGE:
		{
			// This occurs in only one kind of case: a message is too big to send over a port. This
			// is really an edge case, so this shouldn't happen *too* often
			
			// Attached Data:
			// 1) area_id id of an area already owned by the server containing the message
			// 2) size_t offset of the pointer in the area
			// 3) size_t size of the message
			
			area_id area;
			size_t offset;
			size_t msgsize;
			area_info ai;
			int8 *msgpointer;

			link.Read<area_id>(&area);
			link.Read<size_t>(&offset);
			link.Read<size_t>(&msgsize);

			// Part sanity check, part get base pointer :)
			if (get_area_info(area, &ai) < B_OK)
				break;

			msgpointer = (int8*)ai.address + offset;

			RAMLinkMsgReader mlink(msgpointer);
			_DispatchMessage(mlink.Code(), mlink);

			// This is a very special case in the sense that when ServerMemIO is used for this 
			// purpose, it will be set to NOT automatically free the memory which it had 
			// requested. This is the server's job once the message has been dispatched.
			fSharedMem.ReleaseBuffer(msgpointer);
			break;
		}
		case AS_ACQUIRE_SERVERMEM:
		{
			// This particular call is more than a bit of a pain in the neck. We are given a
			// size of a chunk of memory needed. We need to (1) allocate it, (2) get the area for
			// this particular chunk, (3) find the offset in the area for this chunk, and (4)
			// tell the client about it. Good thing this particular call isn't used much
			
			// Received from a ServerMemIO object requesting operating memory
			// Attached Data:
			// 1) size_t requested size
			// 2) port_id reply_port

			size_t memsize;
			link.Read<size_t>(&memsize);

			// TODO: I wonder if ACQUIRE_SERVERMEM should have a minimum size requirement?

			void *sharedmem = fSharedMem.GetBuffer(memsize);

			if (memsize < 1 || sharedmem == NULL) {
				fLink.StartMessage(B_ERROR);
				fLink.Flush();
				break;
			}
			
			area_id owningArea = area_for(sharedmem);
			area_info info;

			if (owningArea == B_ERROR || get_area_info(owningArea, &info) < B_OK) {
				fLink.StartMessage(B_ERROR);
				fLink.Flush();
				break;
			}

			int32 areaoffset = (addr_t)sharedmem - (addr_t)info.address;
			STRACE(("Successfully allocated shared memory of size %ld\n",memsize));

			fLink.StartMessage(B_OK);
			fLink.Attach<area_id>(owningArea);
			fLink.Attach<int32>(areaoffset);
			fLink.Flush();
			break;
		}
		case AS_RELEASE_SERVERMEM:
		{
			// Received when a ServerMemIO object on destruction
			// Attached Data:
			// 1) area_id owning area
			// 2) int32 area offset
			
			area_id owningArea;
			int32 areaoffset;
				
			link.Read<area_id>(&owningArea);
			link.Read<int32>(&areaoffset);
			
			area_info areaInfo;
			if (owningArea < 0 || get_area_info(owningArea, &areaInfo) != B_OK)
				break;
			
			STRACE(("Successfully freed shared memory\n"));
			void *sharedmem = ((int32*)areaInfo.address) + areaoffset;
			
			fSharedMem.ReleaseBuffer(sharedmem);
			
			break;
		}
		case AS_UPDATE_DECORATOR:
		{
			STRACE(("ServerApp %s: Received decorator update notification\n", Signature()));

			// TODO: implement or remove AS_UPDATE_DECORATOR
/*			for (int32 i = 0; i < fWindowList.CountItems(); i++) {
				ServerWindow *window = fWindowList.ItemAt(i);
				window->Lock();
				const_cast<WindowLayer *>(window->Window())->UpdateDecorator();
				window->Unlock();
			}
*/			break;
		}
		case AS_SET_DECORATOR:
		{
			// Received from an application when the user wants to set the window
			// decorator to a new one

			// Attached Data:
			// int32 the index of the decorator to use

			int32 index;
			link.Read<int32>(&index);
			if (gDecorManager.SetDecorator(index))
				fDesktop->BroadcastToAllApps(AS_UPDATE_DECORATOR);
			break;
		}
		case AS_COUNT_DECORATORS:
		{
			fLink.StartMessage(B_OK);
			fLink.Attach<int32>(gDecorManager.CountDecorators());
			fLink.Flush();
			break;
		}
		case AS_GET_DECORATOR:
		{
			fLink.StartMessage(B_OK);
			fLink.Attach<int32>(gDecorManager.GetDecorator());
			fLink.Flush();
			break;
		}
		case AS_GET_DECORATOR_NAME:
		{
			int32 index;
			link.Read<int32>(&index);

			BString str(gDecorManager.GetDecoratorName(index));
			if (str.CountChars() > 0) {
				fLink.StartMessage(B_OK);
				fLink.AttachString(str.String());
			} else
				fLink.StartMessage(B_ERROR);

			fLink.Flush();
			break;
		}
		case AS_R5_SET_DECORATOR:
		{
			// Sort of supports Tracker's nifty Easter Egg. It was easy to do and 
			// it's kind of neat, so why not?

			// Attached Data:
			// int32 value of the decorator to use
			// 0: BeOS
			// 1: Amiga
			// 2: Windows
			// 3: MacOS

			int32 decindex = 0;
			link.Read<int32>(&decindex);

			if (gDecorManager.SetR5Decorator(decindex))
				fDesktop->BroadcastToAllApps(AS_UPDATE_DECORATOR);
			
			break;
		}
		case AS_CREATE_BITMAP:
		{
			STRACE(("ServerApp %s: Received BBitmap creation request\n", Signature()));
			// Allocate a bitmap for an application

			// Attached Data:
			// 1) BRect bounds
			// 2) color_space space
			// 3) int32 bitmap_flags
			// 4) int32 bytes_per_row
			// 5) int32 screen_id::id
			
			// Reply Data:
			//	1) int32 server token
			//	2) area_id id of the area in which the bitmap data resides
			//	3) int32 area pointer offset used to calculate fBasePtr
			
			// First, let's attempt to allocate the bitmap
			ServerBitmap *bitmap = NULL;
			BRect frame;
			color_space colorSpace;
			int32 flags, bytesPerRow;
			screen_id screenID;

			link.Read<BRect>(&frame);
			link.Read<color_space>(&colorSpace);
			link.Read<int32>(&flags);
			link.Read<int32>(&bytesPerRow);
			if (link.Read<screen_id>(&screenID) == B_OK) {
				bitmap = gBitmapManager->CreateBitmap(frame, colorSpace, flags,
													  bytesPerRow, screenID);
			}

			STRACE(("ServerApp %s: Create Bitmap (%.1fx%.1f)\n",
						Signature(), frame.Width(), frame.Height()));

			if (bitmap != NULL && fBitmapList.AddItem(bitmap)) {
				fLink.StartMessage(B_OK);
				fLink.Attach<int32>(bitmap->Token());
				fLink.Attach<area_id>(bitmap->Area());
				fLink.Attach<int32>(bitmap->AreaOffset());
			} else
				fLink.StartMessage(B_NO_MEMORY);

			fLink.Flush();
			break;
		}
		case AS_DELETE_BITMAP:
		{
			STRACE(("ServerApp %s: received BBitmap delete request\n", Signature()));
			// Delete a bitmap's allocated memory

			// Attached Data:
			// 1) int32 token
			int32 token;
			link.Read<int32>(&token);

			ServerBitmap *bitmap = FindBitmap(token);
			if (bitmap && fBitmapList.RemoveItem(bitmap)) {
				STRACE(("ServerApp %s: Deleting Bitmap %ld\n", Signature(), token));

				gBitmapManager->DeleteBitmap(bitmap);
			}
			break;
		}
		case AS_GET_BITMAP_OVERLAY_RESTRICTIONS:
		{
			overlay_restrictions overlayRestrictions;
			status_t status = B_ERROR;

			int32 token;
			if (link.Read<int32>(&token) != B_OK)
				break;

			ServerBitmap *bitmap = FindBitmap(token);
			if (bitmap != NULL) {
				STRACE(("ServerApp %s: Get overlay restrictions for bitmap %ld\n",
					Signature(), token));

				// TODO: fill overlay restrictions
			}

			fLink.StartMessage(status);
			if (status == B_OK)
				fLink.Attach(&overlayRestrictions, sizeof(overlay_restrictions));

			fLink.Flush();
			break;
		}

		case AS_CREATE_PICTURE:
		{
			// TODO: Maybe rename this to AS_UPLOAD_PICTURE ?
			STRACE(("ServerApp %s: Create Picture\n", Signature()));
			ServerPicture *picture = CreatePicture();
			if (picture != NULL) {
				int32 subPicturesCount = 0;
				link.Read<int32>(&subPicturesCount);
				for (int32 c = 0; c < subPicturesCount; c++) {
					// TODO: Support nested pictures
				} 
				
				int32 size = 0;
				link.Read<int32>(&size);
				link.Read(const_cast<void *>(picture->Data()), size);
				
				fLink.StartMessage(B_OK);
				fLink.Attach<int32>(picture->Token());
			} else
				fLink.StartMessage(B_ERROR);

			fLink.Flush();
			break;
		}

		case AS_DELETE_PICTURE:
		{
			STRACE(("ServerApp %s: Delete Picture\n", Signature()));
			int32 token;
			if (link.Read<int32>(&token) == B_OK)
				DeletePicture(token);

			break;
		}

		case AS_CLONE_PICTURE:
		{
			STRACE(("ServerApp %s: Clone Picture\n", Signature()));
			int32 token;
			ServerPicture *original = NULL;
			if (link.Read<int32>(&token) == B_OK)
				original = FindPicture(token);
			
			ServerPicture *cloned = NULL;
			if (original != NULL)
				cloned = CreatePicture(original);
		
			if (cloned != NULL) {
				fLink.StartMessage(B_OK);
				fLink.Attach<int32>(cloned->Token());	
			} else
				fLink.StartMessage(B_ERROR);

			fLink.Flush();	
			break;
		}

		case AS_DOWNLOAD_PICTURE:
		{
			STRACE(("ServerApp %s: Download Picture\n", Signature()));
			int32 token;
			link.Read<int32>(&token);
			ServerPicture *picture = FindPicture(token);
			if (picture != NULL) {
				fLink.StartMessage(B_OK);
				
				// TODO: support nested pictures (subpictures)
				fLink.Attach<int32>(0); // number of subpictures
				fLink.Attach<int32>(picture->DataLength());
				fLink.Attach(picture->Data(), picture->DataLength());
			} else
				fLink.StartMessage(B_ERROR);
			
			fLink.Flush();
			
			break;
		}

		case AS_COUNT_WORKSPACES:
		{
			DesktopSettings settings(fDesktop);

			fLink.StartMessage(B_OK);
			fLink.Attach<int32>(settings.WorkspacesCount());
			fLink.Flush();
			break;
		}

		case AS_SET_WORKSPACE_COUNT:
		{
			DesktopSettings settings(fDesktop);

			int32 newCount;
			if (link.Read<int32>(&newCount) == B_OK) {
				settings.SetWorkspacesCount(newCount);

				// either update the workspaces window, or switch to
				// the last available workspace - which will update
				// the workspaces window automatically
				if (fDesktop->CurrentWorkspace() >= newCount)
					fDesktop->SetWorkspace(newCount - 1);
				else
					fDesktop->UpdateWorkspaces();
			}
			break;
		}

		case AS_CURRENT_WORKSPACE:
			STRACE(("ServerApp %s: get current workspace\n", Signature()));

			fLink.StartMessage(B_OK);
			fLink.Attach<int32>(fDesktop->CurrentWorkspace());
			fLink.Flush();
			break;

		case AS_ACTIVATE_WORKSPACE:
		{
			STRACE(("ServerApp %s: activate workspace\n", Signature()));
			
			// TODO: See above
			int32 index;
			link.Read<int32>(&index);

			fDesktop->SetWorkspace(index);
			break;
		}

		case AS_SHOW_CURSOR:
		{
			STRACE(("ServerApp %s: Show Cursor\n", Signature()));
			fCursorHideLevel--;
			if (fCursorHideLevel < 0)
				fCursorHideLevel = 0;
			fDesktop->HWInterface()->SetCursorVisible(fCursorHideLevel == 0);
			break;
		}
		case AS_HIDE_CURSOR:
		{
			STRACE(("ServerApp %s: Hide Cursor\n", Signature()));
			fCursorHideLevel++;
			fDesktop->HWInterface()->SetCursorVisible(fCursorHideLevel == 0);
			break;
		}
		case AS_OBSCURE_CURSOR:
		{
			STRACE(("ServerApp %s: Obscure Cursor\n", Signature()));
			fDesktop->HWInterface()->ObscureCursor();
			break;
		}
		case AS_QUERY_CURSOR_HIDDEN:
		{
			STRACE(("ServerApp %s: Received IsCursorHidden request\n", Signature()));
			fLink.StartMessage(fCursorHideLevel > 0 ? B_OK : B_ERROR);
			fLink.Flush();
			break;
		}
		case AS_SET_CURSOR:
		{
			STRACE(("ServerApp %s: SetCursor\n", Signature()));
			// Attached data:
			// 1) bool flag to send a reply
			// 2) int32 token ID of the cursor to set
			// 3) port_id port to receive a reply. Only exists if the sync flag is true.
			bool sync;
			int32 token = B_NULL_TOKEN;

			link.Read<bool>(&sync);
			link.Read<int32>(&token);

			fAppCursor = fDesktop->GetCursorManager().FindCursor(token);
			if (fAppCursor != NULL)
				fDesktop->HWInterface()->SetCursor(fAppCursor);
			else {
				// if there is no new cursor, we just set the default cursor
				fDesktop->HWInterface()->SetCursor(fDesktop->GetCursorManager().GetCursor(B_CURSOR_DEFAULT));
			}

			if (sync) {
				// The application is expecting a reply
				fLink.StartMessage(B_OK);
				fLink.Flush();
			}
			break;
		}
		case AS_CREATE_CURSOR:
		{
			STRACE(("ServerApp %s: Create Cursor\n", Signature()));
			// Attached data:
			// 1) 68 bytes of fAppCursor data
			// 2) port_id reply port

			status_t status = B_ERROR;
			uint8 cursorData[68];
			ServerCursor* cursor = NULL;

//			if (link.Read(cursorData, sizeof(cursorData)) >= B_OK) {
//				cursor = new (nothrow) ServerCursor(cursorData);
//				if (cursor == NULL)
//					status = B_NO_MEMORY;
//			}
//
//			if (cursor != NULL) {
//				cursor->SetOwningTeam(fClientTeam);
//				fDesktop->GetCursorManager().AddCursor(cursor);
//
//				// Synchronous message - BApplication is waiting on the cursor's ID
//				fLink.StartMessage(B_OK);
//				fLink.Attach<int32>(cursor->Token());
//			} else
//				fLink.StartMessage(status);

			if (link.Read(cursorData, sizeof(cursorData)) >= B_OK) {
				cursor = fDesktop->GetCursorManager().CreateCursor(fClientTeam,
																   cursorData);
				if (cursor == NULL)
					status = B_NO_MEMORY;
			}

			if (cursor != NULL) {
				// Synchronous message - BApplication is waiting on the cursor's ID
				fLink.StartMessage(B_OK);
				fLink.Attach<int32>(cursor->Token());
			} else
				fLink.StartMessage(status);

			fLink.Flush();
			break;
		}
		case AS_DELETE_CURSOR:
		{
			STRACE(("ServerApp %s: Delete BCursor\n", Signature()));
			// Attached data:
			// 1) int32 token ID of the cursor to delete
			int32 token;
			bool pendingViewCursor;
			link.Read<int32>(&token);
			if (link.Read<bool>(&pendingViewCursor) != B_OK)
				break;

			if (fAppCursor && fAppCursor->Token() == token)
				fAppCursor = NULL;

			ServerCursor* cursor = fDesktop->GetCursorManager().FindCursor(token);
			if (cursor) {
				if (pendingViewCursor)
					cursor->SetPendingViewCursor(true);

				cursor->Release();
			}
			break;
		}

		case AS_GET_SCROLLBAR_INFO:
		{
			STRACE(("ServerApp %s: Get ScrollBar info\n", Signature()));
			scroll_bar_info info;
			DesktopSettings settings(fDesktop);
			settings.GetScrollBarInfo(info);

			fLink.StartMessage(B_OK);
			fLink.Attach<scroll_bar_info>(info);
			fLink.Flush();
			break;
		}
		case AS_SET_SCROLLBAR_INFO:
		{
			STRACE(("ServerApp %s: Set ScrollBar info\n", Signature()));
			// Attached Data:
			// 1) scroll_bar_info scroll bar info structure
			scroll_bar_info info;
			if (link.Read<scroll_bar_info>(&info) == B_OK) {
				DesktopSettings settings(fDesktop);
				settings.SetScrollBarInfo(info);
			}

			fLink.StartMessage(B_OK);
			fLink.Flush();
			break;
		}

		case AS_GET_MENU_INFO:
		{
			STRACE(("ServerApp %s: Get menu info\n", Signature()));
			menu_info info;
			DesktopSettings settings(fDesktop);
			settings.GetMenuInfo(info);

			fLink.StartMessage(B_OK);
			fLink.Attach<menu_info>(info);
			fLink.Flush();
			break;
		}
		case AS_SET_MENU_INFO:
		{
			STRACE(("ServerApp %s: Set menu info\n", Signature()));
			menu_info info;
			if (link.Read<menu_info>(&info) == B_OK) {
				DesktopSettings settings(fDesktop);
				settings.SetMenuInfo(info);
					// TODO: SetMenuInfo() should do some validity check, so
					//	that the answer we're giving can actually be useful
			}

			fLink.StartMessage(B_OK);
			fLink.Flush();
			break;
		}

		case AS_SET_MOUSE_MODE:
		{
			STRACE(("ServerApp %s: Set Focus Follows Mouse mode\n", Signature()));
			// Attached Data:
			// 1) enum mode_mouse FFM mouse mode
			mode_mouse mouseMode;
			if (link.Read<mode_mouse>(&mouseMode) == B_OK) {
				DesktopSettings settings(fDesktop);
				settings.SetMouseMode(mouseMode);
			}
			break;
		}
		case AS_GET_MOUSE_MODE:
		{
			STRACE(("ServerApp %s: Get Focus Follows Mouse mode\n", Signature()));
			DesktopSettings settings(fDesktop);

			fLink.StartMessage(B_OK);
			fLink.Attach<mode_mouse>(settings.MouseMode());
			fLink.Flush();
			break;
		}

		case AS_GET_UI_COLORS:
		{
			// Client application is asking for all the system colors at once
			// using a ColorSet object
			gGUIColorSet.Lock();
			
			fLink.StartMessage(B_OK);
			fLink.Attach<ColorSet>(gGUIColorSet);
			fLink.Flush();
			
			gGUIColorSet.Unlock();
			break;
		}
		case AS_SET_UI_COLORS:
		{
			// Client application is asking to set all the system colors at once
			// using a ColorSet object

			// Attached data:
			// 1) ColorSet new colors to use
			gGUIColorSet.Lock();
			link.Read<ColorSet>(&gGUIColorSet);
			gGUIColorSet.Unlock();

			fDesktop->BroadcastToAllApps(AS_UPDATE_COLORS);
			break;
		}
		case AS_GET_UI_COLOR:
		{
			STRACE(("ServerApp %s: Get UI color\n", Signature()));

			RGBColor color;
			int32 whichColor;
			link.Read<int32>(&whichColor);

			gGUIColorSet.Lock();
			color = gGUIColorSet.AttributeToColor(whichColor);
			gGUIColorSet.Unlock();

			fLink.StartMessage(B_OK);
			fLink.Attach<rgb_color>(color.GetColor32());
			fLink.Flush();
			break;
		}

		/* font messages */

		case AS_SET_SYSTEM_FONT:
		{
			// gets:
			//	1) string - font type ("plain", ...)
			//	2) string - family
			//	3) string - style
			//	4) float - size
			
			char type[B_OS_NAME_LENGTH];
			font_family familyName;
			font_style styleName;
			float size;

			if (link.ReadString(type, sizeof(type)) == B_OK
				&& link.ReadString(familyName, sizeof(familyName)) == B_OK
				&& link.ReadString(styleName, sizeof(styleName)) == B_OK
				&& link.Read<float>(&size) == B_OK) {
				BAutolock locker(gFontManager);

				FontStyle* style = gFontManager->GetStyle(familyName, styleName);
				if (style != NULL) {
					ServerFont font(*style, size);
					DesktopSettings settings(fDesktop);

					if (!strcmp(type, "plain"))
						settings.SetDefaultPlainFont(font);
					else if (!strcmp(type, "bold"))
						settings.SetDefaultBoldFont(font);
					else if (!strcmp(type, "fixed"))
						settings.SetDefaultFixedFont(font);
				}
			}
			break;
		}
		case AS_GET_SYSTEM_DEFAULT_FONT:
		{
			// input:
			//	1) string - font type ("plain", ...)

			ServerFont font;

			char type[B_OS_NAME_LENGTH];
			status_t status = link.ReadString(type, sizeof(type));
			if (status == B_OK) {
				if (!strcmp(type, "plain")) {
					font = *gFontManager->DefaultPlainFont();
				} else if (!strcmp(type, "bold")) {
					font = *gFontManager->DefaultBoldFont();
				} else if (!strcmp(type, "fixed")) {
					font = *gFontManager->DefaultFixedFont();
				} else
					status = B_BAD_VALUE;
			}

			if (status == B_OK) {
				// returns:
				//	1) string - family
				//	2) string - style
				//	3) float - size

				fLink.StartMessage(B_OK);
				fLink.AttachString(font.Family());
				fLink.AttachString(font.Style());
				fLink.Attach<float>(font.Size());
			} else
				fLink.StartMessage(status);

			fLink.Flush();
			break;
		}
		case AS_GET_SYSTEM_FONTS:
		{
			FTRACE(("ServerApp %s: AS_GET_SYSTEM_FONTS\n", Signature()));
			// Returns:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) float - size in points
			// 4) uint16 - face flags
			// 5) uint32 - font flags

			DesktopSettings settings(fDesktop);
			fLink.StartMessage(B_OK);

			for (int32 i = 0; i < 3; i++) {
				ServerFont font;
				switch (i) {
					case 0:
						settings.GetDefaultPlainFont(font);
						fLink.AttachString("plain");
						break;
					case 1:
						settings.GetDefaultBoldFont(font);
						fLink.AttachString("bold");
						break;
					case 2:
						settings.GetDefaultFixedFont(font);
						fLink.AttachString("fixed");
						break;
				}

				fLink.Attach<uint16>(font.FamilyID());
				fLink.Attach<uint16>(font.StyleID());
				fLink.Attach<float>(font.Size());
				fLink.Attach<uint16>(font.Face());
				fLink.Attach<uint32>(font.Flags());
			}

			fLink.Flush();
			break;
		}
		case AS_GET_FONT_LIST_REVISION:
		{
			STRACE(("ServerApp %s: AS_GET_FONT_LIST_REVISION\n", Signature()));

			fLink.StartMessage(B_OK);
			fLink.Attach<int32>(gFontManager->CheckRevision(fDesktop->UserID()));
			fLink.Flush();
			break;
		}
		case AS_GET_FAMILY_AND_STYLES:
		{
			FTRACE(("ServerApp %s: AS_GET_FAMILY_AND_STYLES\n", Signature()));
			// Attached Data:
			// 1) int32 the index of the font family to get

			// Returns:
			// 1) string - name of family
			// 2) uint32 - flags of font family (B_IS_FIXED || B_HAS_TUNED_FONT)
			// 3) count of styles in that family
			// For each style:
			//		1) string - name of style
			//		2) uint16 - face of style
			//		3) uint32 - flags of style

			int32 index;
			link.Read<int32>(&index);

			gFontManager->Lock();

			FontFamily* family = gFontManager->FamilyAt(index);
			if (family) {
				fLink.StartMessage(B_OK);
				fLink.AttachString(family->Name());
				fLink.Attach<uint32>(family->Flags());
				
				int32 count = family->CountStyles();
				fLink.Attach<int32>(count);

				for (int32 i = 0; i < count; i++) {
					FontStyle* style = family->StyleAt(i);

					fLink.AttachString(style->Name());
					fLink.Attach<uint16>(style->Face());
					fLink.Attach<uint32>(style->Flags());
				}
			} else
				fLink.StartMessage(B_BAD_VALUE);

			gFontManager->Unlock();
			fLink.Flush();
			break;
		}
		case AS_GET_FAMILY_AND_STYLE:
		{
			FTRACE(("ServerApp %s: AS_GET_FAMILY_AND_STYLE\n", Signature()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID

			// Returns:
			// 1) font_family The name of the font family
			// 2) font_style - name of the style
			uint16 familyID, styleID;
			link.Read<uint16>(&familyID);
			link.Read<uint16>(&styleID);

			gFontManager->Lock();

			FontStyle *fontStyle = gFontManager->GetStyle(familyID, styleID);
			if (fontStyle != NULL) {
				fLink.StartMessage(B_OK);
				fLink.AttachString(fontStyle->Family()->Name());
				fLink.AttachString(fontStyle->Name());
			} else
				fLink.StartMessage(B_BAD_VALUE);

			fLink.Flush();
			gFontManager->Unlock();
			break;
		}
		case AS_GET_FAMILY_AND_STYLE_IDS:
		{
			FTRACE(("ServerApp %s: AS_GET_FAMILY_AND_STYLE_IDS\n", Signature()));
			// Attached Data:
			// 1) font_family - name of font family to use
			// 2) font_style - name of style in family
			// 3) family ID - only used if 1) is empty
			// 4) style ID - only used if 2) is empty
			// 5) face - the font's current face

			// Returns:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) uint16 - face

			font_family family;
			font_style style;
			uint16 familyID, styleID;
			uint16 face;
			if (link.ReadString(family, sizeof(font_family)) == B_OK
				&& link.ReadString(style, sizeof(font_style)) == B_OK
				&& link.Read<uint16>(&familyID) == B_OK
				&& link.Read<uint16>(&styleID) == B_OK
				&& link.Read<uint16>(&face) == B_OK) {
				// get the font and return IDs and face
				gFontManager->Lock();

				FontStyle *fontStyle = gFontManager->GetStyle(family, style,
					familyID, styleID, face);

				if (fontStyle != NULL) {
					fLink.StartMessage(B_OK);
					fLink.Attach<uint16>(fontStyle->Family()->ID());
					fLink.Attach<uint16>(fontStyle->ID());

					// we try to keep the font face close to what we got
					face = fontStyle->PreservedFace(face);

					fLink.Attach<uint16>(face);
				} else
					fLink.StartMessage(B_NAME_NOT_FOUND);

				gFontManager->Unlock();
			} else
				fLink.StartMessage(B_BAD_VALUE);

			fLink.Flush();
			break;
		}
		case AS_GET_FONT_FILE_FORMAT:
		{
			FTRACE(("ServerApp %s: AS_GET_FONT_FILE_FORMAT\n", Signature()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID

			// Returns:
			// 1) uint16 font_file_format of font

			int32 familyID, styleID;
			link.Read<int32>(&familyID);
			link.Read<int32>(&styleID);

			gFontManager->Lock();

			FontStyle *fontStyle = gFontManager->GetStyle(familyID, styleID);
			if (fontStyle) {
				fLink.StartMessage(B_OK);
				fLink.Attach<uint16>((uint16)fontStyle->FileFormat());
			} else
				fLink.StartMessage(B_BAD_VALUE);

			gFontManager->Unlock();
			fLink.Flush();
			break;
		}
		case AS_GET_STRING_WIDTHS:
		{
			FTRACE(("ServerApp %s: AS_GET_STRING_WIDTHS\n", Signature()));
			// Attached Data:
			// 1) uint16 ID of family
			// 2) uint16 ID of style
			// 3) float point size of font
			// 4) uint8 spacing to use
			// 5) int32 numStrings 
			// 6) int32 string length to measure (numStrings times)
			// 7) string String to measure (numStrings times)

			// Returns:
			// 1) float - width of the string in pixels (numStrings times)

			uint16 family, style;
			float size;
			uint8 spacing;

			link.Read<uint16>(&family);
			link.Read<uint16>(&style);
			link.Read<float>(&size);
			link.Read<uint8>(&spacing);
			int32 numStrings;
			if (link.Read<int32>(&numStrings) != B_OK) {
				// this results in a B_BAD_VALUE return
				numStrings = 0;
				size = 0.0f;
			}

			float widthArray[numStrings];
			int32 lengthArray[numStrings];
			char *stringArray[numStrings];
			for (int32 i = 0; i < numStrings; i++) {
				link.ReadString(&stringArray[i], (size_t *)&lengthArray[i]);
			}

			ServerFont font;

			if (font.SetFamilyAndStyle(family, style) == B_OK && size > 0) {
				font.SetSize(size);
				font.SetSpacing(spacing);

				for (int32 i = 0; i < numStrings; i++) {
					if (!stringArray[i] || lengthArray[i] <= 0)
						widthArray[i] = 0.0;
					else {
						widthArray[i] = fDesktop->GetDrawingEngine()->StringWidth(stringArray[i], lengthArray[i], font);
						// NOTE: The line below will return the exact same thing. However,
						// the line above uses the AGG rendering backend, for which glyph caching
						// actually works. It is about 20 times faster!
						// TODO: I've disabled the AGG version for now, as it produces a dead lock
						//	(font use), that I am currently too lazy to investigate...
//						widthArray[i] = font.StringWidth(stringArray[i], lengthArray[i]);
					}
				}

				fLink.StartMessage(B_OK);
				fLink.Attach(widthArray, sizeof(widthArray));
			} else
				fLink.StartMessage(B_BAD_VALUE);

			fLink.Flush();

			for (int32 i = 0; i < numStrings; i++) {
				free(stringArray[i]);
			}
			break;
		}
		case AS_GET_FONT_BOUNDING_BOX:
		{
			FTRACE(("ServerApp %s: AS_GET_BOUNDING_BOX unimplemented\n",
				Signature()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID

			// Returns:
			// 1) BRect - box holding entire font

			// ToDo: implement me!
			fLink.StartMessage(B_ERROR);
			fLink.Flush();
			break;
		}
		case AS_GET_TUNED_COUNT:
		{
			FTRACE(("ServerApp %s: AS_GET_TUNED_COUNT\n", Signature()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID

			// Returns:
			// 1) int32 - number of font strikes available
			uint16 familyID, styleID;
			link.Read<uint16>(&familyID);
			link.Read<uint16>(&styleID);

			gFontManager->Lock();

			FontStyle *fontStyle = gFontManager->GetStyle(familyID, styleID);
			if (fontStyle != NULL) {
				fLink.StartMessage(B_OK);
				fLink.Attach<int32>(fontStyle->TunedCount());
			} else
				fLink.StartMessage(B_BAD_VALUE);

			gFontManager->Unlock();
			fLink.Flush();
			break;
		}
		case AS_GET_TUNED_INFO:
		{
			FTRACE(("ServerApp %s: AS_GET_TUNED_INFO unimplmemented\n",
				Signature()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) uint32 - index of the particular font strike

			// Returns:
			// 1) tuned_font_info - info on the strike specified
			// ToDo: implement me!
			fLink.StartMessage(B_ERROR);
			fLink.Flush();
			break;
		}
		case AS_GET_EXTRA_FONT_FLAGS:
		{
			FTRACE(("ServerApp %s: AS_GET_EXTRA_FONT_FLAGS\n",
				Signature()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID

			// Returns:
			// 1) uint32 - extra font flags
			uint16 familyID, styleID;
			link.Read<uint16>(&familyID);
			link.Read<uint16>(&styleID);

			gFontManager->Lock();

			FontStyle *fontStyle = gFontManager->GetStyle(familyID, styleID);
			if (fontStyle != NULL) {
				fLink.StartMessage(B_OK);
				fLink.Attach<uint32>(fontStyle->Flags());
			} else
				fLink.StartMessage(B_BAD_VALUE);

			gFontManager->Unlock();
			fLink.Flush();
			break;
		}
		case AS_GET_FONT_HEIGHT:
		{
			FTRACE(("ServerApp %s: AS_GET_FONT_HEIGHT\n", Signature()));
			// Attached Data:
			// 1) uint16 family ID
			// 2) uint16 style ID
			// 3) float size
			uint16 familyID, styleID;
			float size;
			link.Read<uint16>(&familyID);
			link.Read<uint16>(&styleID);
			link.Read<float>(&size);

			gFontManager->Lock();

			FontStyle *fontStyle = gFontManager->GetStyle(familyID, styleID);
			if (fontStyle != NULL) {
				font_height height;
				fontStyle->GetHeight(size, height);

				fLink.StartMessage(B_OK);
				fLink.Attach<font_height>(height);
			} else
				fLink.StartMessage(B_BAD_VALUE);

			gFontManager->Unlock();
			fLink.Flush();
			break;
		}
		case AS_GET_GLYPH_SHAPES:
		{
			FTRACE(("ServerApp %s: AS_GET_GLYPH_SHAPES\n", Signature()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) float - point size
			// 4) float - shear
			// 5) float - rotation
			// 6) uint32 - flags
			// 7) int32 - numChars
			// 8) int32 - numBytes
			// 8) char - chars (bytesInBuffer times)

			// Returns:
			// 1) BShape - glyph shape
			// numChars times

			uint16 familyID, styleID;
			uint32 flags;
			float size, shear, rotation;

			link.Read<uint16>(&familyID);
			link.Read<uint16>(&styleID);
			link.Read<float>(&size);
			link.Read<float>(&shear);
			link.Read<float>(&rotation);
			link.Read<uint32>(&flags);

			int32 numChars, numBytes;
			link.Read<int32>(&numChars);
			link.Read<int32>(&numBytes);

			char *charArray = new char[numBytes];
			link.Read(charArray, numBytes);

			ServerFont font;
			status_t status = font.SetFamilyAndStyle(familyID, styleID);
			if (status == B_OK) {
				font.SetSize(size);
				font.SetShear(shear);
				font.SetRotation(rotation);
				font.SetFlags(flags);

				BShape **shapes = new BShape *[numChars];
				status = font.GetGlyphShapes(charArray, numChars, shapes);
				if (status == B_OK) {
					fLink.StartMessage(B_OK);
					for (int32 i = 0; i < numChars; i++) {
						fLink.AttachShape(*shapes[i]);
						delete shapes[i];
					}

					delete[] shapes;
				} else
					fLink.StartMessage(status);
			} else
				fLink.StartMessage(status);

			delete[] charArray;
			fLink.Flush();
			break;
		}
		case AS_GET_HAS_GLYPHS:
		{
			FTRACE(("ServerApp %s: AS_GET_HAS_GLYPHS\n", Signature()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) int32 - numChars
			// 4) int32 - numBytes
			// 5) char - the char buffer with size numBytes

			uint16 familyID, styleID;
			link.Read<uint16>(&familyID);
			link.Read<uint16>(&styleID);

			int32 numChars, numBytes;
			link.Read<int32>(&numChars);
			link.Read<int32>(&numBytes);
			char* charArray = new char[numBytes];
			link.Read(charArray, numBytes);

			ServerFont font;
			status_t status = font.SetFamilyAndStyle(familyID, styleID);
			if (status == B_OK) {
				bool hasArray[numChars];
				status = font.GetHasGlyphs(charArray, numChars, hasArray);
				if (status == B_OK) {
					fLink.StartMessage(B_OK);
					fLink.Attach(hasArray, sizeof(hasArray));
				} else
					fLink.StartMessage(status);
			} else
				fLink.StartMessage(status);

			delete[] charArray;
			fLink.Flush();
			break;
		}
		case AS_GET_EDGES:
		{
			FTRACE(("ServerApp %s: AS_GET_EDGES\n", Signature()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) int32 - numChars
			// 4) int32 - numBytes
			// 5) char - the char buffer with size numBytes
			
			uint16 familyID, styleID;
			link.Read<uint16>(&familyID);
			link.Read<uint16>(&styleID);

			int32 numChars;
			link.Read<int32>(&numChars);

			uint32 numBytes;
			link.Read<uint32>(&numBytes);
			char* charArray = new char[numBytes];
			link.Read(charArray, numBytes);

			ServerFont font;
			status_t status = font.SetFamilyAndStyle(familyID, styleID);
			if (status == B_OK) {
				edge_info edgeArray[numChars];
				status = font.GetEdges(charArray, numChars, edgeArray);
				if (status == B_OK) {
					fLink.StartMessage(B_OK);
					fLink.Attach(edgeArray, sizeof(edgeArray));
				} else
					fLink.StartMessage(status);
			} else
				fLink.StartMessage(status);

			delete[] charArray;
			fLink.Flush();
			break;
		}
		case AS_GET_ESCAPEMENTS:
		{
			FTRACE(("ServerApp %s: AS_GET_ESCAPEMENTS\n", Signature()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) float - point size
			// 4) float - rotation
			// 5) uint32 - flags
			// 6) int32 - numChars
			// 7) char - char     -\       both
			// 8) BPoint - offset -/ (numChars times)

			// Returns:
			// 1) BPoint - escapement
			// numChars times

			uint16 familyID, styleID;
			uint32 flags;
			float size, rotation;

			link.Read<uint16>(&familyID);
			link.Read<uint16>(&styleID);
			link.Read<float>(&size);
			link.Read<float>(&rotation);
			link.Read<uint32>(&flags);

			escapement_delta delta;
			link.Read<float>(&delta.nonspace);
			link.Read<float>(&delta.space);

			bool wantsOffsets;
			link.Read<bool>(&wantsOffsets);

			int32 numChars;
			link.Read<int32>(&numChars);

			uint32 numBytes;
			link.Read<uint32>(&numBytes);
			char *charArray = new char[numBytes];
			link.Read(charArray, numBytes);

			ServerFont font;
			status_t status = font.SetFamilyAndStyle(familyID, styleID);
			if (status == B_OK) {
				font.SetSize(size);
				font.SetRotation(rotation);
				font.SetFlags(flags);

				BPoint *escapements = new BPoint[numChars];
				BPoint *offsets = NULL;
				if (wantsOffsets)
					offsets = new BPoint[numChars];

				status = font.GetEscapements(charArray, numChars, delta,
					escapements, offsets);

				if (status == B_OK) {
					fLink.StartMessage(B_OK);
					for (int32 i = 0; i < numChars; i++)
						fLink.Attach<BPoint>(escapements[i]);

					delete[] escapements;

					if (wantsOffsets) {
						for (int32 i = 0; i < numChars; i++)
							fLink.Attach<BPoint>(offsets[i]);

						delete[] offsets;
					}
				} else
					fLink.StartMessage(status);
			} else
				fLink.StartMessage(status);

			delete[] charArray;
			fLink.Flush();
			break;
		}
		case AS_GET_ESCAPEMENTS_AS_FLOATS:
		{
			FTRACE(("ServerApp %s: AS_GET_ESCAPEMENTS_AS_FLOATS\n", Signature()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) float - point size
			// 4) float - rotation
			// 5) uint32 - flags

			// 6) float - additional "nonspace" delta
			// 7) float - additional "space" delta

			// 8) int32 - numChars
			// 9) int32 - numBytes
			// 10) char - the char buffer with size numBytes

			// Returns:
			// 1) float - escapement buffer with numChar entries
			
			uint16 familyID, styleID;
			uint32 flags;
			float size, rotation;
			
			link.Read<uint16>(&familyID);
			link.Read<uint16>(&styleID);
			link.Read<float>(&size);
			link.Read<float>(&rotation);
			link.Read<uint32>(&flags);

			escapement_delta delta;
			link.Read<float>(&delta.nonspace);
			link.Read<float>(&delta.space);
			
			int32 numChars;
			link.Read<int32>(&numChars);

			uint32 numBytes;
			link.Read<uint32>(&numBytes);
			char* charArray = new char[numBytes];
			link.Read(charArray, numBytes);

			float* escapements = new float[numChars];

			// figure out escapements

			ServerFont font;
			status_t status = font.SetFamilyAndStyle(familyID, styleID);
			if (status == B_OK) {
				font.SetSize(size);
				font.SetRotation(rotation);
				font.SetFlags(flags);

				status = font.GetEscapements(charArray, numChars, delta,
					escapements);

				if (status == B_OK) {
					fLink.StartMessage(B_OK);
					fLink.Attach(escapements, numChars * sizeof(float));
				}
			}

			delete[] charArray;
			delete[] escapements;

			if (status != B_OK)
				fLink.StartMessage(status);

			fLink.Flush();
			break;
		}
		case AS_GET_BOUNDINGBOXES_CHARS:
		{
			FTRACE(("ServerApp %s: AS_GET_BOUNDINGBOXES_CHARS\n", Signature()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) float - point size
			// 4) float - rotation
			// 5) float - shear
			// 6) uint8 - spacing
			// 7) uint32 - flags
			
			// 8) font_metric_mode - mode
			// 9) bool - string escapement

			// 10) escapement_delta - additional delta

			// 11) int32 - numChars
			// 12) int32 - numBytes
			// 13) char - the char buffer with size numBytes

			// Returns:
			// 1) BRect - rects with numChar entries
			
			uint16 famid, styid;
			uint32 flags;
			float ptsize, rotation, shear;
			uint8 spacing;
			font_metric_mode mode;
			bool string_escapement;
			
			link.Read<uint16>(&famid);
			link.Read<uint16>(&styid);
			link.Read<float>(&ptsize);
			link.Read<float>(&rotation);
			link.Read<float>(&shear);
			link.Read<uint8>(&spacing);
			link.Read<uint32>(&flags);
			link.Read<font_metric_mode>(&mode);
			link.Read<bool>(&string_escapement);

			escapement_delta delta;
			link.Read<escapement_delta>(&delta);
			
			int32 numChars;
			link.Read<int32>(&numChars);

			uint32 numBytes;
			link.Read<uint32>(&numBytes);

			char *charArray = new char[numBytes];
			link.Read(charArray, numBytes);

			BRect rectArray[numChars];
			// figure out escapements

			ServerFont font;
			bool success = false;
			if (font.SetFamilyAndStyle(famid, styid) == B_OK) {
				font.SetSize(ptsize);
				font.SetRotation(rotation);
				font.SetShear(shear);
				font.SetSpacing(spacing);
				font.SetFlags(flags);

				// TODO implement for real
				if (font.GetBoundingBoxesAsString(charArray, numChars,
					rectArray, string_escapement, mode, delta) == B_OK) {
					fLink.StartMessage(B_OK);
					fLink.Attach(rectArray, sizeof(rectArray));
					success = true;
				}
			}

			if (!success)
				fLink.StartMessage(B_ERROR);

			delete[] charArray;
			fLink.Flush();
			break;
		}
		case AS_GET_BOUNDINGBOXES_STRINGS:
		{
			FTRACE(("ServerApp %s: AS_GET_BOUNDINGBOXES_STRINGS\n", Signature()));
			// Attached Data:
			// 1) uint16 - family ID
			// 2) uint16 - style ID
			// 3) float - point size
			// 4) float - rotation
			// 5) float - shear
			// 6) uint8 - spacing
			// 7) uint32 - flags
			
			// 8) font_metric_mode - mode
			// 9) int32 numStrings

			// 10) escapement_delta - additional delta (numStrings times)
			// 11) int32 string length to measure (numStrings times)
			// 12) string - string (numStrings times)

			// Returns:
			// 1) BRect - rects with numStrings entries
			
			uint16 famid, styid;
			uint32 flags;
			float ptsize, rotation, shear;
			uint8 spacing;
			font_metric_mode mode;
			
			link.Read<uint16>(&famid);
			link.Read<uint16>(&styid);
			link.Read<float>(&ptsize);
			link.Read<float>(&rotation);
			link.Read<float>(&shear);
			link.Read<uint8>(&spacing);
			link.Read<uint32>(&flags);
			link.Read<font_metric_mode>(&mode);

			int32 numStrings;
			link.Read<int32>(&numStrings);
			
			escapement_delta deltaArray[numStrings];
			char *stringArray[numStrings];
			int32 lengthArray[numStrings];
			for(int32 i=0; i<numStrings; i++) {
				link.Read<int32>(&lengthArray[i]);
				link.Read<escapement_delta>(&deltaArray[i]);
				link.ReadString(&stringArray[i]);
			}

			BRect rectArray[numStrings];

			ServerFont font;
			bool success = false;
			if (font.SetFamilyAndStyle(famid, styid) == B_OK) {
				font.SetSize(ptsize);
				font.SetRotation(rotation);
				font.SetShear(shear);
				font.SetSpacing(spacing);
				font.SetFlags(flags);

				// TODO implement for real
				if (font.GetBoundingBoxesForStrings(stringArray, lengthArray,
					numStrings, rectArray, mode, deltaArray) == B_OK) {
					fLink.StartMessage(B_OK);
					fLink.Attach(rectArray, sizeof(rectArray));
					success = true;
				}
			}

			for (int32 i = 0; i < numStrings; i++)
				free(stringArray[i]);

			if (!success)
				fLink.StartMessage(B_ERROR);

			fLink.Flush();
			break;
		}

		/* screen commands */

		case AS_VALID_SCREEN_ID:
		{
			// Attached data
			// 1) screen_id screen
			screen_id id;
			if (link.Read<screen_id>(&id) == B_OK
				&& id.id == B_MAIN_SCREEN_ID.id)
				fLink.StartMessage(B_OK);
			else
				fLink.StartMessage(B_ERROR);

			fLink.Flush();
			break;
		}

		case AS_GET_NEXT_SCREEN_ID:
		{
			// Attached data
			// 1) screen_id screen
			screen_id id;
			link.Read<screen_id>(&id);

			// TODO: for now, just say we're the last one
			fLink.StartMessage(B_ENTRY_NOT_FOUND);
			fLink.Flush();
			break;
		}

		case AS_GET_SCREEN_ID_FROM_WINDOW:
		{
			status_t status = B_ENTRY_NOT_FOUND;

			// Attached data
			// 1) int32 - window client token
			int32 clientToken;
			if (link.Read<int32>(&clientToken) != B_OK)
				status = B_BAD_DATA;
			else {
				BAutolock locker(fWindowListLock);

				for (int32 i = fWindowList.CountItems(); i-- > 0;) {
					ServerWindow* window = fWindowList.ItemAt(i);

					if (window->ClientToken() == clientToken) {
						// got it!
						fLink.StartMessage(B_OK);
						fLink.Attach<screen_id>(B_MAIN_SCREEN_ID);
							// TODO: for now...
						status = B_OK;
					}
				}
			}

			if (status != B_OK)
				fLink.StartMessage(status);
			fLink.Flush();
			break;
		}

		case AS_SCREEN_GET_MODE:
		{
			STRACE(("ServerApp %s: AS_SCREEN_GET_MODE\n", Signature()));
			// Attached data
			// 1) screen_id screen
			// 2) uint32 workspace index
			screen_id id;
			link.Read<screen_id>(&id);
			uint32 workspace;
			link.Read<uint32>(&workspace);

			// TODO: the display_mode can be different between
			// the various screens.
			// We have the screen_id and the workspace number, with these
			// we need to find the corresponding "driver", and call getmode on it
			display_mode mode;
			fDesktop->ScreenAt(0)->GetMode(&mode);
			// actually this isn't still enough as different workspaces can
			// have different display_modes

			fLink.StartMessage(B_OK);
			fLink.Attach<display_mode>(mode);
			fLink.Flush();
			break;
		}
		case AS_SCREEN_SET_MODE:
		{
			STRACE(("ServerApp %s: AS_SCREEN_SET_MODE\n", Signature()));
			// Attached data
			// 1) screen_id
			// 2) workspace index
			// 3) display_mode to set
			// 4) 'makedefault' boolean
			// TODO: See above: workspaces support, etc.

			screen_id id;
			link.Read<screen_id>(&id);

			uint32 workspace;
			link.Read<uint32>(&workspace);

			display_mode mode;
			link.Read<display_mode>(&mode);

			bool makeDefault = false;
			status_t status = link.Read<bool>(&makeDefault);

			if (status == B_OK && fDesktop->LockAllWindows()) {
				status = fDesktop->ScreenAt(0)->SetMode(mode, makeDefault);
				if (status == B_OK) {
					gInputManager->UpdateScreenBounds(fDesktop->ScreenAt(0)->Frame());
					fDesktop->ScreenChanged(fDesktop->ScreenAt(0), makeDefault);
				}
				fDesktop->UnlockAllWindows();
			} else
				status = B_ERROR;

			fLink.StartMessage(status);
			fLink.Flush();
			break;
		}

		case AS_PROPOSE_MODE:
		{
			STRACE(("ServerApp %s: AS_PROPOSE_MODE\n", Signature()));
			screen_id id;
			link.Read<screen_id>(&id);

			display_mode target, low, high;
			link.Read<display_mode>(&target);
			link.Read<display_mode>(&low);
			link.Read<display_mode>(&high);
			status_t status = fDesktop->HWInterface()->ProposeMode(&target, &low, &high);

			// ProposeMode() returns B_BAD_VALUE to hint that the candidate is
			// not within the given limits (but is supported)
			if (status == B_OK || status == B_BAD_VALUE) {
				fLink.StartMessage(B_OK);
				fLink.Attach<display_mode>(target);
				fLink.Attach<bool>(status == B_OK);
			} else
				fLink.StartMessage(status);

			fLink.Flush();
			break;
		}

		case AS_GET_MODE_LIST:
		{
			screen_id id;
			link.Read<screen_id>(&id);
			// TODO: use this screen id

			display_mode* modeList;
			uint32 count;
			status_t status = fDesktop->HWInterface()->GetModeList(&modeList, &count);
			if (status == B_OK) {
				fLink.StartMessage(B_OK);
				fLink.Attach<uint32>(count);
				fLink.Attach(modeList, sizeof(display_mode) * count);

				delete[] modeList;
			} else
				fLink.StartMessage(status);

			fLink.Flush();
			break;
		}

		case AS_SCREEN_GET_COLORMAP:
		{
			STRACE(("ServerApp %s: AS_SCREEN_GET_COLORMAP\n", Signature()));

			screen_id id;
			link.Read<screen_id>(&id);

			const color_map *colorMap = SystemColorMap();
			if (colorMap != NULL) {
				fLink.StartMessage(B_OK);
				fLink.Attach<color_map>(*colorMap);
			} else 
				fLink.StartMessage(B_ERROR);

			fLink.Flush();
			break;
		}

		case AS_GET_DESKTOP_COLOR:
		{
			STRACE(("ServerApp %s: get desktop color\n", Signature()));

			uint32 index;
			link.Read<uint32>(&index);

			fLink.StartMessage(B_OK);
			fDesktop->Lock();

			// we're nice to our children (and also take the default case
			// into account which asks for the current workspace)
			if (index >= (uint32)kMaxWorkspaces)
				index = fDesktop->CurrentWorkspace();

			Workspace workspace(*fDesktop, index);
			fLink.Attach<rgb_color>(workspace.Color().GetColor32());

			fDesktop->Unlock();
			fLink.Flush();
			break;
		}

		case AS_SET_DESKTOP_COLOR:
		{
			STRACE(("ServerApp %s: set desktop color\n", Signature()));

			rgb_color color;
			uint32 index;
			bool makeDefault;

			link.Read<rgb_color>(&color);
			link.Read<uint32>(&index);
			if (link.Read<bool>(&makeDefault) != B_OK)
				break;

			fDesktop->Lock();

			// we're nice to our children (and also take the default case
			// into account which asks for the current workspace)
			if (index >= (uint32)kMaxWorkspaces)
				index = fDesktop->CurrentWorkspace();

			Workspace workspace(*fDesktop, index);
			workspace.SetColor(color, makeDefault);

			fDesktop->Unlock();
			break;
		}

		case AS_GET_ACCELERANT_INFO:
		{
			STRACE(("ServerApp %s: get accelerant info\n", Signature()));

			// We aren't using the screen_id for now...
			screen_id id;
			link.Read<screen_id>(&id);

			accelerant_device_info accelerantInfo;
			// TODO: I wonder if there should be a "desktop" lock...
			status_t status = fDesktop->HWInterface()->GetDeviceInfo(&accelerantInfo);
			if (status == B_OK) {
				fLink.StartMessage(B_OK);
				fLink.Attach<accelerant_device_info>(accelerantInfo);
			} else
				fLink.StartMessage(status);

			fLink.Flush();
			break;
		}

		case AS_GET_FRAME_BUFFER_CONFIG:
		{
			STRACE(("ServerApp %s: get frame buffer config\n", Signature()));

			// We aren't using the screen_id for now...
			screen_id id;
			link.Read<screen_id>(&id);

			frame_buffer_config config;
			// TODO: I wonder if there should be a "desktop" lock...
			status_t status = fDesktop->HWInterface()->GetFrameBufferConfig(config);
			if (status == B_OK) {
				fLink.StartMessage(B_OK);
				fLink.Attach<frame_buffer_config>(config);
			} else
				fLink.StartMessage(status);

			fLink.Flush();
			break;
		}

		case AS_GET_RETRACE_SEMAPHORE:
		{
			STRACE(("ServerApp %s: get retrace semaphore\n", Signature()));

			// We aren't using the screen_id for now...
			screen_id id;
			link.Read<screen_id>(&id);

			sem_id semaphore = fDesktop->HWInterface()->RetraceSemaphore();
			fLink.StartMessage(semaphore);
			fLink.Flush();
			break;
		}

		case AS_GET_TIMING_CONSTRAINTS:
		{
			STRACE(("ServerApp %s: get timing constraints\n", Signature()));
			// We aren't using the screen_id for now...
			screen_id id;
			link.Read<screen_id>(&id);

			display_timing_constraints constraints;
			status_t status = fDesktop->HWInterface()->GetTimingConstraints(
				&constraints);
			if (status == B_OK) {
				fLink.StartMessage(B_OK);
				fLink.Attach<display_timing_constraints>(constraints);
			} else
				fLink.StartMessage(status);

			fLink.Flush();
			break;
		}

		case AS_GET_PIXEL_CLOCK_LIMITS:
		{
			STRACE(("ServerApp %s: get pixel clock limits\n", Signature()));
			// We aren't using the screen_id for now...
			screen_id id;
			link.Read<screen_id>(&id);
			display_mode mode;
			link.Read<display_mode>(&mode);

			uint32 low, high;
			status_t status = fDesktop->HWInterface()->GetPixelClockLimits(&mode,
				&low, &high);
			if (status == B_OK) {
				fLink.StartMessage(B_OK);
				fLink.Attach<uint32>(low);
				fLink.Attach<uint32>(high);
			} else
				fLink.StartMessage(status);

			fLink.Flush();
			break;
		}

		case AS_SET_DPMS:
		{
			STRACE(("ServerApp %s: AS_SET_DPMS\n", Signature()));
			screen_id id;
			link.Read<screen_id>(&id);

			uint32 mode;
			link.Read<uint32>(&mode);

			status_t status = fDesktop->HWInterface()->SetDPMSMode(mode);
			fLink.StartMessage(status);

			fLink.Flush();
			break;
		}

		case AS_GET_DPMS_STATE:
		{
			STRACE(("ServerApp %s: AS_GET_DPMS_STATE\n", Signature()));

			screen_id id;
			link.Read<screen_id>(&id);

			uint32 state = fDesktop->HWInterface()->DPMSMode();
			fLink.StartMessage(B_OK);
			fLink.Attach<uint32>(state);
			fLink.Flush();
			break;
		}

		case AS_GET_DPMS_CAPABILITIES:
		{
			STRACE(("ServerApp %s: AS_GET_DPMS_CAPABILITIES\n", Signature()));
			screen_id id;
			link.Read<screen_id>(&id);

			uint32 capabilities = fDesktop->HWInterface()->DPMSCapabilities();
			fLink.StartMessage(B_OK);
			fLink.Attach<uint32>(capabilities);
			fLink.Flush();
			break;
		}

		case AS_READ_BITMAP:
		{
			STRACE(("ServerApp %s: AS_READ_BITMAP\n", Signature()));
			int32 bitmapToken;
			link.Read<int32>(&bitmapToken);

			bool drawCursor = true;
			link.Read<bool>(&drawCursor);

			BRect bounds;
			link.Read<BRect>(&bounds);

			ServerBitmap *bitmap = FindBitmap(bitmapToken);
			if (bitmap != NULL) {
				fLink.StartMessage(B_OK);
				// TODO: Implement for real
			} else
				fLink.StartMessage(B_BAD_VALUE);

			fLink.Flush();
			break;
		}
		
		case AS_GET_ACCELERANT_PATH:
		{
			int32 index;
			fLink.Read<int32>(&index);
			
			BString path;
			status_t status = fDesktop->HWInterface()->GetAccelerantPath(path);
			fLink.StartMessage(status);
			if (status == B_OK)
				fLink.AttachString(path.String());
			
			fLink.Flush();
			break;
		}
		
		case AS_GET_DRIVER_PATH:
		{
			int32 index;
			fLink.Read<int32>(&index);
			
			BString path;
			status_t status = fDesktop->HWInterface()->GetDriverPath(path);
			fLink.StartMessage(status);
			if (status == B_OK)
				fLink.AttachString(path.String());
			
			fLink.Flush();
			break;
		}

		default:
			printf("ServerApp %s received unhandled message code %ld\n",
				Signature(), code);

			if (link.NeedsReply()) {
				// the client is now blocking and waiting for a reply!
				fLink.StartMessage(B_ERROR);
				fLink.Flush();
			} else
				puts("message doesn't need a reply!");
			break;
	}
}


status_t
ServerApp::_CreateWindow(int32 code, BPrivate::LinkReceiver& link,
	port_id& clientReplyPort)
{
	// Attached data:
	// 1) int32 bitmap token (only for AS_CREATE_OFFSCREEN_WINDOW)
	// 2) BRect window frame
	// 3) uint32 window look
	// 4) uint32 window feel
	// 5) uint32 window flags
	// 6) uint32 workspace index
	// 7) int32 BHandler token of the window
	// 8) port_id window's reply port
	// 9) port_id window's looper port
	// 10) const char * title

	BRect frame;
	int32 bitmapToken;
	uint32 look;
	uint32 feel;
	uint32 flags;
	uint32 workspaces;
	int32 token;
	port_id looperPort;
	char* title;

	if (code == AS_CREATE_OFFSCREEN_WINDOW)
		link.Read<int32>(&bitmapToken);

	link.Read<BRect>(&frame);
	link.Read<uint32>(&look);
	link.Read<uint32>(&feel);
	link.Read<uint32>(&flags);
	link.Read<uint32>(&workspaces);
	link.Read<int32>(&token);
	link.Read<port_id>(&clientReplyPort);
	link.Read<port_id>(&looperPort);
	if (link.ReadString(&title) != B_OK)
		return B_ERROR;

	if (!frame.IsValid()) {
		// make sure we pass a valid rectangle to ServerWindow
		frame.right = frame.left + 1;
		frame.bottom = frame.top + 1;
	}

	status_t status = B_ERROR;
	ServerWindow *window = NULL;

	if (code == AS_CREATE_OFFSCREEN_WINDOW) {
		ServerBitmap* bitmap = FindBitmap(bitmapToken);

		if (bitmap != NULL) {
			window = new OffscreenServerWindow(title, this, clientReplyPort,
				looperPort, token, bitmap);
		}
	} else {
		window = new ServerWindow(title, this, clientReplyPort, looperPort, token);
		STRACE(("\nServerApp %s: New Window %s (%g:%g, %g:%g)\n",
			fSignature(), title, frame.left, frame.top,
			frame.right, frame.bottom));
	}

	free(title);

	// NOTE: the reply to the client is handled in ServerWindow::Run()
	if (window != NULL) {
		status = window->Init(frame, (window_look)look, (window_feel)feel,
			flags, workspaces);
		if (status == B_OK && !window->Run())
			status = B_ERROR;

		// add the window to the list
		if (status == B_OK && fWindowListLock.Lock()) {
			status = fWindowList.AddItem(window) ? B_OK : B_NO_MEMORY;
			fWindowListLock.Unlock();
		}

		if (status < B_OK)
			delete window;
	}

	return status;
}


void
ServerApp::RemoveWindow(ServerWindow* window)
{
	BAutolock locker(fWindowListLock);

	fWindowList.RemoveItem(window);
}


bool
ServerApp::InWorkspace(int32 index) const
{
	BAutolock locker(fWindowListLock);

	// we could cache this, but then we'd have to recompute the cached
	// value everytime a window has closed or changed workspaces

	// TODO: support initial application workspace!

	for (int32 i = fWindowList.CountItems(); i-- > 0;) {
		ServerWindow* window = fWindowList.ItemAt(i);
		const WindowLayer* layer = window->Window();

		// only normal and unhidden windows count

		if (layer->IsNormal() && !layer->IsHidden() && layer->InWorkspace(index))
			return true;
	}

	return false;
}


uint32
ServerApp::Workspaces() const
{
	uint32 workspaces = 0;

	BAutolock locker(fWindowListLock);

	// we could cache this, but then we'd have to recompute the cached
	// value everytime a window has closed or changed workspaces

	for (int32 i = fWindowList.CountItems(); i-- > 0;) {
		ServerWindow* window = fWindowList.ItemAt(i);
		const WindowLayer* layer = window->Window();

		// only normal and unhidden windows count

		if (layer->IsNormal() && !layer->IsHidden())
			workspaces |= layer->Workspaces();
	}

	// TODO: add initial application workspace!
	return workspaces;
}


int32
ServerApp::CountBitmaps() const
{
	return fBitmapList.CountItems();
}


/*!
	\brief Looks up a ServerApp's ServerBitmap in its list
	\param token ID token of the bitmap to find
	\return The bitmap having that ID or NULL if not found
*/
ServerBitmap*
ServerApp::FindBitmap(int32 token) const
{
	// TODO: we need to make sure the bitmap is ours?!
	ServerBitmap* bitmap;
	if (gTokenSpace.GetToken(token, kBitmapToken, (void**)&bitmap) == B_OK)
		return bitmap;
	
	return NULL;
}


int32
ServerApp::CountPictures() const
{
	return fPictureList.CountItems();
}


ServerPicture *
ServerApp::CreatePicture(const ServerPicture *original)
{
	ServerPicture *picture;
	if (original != NULL)
		picture = new (nothrow) ServerPicture(*original);
	else
		picture = new (nothrow) ServerPicture();

	if (picture != NULL)
		fPictureList.AddItem(picture);
	
	return picture;
}


ServerPicture *
ServerApp::FindPicture(const int32 &token) const
{
	// TODO: we need to make sure the picture is ours?!
	ServerPicture* picture;
	if (gTokenSpace.GetToken(token, kPictureToken, (void**)&picture) == B_OK)
		return picture;

	return NULL;
}


bool
ServerApp::DeletePicture(const int32 &token)
{
	ServerPicture *picture = FindPicture(token);
	if (picture == NULL)
		return false;
	
	if (!fPictureList.RemoveItem(picture))
		return false;
	
	delete picture;

	return true;
}


team_id
ServerApp::ClientTeam() const
{
	return fClientTeam;
}

