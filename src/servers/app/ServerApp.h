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
#ifndef _SERVERAPP_H_
#define _SERVERAPP_H_


#include <OS.h>
#include <String.h>
#include <PortLink.h>

#include "SubWindowList.h"
#include "BGet++.h"

class AreaPool;
class BMessage;
class BList;
class DisplayDriver;
class ServerPicture;
class ServerCursor;
class ServerBitmap;

namespace BPrivate {
	class PortLink;
};

/*!
	\class ServerApp ServerApp.h
	\brief Counterpart to BApplication within the app_server
*/
class ServerApp {
public:
	ServerApp(port_id clientAppPort, port_id clientLooperPort,
		team_id clientTeamID, int32 handlerID, const char* signature);
	~ServerApp();

	status_t InitCheck();
	bool Run();
	void Quit(sem_id shutdownSemaphore = -1);

	/*!
		\brief Determines whether the application is the active one
		\return true if active, false if not.
	*/
	bool IsActive(void) const { return fIsActive; }
	void Activate(bool value);

	bool PingTarget(void);

	void PostMessage(int32 code);
	void SendMessageToClient(const BMessage* msg) const;

	void SetAppCursor(void);

	team_id	ClientTeam() const;
	thread_id Thread() const;

	const char *Signature() const { return fSignature.String(); }

	void RemoveWindow(ServerWindow* window);

	int32 CountBitmaps() const;
	ServerBitmap *FindBitmap(int32 token) const;

	int32 CountPictures() const;
	ServerPicture *FindPicture(int32 token) const;

	AreaPool *AppAreaPool() { return &fSharedMem; }

	SubWindowList fAppSubWindowList;

private:
	void _DispatchMessage(int32 code, BPrivate::LinkReceiver &link);
	void _MessageLooper();

	static int32 _message_thread(void *data);	

	// our BApplication's event port
	port_id	fClientReplyPort;	
	// port we receive messages from our BApplication
	port_id	fMessagePort;
			// TODO: find out why there is both the app port and the looper port. Do 
			// we really need both? Actually, we aren't using any of these ports,
			// as BAppServerLink/BPortlink's messages always contain the reply port
	// To send a message to the client, write a BMessage to this port
	port_id	fClientLooperPort;

	BString fSignature;

	thread_id fThread;
	team_id fClientTeam;

	BPrivate::PortLink fLink;

	BLocker fWindowListLock;
	BList fWindowList;

	// TODO:
	// - Are really Bitmaps and Pictures stored per application and not globally ?
	// - As we reference these stuff by token, what about putting them in hash tables ?
	BList fBitmapList;
	BList fPictureList;

	ServerCursor *fAppCursor;

	bool fCursorHidden;
	bool fIsActive;
	
	// token ID of the BApplication's BHandler object.
	// Used for BMessage target specification
	// TODO: Is it still needed ? We aren't using it.
	//int32 fHandlerToken;

	AreaPool fSharedMem;

	bool fQuitting;
};

#endif	// _SERVERAPP_H_
