/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	Class used to encapsulate desktop management */


#include "Desktop.h"

#include "AppServer.h"
#include "DesktopSettingsPrivate.h"
#include "DrawingEngine.h"
#include "HWInterface.h"
#include "InputManager.h"
#include "Layer.h"
#include "RootLayer.h"
#include "ServerApp.h"
#include "ServerConfig.h"
#include "ServerScreen.h"
#include "ServerWindow.h"
#include "WindowLayer.h"
#include "Workspace.h"

#include <WindowInfo.h>
#include <ServerProtocol.h>

#include <Entry.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Region.h>

#include <stdio.h>

#if TEST_MODE
#	include "EventStream.h"
#endif

//#define DEBUG_DESKTOP
#ifdef DEBUG_DESKTOP
#	define STRACE(a) printf(a)
#else
#	define STRACE(a) ;
#endif


class KeyboardFilter : public BMessageFilter {
	public:
		KeyboardFilter(Desktop* desktop);

		virtual filter_result Filter(BMessage* message, BHandler** _target);

	private:
		Desktop* fDesktop;
};


KeyboardFilter::KeyboardFilter(Desktop* desktop)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
	fDesktop(desktop)
{
}


filter_result
KeyboardFilter::Filter(BMessage* message, BHandler** /*_target*/)
{
	int32 key;
	int32 modifiers;

	if (message->what != B_KEY_DOWN
		|| message->FindInt32("key", &key) != B_OK
		|| message->FindInt32("modifiers", &modifiers) != B_OK)
		return B_DISPATCH_MESSAGE;

	// Check for safe video mode (F12 + l-cmd + l-ctrl + l-shift)
	if (key == 0x0d
		&& (modifiers & (B_LEFT_COMMAND_KEY
				| B_LEFT_CONTROL_KEY | B_LEFT_SHIFT_KEY)) != 0)
	{
		// TODO: Set to Safe Mode in KeyboardEventHandler:B_KEY_DOWN.
		STRACE(("Safe Video Mode invoked - code unimplemented\n"));
		return B_SKIP_MESSAGE;
	}

	if (key > 0x01 && key < 0x0e) {
		// workspace change, F1-F12

#if !TEST_MODE
		if (modifiers & B_COMMAND_KEY)
#else
		if (modifiers & B_CONTROL_KEY)
#endif
		{
			STRACE(("Set Workspace %ld\n", key - 1));

			fDesktop->SetWorkspace(key - 2);
			return B_SKIP_MESSAGE;
		}
	}

	// TODO: this should be moved client side!
	// (that's how it is done in BeOS, clients could need this key for
	// different purposes - also, it's preferrable to let the client
	// write the dump within his own environment)
	if (key == 0xe) {
		// screen dump, PrintScreen
		char filename[128];
		BEntry entry;

		int32 index = 1;
		do {
			sprintf(filename, "/boot/home/screen%ld.png", index++);
			entry.SetTo(filename);
		} while(entry.Exists());

		fDesktop->GetDrawingEngine()->DumpToFile(filename);
		return B_SKIP_MESSAGE;
	}

	return B_DISPATCH_MESSAGE;
}


//	#pragma mark -


Desktop::Desktop(uid_t userID)
	: MessageLooper("desktop"),
	fUserID(userID),
	fSettings(new DesktopSettings::Private()),
	fAppListLock("application list"),
	fShutdownSemaphore(-1),
	fWindowLayerList(64),
	fActiveScreen(NULL),
	fCursorManager()
{
	char name[B_OS_NAME_LENGTH];
	Desktop::_GetLooperName(name, sizeof(name));

	fMessagePort = create_port(DEFAULT_MONITOR_PORT_SIZE, name);
	if (fMessagePort < B_OK)
		return;

	fLink.SetReceiverPort(fMessagePort);
	gFontManager->AttachUser(fUserID);
}


Desktop::~Desktop()
{
	// root layer only knows the visible WindowLayers, so we delete them all over here
	for (int32 i = 0; WindowLayer *border = (WindowLayer *)fWindowLayerList.ItemAt(i); i++)
		delete border;

	delete fRootLayer;
	delete fSettings;

	delete_port(fMessagePort);
	gFontManager->DetachUser(fUserID);
}


