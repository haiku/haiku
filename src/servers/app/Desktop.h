/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef DESKTOP_H
#define DESKTOP_H


#include "CursorManager.h"
#include "EventDispatcher.h"
#include "ScreenManager.h"
#include "ServerScreen.h"
#include "VirtualScreen.h"
#include "DesktopSettings.h"
#include "MessageLooper.h"
#include "Workspace.h"

#include <InterfaceDefs.h>
#include <List.h>
#include <Locker.h>
#include <Menu.h>
#include <Autolock.h>
#include <ObjectList.h>


class BMessage;

class DrawingEngine;
class HWInterface;
class Layer;
class RootLayer;
class WindowLayer;

namespace BPrivate {
	class LinkSender;
};


class Desktop : public MessageLooper, public ScreenOwner {
	public:
								Desktop(uid_t userID);
		virtual					~Desktop();

		void					Init();

		uid_t					UserID() const { return fUserID; }
		virtual port_id			MessagePort() const { return fMessagePort; }

		::EventDispatcher&		EventDispatcher() { return fEventDispatcher; }

		void					BroadcastToAllApps(int32 code);

		// Screen and drawing related methods

		Screen*					ScreenAt(int32 index) const
									{ return fActiveScreen; }
		Screen*					ActiveScreen() const
									{ return fActiveScreen; }
		::RootLayer*			RootLayer() const { return fRootLayer; }
		CursorManager&			GetCursorManager() { return fCursorManager; }

		void					ScreenChanged(Screen* screen);

		void					ScreenRemoved(Screen* screen) {}
		void					ScreenAdded(Screen* screen) {}
		bool					ReleaseScreen(Screen* screen) { return false; }

		const ::VirtualScreen&	VirtualScreen() const { return fVirtualScreen; }
		DrawingEngine*			GetDrawingEngine() const
									{ return fVirtualScreen.DrawingEngine(); }
		::HWInterface*			HWInterface() const
									{ return fVirtualScreen.HWInterface(); }

		// Workspace methods

		void					SetWorkspace(int32 index);
		int32					CurrentWorkspace()
									{ return fCurrentWorkspace; }
		::Workspace&			WorkspaceAt(int32 index)
									{ return fWorkspaces[index]; }

		// WindowLayer methods

		void					ActivateWindow(WindowLayer* window);
		void					SendBehindWindow(WindowLayer* window, WindowLayer* front);

		void					ShowWindow(WindowLayer* window);
		void					HideWindow(WindowLayer* window);

		void					SetWindowWorkspaces(WindowLayer* window, uint32 workspaces);

		void					AddWindowLayer(WindowLayer *windowLayer);
		void					RemoveWindowLayer(WindowLayer *windowLayer);
		void					SetWindowLayerFeel(WindowLayer *windowLayer,
									uint32 feel);

		WindowLayer*			FindWindowLayerByClientToken(int32 token, team_id teamID);
		//WindowLayer*			FindWindowLayerByServerToken(int32 token);

		// get list of registed windows
		const BObjectList<WindowLayer>& WindowList() const;

		void					WriteWindowList(team_id team,
									BPrivate::LinkSender& sender);
		void					WriteWindowInfo(int32 serverToken,
									BPrivate::LinkSender& sender);

	private:
 		status_t				_ActivateApp(team_id team);
		void					_GetLooperName(char* name, size_t size);
		void					_PrepareQuit();
		void					_DispatchMessage(int32 code,
									BPrivate::LinkReceiver &link);

	private:
		friend class DesktopSettings;

		uid_t					fUserID;
		::VirtualScreen			fVirtualScreen;
		DesktopSettings::Private* fSettings;
		port_id					fMessagePort;
		::EventDispatcher		fEventDispatcher;
		port_id					fInputPort;

		BLocker					fAppListLock;
		BList					fAppList;

		sem_id					fShutdownSemaphore;
		int32					fShutdownCount;

		::Workspace				fWorkspaces[32];//kMaxWorkspaces];
		int32					fCurrentWorkspace;

		BObjectList<WindowLayer> fWindowLayerList;

		::RootLayer*			fRootLayer;
		Screen*					fActiveScreen;

		CursorManager			fCursorManager;
};

#endif	// DESKTOP_H
