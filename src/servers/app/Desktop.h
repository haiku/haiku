/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _DESKTOP_H_
#define _DESKTOP_H_

#include "CursorManager.h"
#include "ScreenManager.h"
#include "ServerScreen.h"
#include "VirtualScreen.h"
#include "DesktopSettings.h"
#include "MessageLooper.h"

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
class WinBorder;

namespace BPrivate {
	class LinkSender;
};


class Desktop : public MessageLooper, public ScreenOwner {
 public:
	// startup methods
								Desktop(uid_t userID);
	virtual						~Desktop();

			void				Init();

			uid_t				UserID() const { return fUserID; }
	virtual port_id				MessagePort() const { return fMessagePort; }

			void				BroadcastToAllApps(int32 code);

	// Methods for multiple monitors.
	inline	Screen*				ScreenAt(int32 index) const
									{ return fActiveScreen; }
	inline	Screen*				ActiveScreen() const
									{ return fActiveScreen; }
	inline	RootLayer*			ActiveRootLayer() const { return fRootLayer; }
	inline	CursorManager&		GetCursorManager() { return fCursorManager; }

	virtual void				ScreenRemoved(Screen* screen) {}
	virtual void				ScreenAdded(Screen* screen) {}
	virtual bool				ReleaseScreen(Screen* screen) { return false; }

	const	::VirtualScreen&	VirtualScreen() const { return fVirtualScreen; }
	inline	DrawingEngine*		GetDrawingEngine() const
									{ return fVirtualScreen.DrawingEngine(); }
	inline	HWInterface*		GetHWInterface() const
									{ return fVirtualScreen.HWInterface(); }

	// Methods for layer(WinBorder) manipulation.
			void				AddWinBorder(WinBorder *winBorder);
			void				RemoveWinBorder(WinBorder *winBorder);
			void				SetWinBorderFeel(WinBorder *winBorder,
												 uint32 feel);
			void				AddWinBorderToSubset(WinBorder *winBorder,
													 WinBorder *toWinBorder);
			void				RemoveWinBorderFromSubset(WinBorder *winBorder,
														  WinBorder *fromWinBorder);

			WinBorder*			FindWinBorderByClientToken(int32 token, team_id teamID);
			//WinBorder*		FindWinBorderByServerToken(int32 token);

			// get list of registed windows
			const BObjectList<WinBorder>& WindowList() const;

			void				WriteWindowList(team_id team, BPrivate::LinkSender& sender);
			void				WriteWindowInfo(int32 serverToken, BPrivate::LinkSender& sender);

 private:
 			status_t			_ActivateApp(team_id team);
	virtual void				_GetLooperName(char* name, size_t size);
	virtual void				_PrepareQuit();
	virtual void				_DispatchMessage(int32 code, BPrivate::LinkReceiver &link);

 private:
			friend class DesktopSettings;

			uid_t				fUserID;
			::VirtualScreen		fVirtualScreen;
			DesktopSettings::Private* fSettings;
			port_id				fMessagePort;

			BLocker				fAppListLock;
			BList				fAppList;

			sem_id				fShutdownSemaphore;
			int32				fShutdownCount;

			BObjectList<WinBorder> fWinBorderList;

			RootLayer*			fRootLayer;
			Screen*				fActiveScreen;
			
			CursorManager		fCursorManager;
};

#endif	// _DESKTOP_H_
