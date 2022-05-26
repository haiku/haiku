/*
 * Copyright 2001-2013, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Stefano Ceccherini (burton666@libero.it)
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SERVER_APP_H
#define SERVER_APP_H


#include "ClientMemoryAllocator.h"
#include "MessageLooper.h"
#include "ServerFont.h"

#include <ObjectList.h>
#include <TokenSpace.h>

#include <Messenger.h>
#include <String.h>


class AreaPool;
class BMessage;
class BList;
class Desktop;
class DrawingEngine;
class ServerPicture;
class ServerCursor;
class ServerBitmap;
class ServerWindow;

namespace BPrivate {
	class PortLink;
};


class ServerApp : public MessageLooper {
public:
								ServerApp(Desktop* desktop,
									port_id clientAppPort,
									port_id clientLooperPort,
									team_id clientTeamID, int32 handlerID,
									const char* signature);
	virtual						~ServerApp();

			status_t			InitCheck();

	virtual	void				Quit();
			void				Quit(sem_id shutdownSemaphore);

	virtual	port_id				MessagePort() const { return fMessagePort; }

	/*!
		\brief Determines whether the application is the active one
		\return true if active, false if not.
	*/
			bool				IsActive() const { return fIsActive; }
			void				Activate(bool value);

			void				SendMessageToClient(BMessage* message) const;

			void				SetCurrentCursor(ServerCursor* cursor);
			ServerCursor*		CurrentCursor() const;

			team_id				ClientTeam() const { return fClientTeam; }

			const char*			Signature() const
									{ return fSignature.String(); }
			const char*			SignatureLeaf() const
									{ return fSignature.String() + 12; }

			bool				AddWindow(ServerWindow* window);
			void				RemoveWindow(ServerWindow* window);
			bool				InWorkspace(int32 index) const;
			uint32				Workspaces() const;
			int32				InitialWorkspace() const
									{ return fInitialWorkspace; }

			ServerBitmap*		GetBitmap(int32 token) const;

			ServerPicture*		CreatePicture(
									const ServerPicture* original = NULL);
			ServerPicture*		GetPicture(int32 token) const;
			bool				AddPicture(ServerPicture* picture);
			void				RemovePicture(ServerPicture* picture);

			Desktop*			GetDesktop() const { return fDesktop; }

			const ServerFont&	PlainFont() const { return fPlainFont; }

			BPrivate::BTokenSpace& ViewTokens() { return fViewTokens; }

			void				NotifyDeleteClientArea(area_id serverArea);

private:
	virtual	void				_GetLooperName(char* name, size_t size);
	virtual	void				_DispatchMessage(int32 code,
									BPrivate::LinkReceiver& link);
	virtual	void				_MessageLooper();
			status_t			_CreateWindow(int32 code,
									BPrivate::LinkReceiver& link,
									port_id& clientReplyPort);

			bool				_HasWindowUnderMouse();

			bool				_AddBitmap(ServerBitmap* bitmap);
			void				_DeleteBitmap(ServerBitmap* bitmap);
			ServerBitmap*		_FindBitmap(int32 token) const;

			ServerPicture*		_FindPicture(int32 token) const;

private:
	typedef std::map<int32, BReference<ServerBitmap> > BitmapMap;
	typedef std::map<int32, BReference<ServerPicture> > PictureMap;

			port_id				fMessagePort;
			port_id				fClientReplyPort;
									// our BApplication's event port

			BMessenger			fHandlerMessenger;
			port_id				fClientLooperPort;
			int32				fClientToken;
									// To send a BMessage to the client
									// (port + token)

			Desktop*			fDesktop;
			BString				fSignature;
			team_id				fClientTeam;

			ServerFont			fPlainFont;
			ServerFont			fBoldFont;
			ServerFont			fFixedFont;

	mutable	BLocker				fWindowListLock;
			BObjectList<ServerWindow> fWindowList;
			BPrivate::BTokenSpace fViewTokens;

			int32				fInitialWorkspace;
			uint32				fTemporaryDisplayModeChange;

			// NOTE: Bitmaps and Pictures are stored globally, but ServerApps
			// remember which ones they own so that they can destroy them when
			// they quit.
	mutable	BLocker				fMapLocker;
			BitmapMap			fBitmapMap;
			PictureMap			fPictureMap;

			BReference<ServerCursor>
								fAppCursor;
			BReference<ServerCursor>
								fViewCursor;
			int32				fCursorHideLevel;
									// 0 = cursor visible

			bool				fIsActive;

			BReference<ClientMemoryAllocator> fMemoryAllocator;
};


#endif	// SERVER_APP_H
