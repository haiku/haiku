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
 */
#ifndef SERVER_APP_H
#define SERVER_APP_H


#include "MessageLooper.h"
#include "BGet++.h"

#include <String.h>

#include <ObjectList.h>
#include <TokenSpace.h>


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
										  team_id clientTeamID,
										  int32 handlerID,
										  const char* signature);
	virtual						~ServerApp();

			status_t			InitCheck();
			void				Quit(sem_id shutdownSemaphore = -1);

	virtual	bool				Run();
	virtual	port_id				MessagePort() const { return fMessagePort; }

	/*!
		\brief Determines whether the application is the active one
		\return true if active, false if not.
	*/
			bool				IsActive(void) const { return fIsActive; }
			void				Activate(bool value);

			void				SendMessageToClient(BMessage* message) const;

			void				SetCurrentCursor(ServerCursor* cursor);
			ServerCursor*		CurrentCursor() const;

			team_id				ClientTeam() const;
			const char*			Signature() const { return fSignature.String(); }

			void				RemoveWindow(ServerWindow* window);
			bool				InWorkspace(int32 index) const;
			uint32				Workspaces() const;
			int32				InitialWorkspace() const { return fInitialWorkspace; }

			int32				CountBitmaps() const;
			ServerBitmap*		FindBitmap(int32 token) const;

			int32				CountPictures() const;
			ServerPicture*		CreatePicture(const ServerPicture* original = NULL);
			ServerPicture*		FindPicture(const int32& token) const;
			bool				DeletePicture(const int32& token);

			AreaPool*			AppAreaPool() { return &fSharedMem; }

			Desktop*			GetDesktop() const { return fDesktop; }

			BPrivate::BTokenSpace& ViewTokens() { return fViewTokens; }

 private:
	virtual	void				_DispatchMessage(int32 code,
												 BPrivate::LinkReceiver& link);
	virtual	void				_MessageLooper();
	virtual	void				_GetLooperName(char* name, size_t size);
			status_t			_CreateWindow(int32 code,
											  BPrivate::LinkReceiver& link,
											  port_id& clientReplyPort);

			bool				_HasWindowUnderMouse();

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

	mutable	BLocker				fWindowListLock;
			BObjectList<ServerWindow> fWindowList;
			BPrivate::BTokenSpace fViewTokens;

			int32				fInitialWorkspace;

		// NOTE: Bitmaps and Pictures are stored globally, but ServerApps remember
		// which ones they own so that they can destroy them when they quit.
		// TODO:
		// - As we reference these stuff by token, what about putting them in hash tables ?
			BList				fBitmapList;
			BList				fPictureList;

			ServerCursor*		fAppCursor;
			ServerCursor*		fViewCursor;
			int32				fCursorHideLevel;
									// 0 = cursor visible

			bool				fIsActive;

			AreaPool			fSharedMem;
};

#endif	// SERVER_APP_H
