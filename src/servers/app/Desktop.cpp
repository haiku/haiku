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


#include <stdio.h>

#include <Message.h>
#include <Region.h>

#include <WindowInfo.h>
#include <ServerProtocol.h>

#include "AppServer.h"
#include "DrawingEngine.h"
#include "Layer.h"
#include "RootLayer.h"
#include "ServerConfig.h"
#include "ServerScreen.h"
#include "ServerApp.h"
#include "ServerWindow.h"
#include "WinBorder.h"
#include "Workspace.h"
#include "DesktopSettingsPrivate.h"


#ifdef __HAIKU__
#	define USE_ACCELERANT 1
#else
#	define USE_ACCELERANT 0
#endif

#if USE_ACCELERANT
#	include "AccelerantHWInterface.h"
#else
#	include "ViewHWInterface.h"
#endif

#include "Desktop.h"

//#define DEBUG_DESKTOP

#ifdef DEBUG_DESKTOP
#	define STRACE(a) printf(a)
#else
#	define STRACE(a) /* nothing */
#endif


Desktop::Desktop(uid_t userID)
	: MessageLooper("desktop"),
	fUserID(userID),
	fSettings(new DesktopSettings::Private()),
	fAppListLock("application list"),
	fShutdownSemaphore(-1),
	fWinBorderList(64),
	fActiveScreen(NULL),
	fCursorManager()
{
	// TODO: use user name
	char name[B_OS_NAME_LENGTH];
	snprintf(name, sizeof(name), "d:%s", "baron");

	fMessagePort = create_port(DEFAULT_MONITOR_PORT_SIZE, name);
	if (fMessagePort < B_OK)
		return;

	fLink.SetReceiverPort(fMessagePort);
	gFontManager->AttachUser(fUserID);
}


Desktop::~Desktop()
{
	for (int32 i = 0; WinBorder *border = (WinBorder *)fWinBorderList.ItemAt(i); i++)
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
	fRootLayer = new RootLayer(name, 4, this, GetDrawingEngine());

#if TEST_MODE
	RegisterInputServer(find_port(SERVER_INPUT_PORT));
		// this is where the ViewHWInterface will send its input events to
#endif

	// take care of setting the default cursor
	ServerCursor *cursor = fCursorManager.GetCursor(B_CURSOR_DEFAULT);
	if (cursor)
		fVirtualScreen.HWInterface()->SetCursor(cursor);
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
		// is this layer in fact a WinBorder?
		WinBorder *winBorder = WindowList().ItemAt(i);

		// if winBorder is valid and not hidden, then we've found our target
		if (winBorder != NULL && !winBorder->IsHidden()
			&& winBorder->App()->ClientTeam() == team) {
			if (fRootLayer->Lock()) {
				fRootLayer->SetActive(winBorder);
				fRootLayer->Unlock();

				if (fRootLayer->Active() == winBorder)
					return B_OK;

				status = B_ERROR;
			}
		}
	}

	return status;
}


