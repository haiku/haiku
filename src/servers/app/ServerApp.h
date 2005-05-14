//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ServerApp.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@myrealbox.com>
//	Description:	Server-side BApplication counterpart
//  
//------------------------------------------------------------------------------
#ifndef _SERVERAPP_H_
#define _SERVERAPP_H_

#include <OS.h>
#include <String.h>

#include "FMWList.h"

class AreaPool;
class BMessage;
class BPortLink;
class BList;
class DisplayDriver;
class LinkMsgReader;
class LinkMsgSender;
class ServerPicture;
class ServerCursor;
class ServerBitmap;

/*!
	\class ServerApp ServerApp.h
	\brief Counterpart to BApplication within the app_server
*/
class ServerApp {
public:
	ServerApp(port_id sendport, port_id rcvport, port_id clientLooperPort,
		team_id clientTeamID, int32 handlerID, const char* signature);
	virtual	~ServerApp(void);
	
	bool Run(void);
	/*
	TODO: These aren't even implemented...
	void Lock(void);
	void Unlock(void);
	bool IsLocked(void);
	*/
	/*!
		\brief Determines whether the application is the active one
		\return true if active, false if not.
	*/
	bool IsActive(void) const { return fIsActive; }
	void Activate(bool value);
	
	bool PingTarget(void);
	
	void PostMessage(int32 code);
	void SendMessageToClient( const BMessage* msg ) const;
	
	void SetAppCursor(void);
	
	team_id	ClientTeamID() const;
	thread_id MonitorThreadID() const;
	
	const char *Title() const { return fSignature.String(); }
	
	int32 CountBitmaps() const;
	ServerBitmap *FindBitmap(int32 token) const;
	
	int32 CountPictures() const;
	ServerPicture *FindPicture(int32 token) const;

	AreaPool *AppAreaPool() { return fSharedMem; }
	
	FMWList fAppFMWList;
	
private:
	void DispatchMessage(int32 code, LinkMsgReader &link);
	
	static int32 MonitorApp(void *data);	
	
	// our BApplication's event port
	port_id	fClientAppPort;	
	// port we receive messages from our BApplication
	port_id	fMessagePort;
			// TODO: find out why there is both the app port and the looper port. Do 
			// we really need both?
	// To send a message to the client, write a BMessage to this port
	port_id	fClientLooperPort;
	
	BString fSignature;
	
	thread_id fMonitorThreadID;
	team_id fClientTeamID;
	
	LinkMsgReader *fMsgReader;
	LinkMsgSender *fMsgSender;
	
	// TODO:
	// - Are really Bitmaps and Pictures stored per application and not globally ?
	// - As we reference these stuff by token, what about putting them in hash tables ?
	BList *fSWindowList,
		  *fBitmapList,
		  *fPictureList;
		  
	ServerCursor *fAppCursor;
	
	sem_id fLockSem;
	
	bool fCursorHidden;
	bool fIsActive;
	
	// token ID of the BApplication's BHandler object.
	// Used for BMessage target specification
	// TODO: Is it still needed ? We aren't using it.
	//int32 fHandlerToken;
	
	AreaPool *fSharedMem;
	
	bool fQuitting;
};

#endif