void
Desktop::Init()
{
	fVirtualScreen.RestoreConfiguration(*this, fSettings->WorkspacesMessage(0));

	// TODO: temporary workaround, fActiveScreen will be removed
	fActiveScreen = fVirtualScreen.ScreenAt(0);

	// TODO: add user identity to the name
	char name[32];
	sprintf(name, "RootLayer %d", 1);
	fRootLayer = new ::RootLayer(name, this, GetDrawingEngine());

#if TEST_MODE
	gInputManager->AddStream(new InputServerStream);
#endif
	fEventDispatcher.SetTo(gInputManager->GetStream());
	fEventDispatcher.SetHWInterface(fVirtualScreen.HWInterface());

	// temporary hack to get things started
	class MouseFilter : public BMessageFilter {
		public:
			MouseFilter(::RootLayer* layer)
				: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
				fRootLayer(layer)
			{
			}

			virtual filter_result
			Filter(BMessage* message, BHandler** /*_target*/)
			{
				fRootLayer->Lock();
				fRootLayer->MouseEventHandler(message);
				fRootLayer->Unlock();
				return B_DISPATCH_MESSAGE;
			}

		private:
			::RootLayer* fRootLayer;
	};
	fEventDispatcher.SetMouseFilter(new MouseFilter(fRootLayer));
	fEventDispatcher.SetKeyboardFilter(new KeyboardFilter(this));

	// take care of setting the default cursor
	ServerCursor *cursor = fCursorManager.GetCursor(B_CURSOR_DEFAULT);
	if (cursor)
		fVirtualScreen.HWInterface()->SetCursor(cursor);

	fVirtualScreen.HWInterface()->MoveCursorTo(fVirtualScreen.Frame().Width() / 2,
		fVirtualScreen.Frame().Height() / 2);
	fVirtualScreen.HWInterface()->SetCursorVisible(true);
}


void
Desktop::_GetLooperName(char* name, size_t length)
{
	snprintf(name, length, "d:%d:%s", /*id*/0, /*name*/"baron");
}


void
Desktop::_PrepareQuit()
{
	// let's kill all remaining applications

	fAppListLock.Lock();

	int32 count = fAppList.CountItems();
	for (int32 i = 0; i < count; i++) {
		ServerApp *app = (ServerApp*)fAppList.ItemAt(i);
		team_id clientTeam = app->ClientTeam();

		app->Quit();
		kill_team(clientTeam);
	}

	// wait for the last app to die
	if (count > 0)
		acquire_sem_etc(fShutdownSemaphore, fShutdownCount, B_RELATIVE_TIMEOUT, 250000);

	fAppListLock.Unlock();
}