void
Desktop::RegisterInputServer(port_id port)
{
	fInputPort = port;
	fRootLayer->RunThread();

	fVirtualScreen.HWInterface()->SetCursorVisible(true);
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


//	#pragma mark - Methods for WinBorder manipulation


void
Desktop::AddWinBorder(WinBorder *winBorder)
{
	if (!winBorder)
		return;

	// R2: how to determine the RootLayer to which this window should be added???
	// for now, use ActiveRootLayer() because we only have one instance.

	int32 feel = winBorder->Feel();

	// we are ServerApp thread, we need to lock RootLayer here.
	ActiveRootLayer()->Lock();

	// we're playing with window list. lock first.
	Lock();

	if (fWinBorderList.HasItem(winBorder)) {
		Unlock();
		ActiveRootLayer()->Unlock();
		debugger("AddWinBorder: WinBorder already in Desktop list\n");
		return;
	}

	// we have a new window. store a record of it.
	fWinBorderList.AddItem(winBorder);

	// add FLOATING_APP windows to the local list of all normal windows.
	// This is to keep the order all floating windows (app or subset) when we go from
	// one normal window to another.
	if (feel == B_FLOATING_APP_WINDOW_FEEL || feel == B_NORMAL_WINDOW_FEEL) {
		WinBorder *wb = NULL;
		int32 count = fWinBorderList.CountItems();
		int32 feelToLookFor = (feel == B_NORMAL_WINDOW_FEEL ?
			B_FLOATING_APP_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL);

		for (int32 i = 0; i < count; i++) {
			wb = (WinBorder *)fWinBorderList.ItemAt(i);

			if (wb->App()->ClientTeam() == winBorder->App()->ClientTeam()
				&& wb->Feel() == feelToLookFor) {
				// R2: RootLayer comparison is needed.
				feel == B_NORMAL_WINDOW_FEEL ?
					winBorder->fSubWindowList.AddWinBorder(wb) :
					wb->fSubWindowList.AddWinBorder(winBorder);
			}
		}
	}

	// add application's list of modal windows.
	if (feel == B_MODAL_APP_WINDOW_FEEL) {
		winBorder->App()->fAppSubWindowList.AddWinBorder(winBorder);
	}

	// send WinBorder to be added to workspaces
	ActiveRootLayer()->AddWinBorder(winBorder);

	// hey, unlock!
	Unlock();

	ActiveRootLayer()->Unlock();
}


void
Desktop::RemoveWinBorder(WinBorder *winBorder)
{
	if (!winBorder)
		return;

	// we are ServerApp thread, we need to lock RootLayer here.
	ActiveRootLayer()->Lock();

	// we're playing with window list. lock first.
	Lock();

	// remove from main WinBorder list.
	if (fWinBorderList.RemoveItem(winBorder)) {
		int32 feel = winBorder->Feel();

		// floating app/subset and modal_subset windows require special atention because
		// they are/may_be added to the list of a lot normal windows.
		if (feel == B_FLOATING_SUBSET_WINDOW_FEEL
			|| feel == B_MODAL_SUBSET_WINDOW_FEEL
			|| feel == B_FLOATING_APP_WINDOW_FEEL)
		{
			WinBorder *wb = NULL;
			int32 count = fWinBorderList.CountItems();

			for (int32 i = 0; i < count; i++) {
				wb = (WinBorder*)fWinBorderList.ItemAt(i);

				if (wb->Feel() == B_NORMAL_WINDOW_FEEL
					&& wb->App()->ClientTeam() == winBorder->App()->ClientTeam()) {
					// R2: RootLayer comparison is needed. We'll see.
					wb->fSubWindowList.RemoveItem(winBorder);
				}
			}
		}

		// remove from application's list
		if (feel == B_MODAL_APP_WINDOW_FEEL) {
			winBorder->App()->fAppSubWindowList.RemoveItem(winBorder);
		}
	} else {
		Unlock();
		ActiveRootLayer()->Unlock();
		debugger("RemoveWinBorder: WinBorder not found in Desktop list\n");
		return;
	}

	// Tell to winBorder's RootLayer about this.
	ActiveRootLayer()->RemoveWinBorder(winBorder);

	Unlock();
	ActiveRootLayer()->Unlock();
}


void
Desktop::AddWinBorderToSubset(WinBorder *winBorder, WinBorder *toWinBorder)
{
	// NOTE: we can safely lock the entire method body, because this method is called from
	//		 RootLayer's thread only.

	// we're playing with window list. lock first.
	Lock();

	if (!winBorder || !toWinBorder
		|| !fWinBorderList.HasItem(winBorder)
		|| !fWinBorderList.HasItem(toWinBorder)) {
		Unlock();
		debugger("AddWinBorderToSubset: NULL WinBorder or not found in Desktop list\n");
		return;
	}

	if ((winBorder->Feel() == B_FLOATING_SUBSET_WINDOW_FEEL
			|| winBorder->Feel() == B_MODAL_SUBSET_WINDOW_FEEL)
		&& toWinBorder->Feel() == B_NORMAL_WINDOW_FEEL
		&& toWinBorder->App()->ClientTeam() == winBorder->App()->ClientTeam()
		&& !toWinBorder->fSubWindowList.HasItem(winBorder)) {
		// add to normal_window's list
		toWinBorder->fSubWindowList.AddWinBorder(winBorder);
	} else {
		Unlock();
		debugger("AddWinBorderToSubset: you must add a subset_window to a normal_window's subset with the same team_id\n");
		return;
	}

	// send WinBorder to be added to workspaces, if not already in there.
	ActiveRootLayer()->AddSubsetWinBorder(winBorder, toWinBorder);

	Unlock();
}


void
Desktop::RemoveWinBorderFromSubset(WinBorder *winBorder, WinBorder *fromWinBorder)
{
	// NOTE: we can safely lock the entire method body, because this method is called from
	//		 RootLayer's thread only.

	// we're playing with window list. lock first.
	Lock();

	if (!winBorder || !fromWinBorder
		|| !fWinBorderList.HasItem(winBorder)
		|| !fWinBorderList.HasItem(fromWinBorder)) {
		Unlock();
		debugger("RemoveWinBorderFromSubset: NULL WinBorder or not found in Desktop list\n");
		return;
	}

	// remove WinBorder from workspace, if needed - some other windows may still have it in their subset
	ActiveRootLayer()->RemoveSubsetWinBorder(winBorder, fromWinBorder);

	if (fromWinBorder->Feel() == B_NORMAL_WINDOW_FEEL) {
		//remove from this normal_window's subset.
		fromWinBorder->fSubWindowList.RemoveItem(winBorder);
	} else {
		Unlock();
		debugger("RemoveWinBorderFromSubset: you must remove a subset_window from a normal_window's subset\n");
		return;
	}

	Unlock();
}


void
Desktop::SetWinBorderFeel(WinBorder *winBorder, uint32 feel)
{
	// NOTE: this method is called from RootLayer thread only

	// we're playing with window list. lock first.
	Lock();

	RemoveWinBorder(winBorder);
	winBorder->QuietlySetFeel(feel);
	AddWinBorder(winBorder);

	Unlock();
}


WinBorder *
Desktop::FindWinBorderByClientToken(int32 token, team_id teamID)
{
	BAutolock locker(this);

	WinBorder *wb;
	for (int32 i = 0; (wb = (WinBorder *)fWinBorderList.ItemAt(i)); i++) {
		if (wb->Window()->ClientToken() == token
			&& wb->Window()->ClientTeam() == teamID)
			return wb;
	}

	return NULL;
}


const BObjectList<WinBorder> &
Desktop::WindowList() const
{
	if (!IsLocked())
		debugger("You must lock before getting registered windows list\n");

	return fWinBorderList;
}


void
Desktop::WriteWindowList(team_id team, BPrivate::LinkSender& sender)
{
	BAutolock locker(this);

	// compute the number of windows

	int32 count = 0;
	if (team >= B_OK) {
		for (int32 i = 0; i < fWinBorderList.CountItems(); i++) {
			WinBorder* border = fWinBorderList.ItemAt(i);

			if (border->Window()->ClientTeam() == team)
				count++;
		}
	} else
		count = fWinBorderList.CountItems();

	// write list

	sender.StartMessage(SERVER_TRUE);
	sender.Attach<int32>(count);

	for (int32 i = 0; i < fWinBorderList.CountItems(); i++) {
		WinBorder* border = fWinBorderList.ItemAt(i);

		if (team >= B_OK && border->Window()->ClientTeam() != team)
			continue;

		sender.Attach<int32>(border->Window()->ServerToken());
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