void
Desktop::_DispatchMessage(int32 code, BPrivate::LinkReceiver &link)
{
	switch (code) {
		case AS_CREATE_APP:
		{
			// Create the ServerApp to node monitor a new BApplication

			// Attached data:
			// 1) port_id - receiver port of a regular app
			// 2) port_id - client looper port - for sending messages to the client
			// 2) team_id - app's team ID
			// 3) int32 - handler token of the regular app
			// 4) char * - signature of the regular app

			// Find the necessary data
			team_id	clientTeamID = -1;
			port_id	clientLooperPort = -1;
			port_id clientReplyPort = -1;
			int32 htoken = B_NULL_TOKEN;
			char *appSignature = NULL;

			link.Read<port_id>(&clientReplyPort);
			link.Read<port_id>(&clientLooperPort);
			link.Read<team_id>(&clientTeamID);
			link.Read<int32>(&htoken);
			if (link.ReadString(&appSignature) != B_OK)
				break;

			ServerApp *app = new ServerApp(this, clientReplyPort,
				clientLooperPort, clientTeamID, htoken, appSignature);
			if (app->InitCheck() == B_OK
				&& app->Run()) {
				// add the new ServerApp to the known list of ServerApps
				fAppListLock.Lock();
				fAppList.AddItem(app);
				fAppListLock.Unlock();
			} else {
				delete app;

				// if everything went well, ServerApp::Run() will notify
				// the client - but since it didn't, we do it here
				BPrivate::LinkSender reply(clientReplyPort);
				reply.StartMessage(SERVER_FALSE);
				reply.Flush();
			}

			// This is necessary because BPortLink::ReadString allocates memory
			free(appSignature);
			break;
		}

		case AS_DELETE_APP:
		{
			// Delete a ServerApp. Received only from the respective ServerApp when a
			// BApplication asks it to quit.

			// Attached Data:
			// 1) thread_id - thread ID of the ServerApp to be deleted

			thread_id thread = -1;
			if (link.Read<thread_id>(&thread) < B_OK)
				break;

			fAppListLock.Lock();

			// Run through the list of apps and nuke the proper one

			int32 count = fAppList.CountItems();
			ServerApp *removeApp = NULL;

			for (int32 i = 0; i < count; i++) {
				ServerApp *app = (ServerApp *)fAppList.ItemAt(i);

				if (app != NULL && app->Thread() == thread) {
					fAppList.RemoveItem(i);
					removeApp = app;
					break;
				}
			}

			fAppListLock.Unlock();

			if (removeApp != NULL)
				removeApp->Quit(fShutdownSemaphore);

			if (fQuitting && count <= 1) {
				// wait for the last app to die
				acquire_sem_etc(fShutdownSemaphore, fShutdownCount, B_RELATIVE_TIMEOUT, 500000);
				PostMessage(kMsgQuitLooper);
			}
			break;
		}

		case AS_ACTIVATE_APP:
		{
			// Someone is requesting to activation of a certain app.

			// Attached data:
			// 1) port_id reply port
			// 2) team_id team

			status_t status;

			// get the parameters
			port_id replyPort;
			team_id team;
			if (link.Read(&replyPort) == B_OK
				&& link.Read(&team) == B_OK)
				status = _ActivateApp(team);
			else
				status = B_ERROR;

			// send the reply
			BPrivate::PortLink replyLink(replyPort);
			replyLink.StartMessage(status);
			replyLink.Flush();
			break;
		}

		case AS_SET_SYSCURSOR_DEFAULTS:
		{
			GetCursorManager().SetDefaults();
			break;
		}

		case B_QUIT_REQUESTED:
			// We've been asked to quit, so (for now) broadcast to all
			// test apps to quit. This situation will occur only when the server
			// is compiled as a regular Be application.

			fAppListLock.Lock();
			fShutdownSemaphore = create_sem(0, "desktop shutdown");
			fShutdownCount = fAppList.CountItems();
			fAppListLock.Unlock();

			fQuitting = true;
			BroadcastToAllApps(AS_QUIT_APP);

			// We now need to process the remaining AS_DELETE_APP messages and
			// wait for the kMsgShutdownServer message.
			// If an application does not quit as asked, the picasso thread
			// will send us this message in 2-3 seconds.

			// if there are no apps to quit, shutdown directly
			if (fShutdownCount == 0)
				PostMessage(kMsgQuitLooper);
			break;

		default:
			printf("Desktop %d:%s received unexpected code %ld\n", 0, "baron", code);

			if (link.NeedsReply()) {
				// the client is now blocking and waiting for a reply!
				fLink.StartMessage(B_ERROR);
				fLink.Flush();
			}
			break;
	}
}


/*!
	\brief activate one of the app's windows.
*/
status_t
Desktop::_ActivateApp(team_id team)
{
	status_t status = B_BAD_TEAM_ID;

	// search for an unhidden window to give focus to
	int32 windowCount = WindowList().CountItems();
	for (int32 i = 0; i < windowCount; ++i) {
		// is this layer in fact a WindowLayer?
		WindowLayer *windowLayer = WindowList().ItemAt(i);

		// if winBorder is valid and not hidden, then we've found our target
		if (windowLayer != NULL && !windowLayer->IsHidden()
			&& windowLayer->App()->ClientTeam() == team) {
			fRootLayer->ActivateWindow(windowLayer);
			return B_OK;
		}
	}

	return status;
}


/*!
	\brief Send a quick (no attachments) message to all applications
	
	Quite useful for notification for things like server shutdown, system 
	color changes, etc.
*/
void
Desktop::BroadcastToAllApps(int32 code)
{
	BAutolock locker(fAppListLock);

	for (int32 i = 0; i < fAppList.CountItems(); i++) {
		ServerApp *app = (ServerApp *)fAppList.ItemAt(i);

		if (!app) {
			printf("PANIC in Broadcast()\n");
			continue;
		}
		app->PostMessage(code);
	}
}


void
Desktop::SetWorkspace(int32 index)
{
	BAutolock _(this);
	DesktopSettings settings(this);

	if (index < 0 || index >= settings.WorkspacesCount())
		return;

	fRootLayer->SetWorkspace(index, fWorkspaces[index]);
}


void
Desktop::ScreenChanged(Screen* screen)
{
	BMessage update(B_SCREEN_CHANGED);
	update.AddInt64("when", real_time_clock_usecs());
	update.AddRect("frame", screen->Frame());
	update.AddInt32("mode", screen->ColorSpace());

	BAutolock locker(this);

	// send B_SCREEN_CHANGED to windows on that screen
	// TODO: currently ignores the screen argument!

	for (int32 i = fWindowLayerList.CountItems(); i-- > 0;) {
		WindowLayer* layer = fWindowLayerList.ItemAt(i);
		ServerWindow* window = layer->Window();

		window->SendMessageToClient(&update);
	}
}


//	#pragma mark - Methods for WindowLayer manipulation


void
Desktop::ActivateWindow(WindowLayer* windowLayer)
{
	fRootLayer->ActivateWindow(windowLayer);
}


void
Desktop::SendBehindWindow(WindowLayer* windowLayer, WindowLayer* front)
{
	fRootLayer->SendBehindWindow(windowLayer, front);
}


void
Desktop::SetWindowWorkspaces(WindowLayer* windowLayer, uint32 workspaces)
{
	BAutolock _(this);

	windowLayer->SetWorkspaces(workspaces);

	// is the window still visible on screen?
	bool remove = (workspaces & (1UL << CurrentWorkspace())) == 0;

	if (remove && windowLayer->Parent() != NULL) {
		// the window is no longer visible on screen
		fRootLayer->RemoveWindowLayer(windowLayer);
	} else if (!remove && windowLayer->Parent() == NULL) {
		// the window is now visible on screen
		fRootLayer->AddWindowLayer(windowLayer);
	}
}


void
Desktop::AddWindowLayer(WindowLayer *windowLayer)
{
	Lock();

	if (fWindowLayerList.HasItem(windowLayer)) {
		Unlock();
		debugger("AddWindowLayer: WindowLayer already in Desktop list\n");
		return;
	}

	fWindowLayerList.AddItem(windowLayer);
	Unlock();

	RootLayer()->AddWindowLayer(windowLayer);
}


void
Desktop::RemoveWindowLayer(WindowLayer *windowLayer)
{
	Lock();
	fWindowLayerList.RemoveItem(windowLayer);
	Unlock();

	RootLayer()->RemoveWindowLayer(windowLayer);
}


void
Desktop::SetWindowLayerFeel(WindowLayer *winBorder, uint32 feel)
{
	// TODO: implement
}


WindowLayer *
Desktop::FindWindowLayerByClientToken(int32 token, team_id teamID)
{
	BAutolock locker(this);

	WindowLayer *wb;
	for (int32 i = 0; (wb = (WindowLayer *)fWindowLayerList.ItemAt(i)); i++) {
		if (wb->Window()->ClientToken() == token
			&& wb->Window()->ClientTeam() == teamID)
			return wb;
	}

	return NULL;
}


const BObjectList<WindowLayer> &
Desktop::WindowList() const
{
	if (!IsLocked())
		debugger("You must lock before getting registered windows list\n");

	return fWindowLayerList;
}


void
Desktop::WriteWindowList(team_id team, BPrivate::LinkSender& sender)
{
	BAutolock locker(this);

	// compute the number of windows

	int32 count = 0;
	if (team >= B_OK) {
		for (int32 i = 0; i < fWindowLayerList.CountItems(); i++) {
			WindowLayer* layer = fWindowLayerList.ItemAt(i);

			if (layer->Window()->ClientTeam() == team)
				count++;
		}
	} else
		count = fWindowLayerList.CountItems();

	// write list

	sender.StartMessage(SERVER_TRUE);
	sender.Attach<int32>(count);

	for (int32 i = 0; i < fWindowLayerList.CountItems(); i++) {
		WindowLayer* layer = fWindowLayerList.ItemAt(i);

		if (team >= B_OK && layer->Window()->ClientTeam() != team)
			continue;

		sender.Attach<int32>(layer->Window()->ServerToken());
	}

	sender.Flush();
}


void
Desktop::WriteWindowInfo(int32 serverToken, BPrivate::LinkSender& sender)
{
	BAutolock locker(this);
	BAutolock tokenLocker(BPrivate::gDefaultTokens);

	ServerWindow* window;
	if (BPrivate::gDefaultTokens.GetToken(serverToken,
			B_SERVER_TOKEN, (void**)&window) != B_OK) {
		sender.StartMessage(B_ENTRY_NOT_FOUND);
		sender.Flush();
		return;
	}

	window_info info;
	window->GetInfo(info);

	int32 length = window->Title() ? strlen(window->Title()) : 0;

	sender.StartMessage(B_OK);
	sender.Attach<int32>(sizeof(window_info) + length + 1);
	sender.Attach(&info, sizeof(window_info));
	if (length > 0)
		sender.Attach(window->Title(), length + 1);
	else
		sender.Attach<char>('\0');
	sender.Flush();
}

