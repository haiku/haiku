//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	Description:	Server-side BApplication counterpart
//  
//------------------------------------------------------------------------------
#ifndef _SERVERAPP_H_
#define _SERVERAPP_H_

#include <OS.h>
#include <String.h>

class AppServer;
class BMessage;
class PortLink;
class PortMessage;
class BList;
class DisplayDriver;
class ServerCursor;

/*!
	\class ServerApp ServerApp.h
	\brief Counterpart to BApplication within the app_server
*/
class ServerApp
{
public:
	ServerApp(port_id sendport, port_id rcvport, int32 handlerID, char *signature);
	~ServerApp(void);

	bool Run(void);
	static int32 MonitorApp(void *data);	
	void Lock(void);
	void Unlock(void);
	bool IsLocked(void);
	
	/*!
		\brief Determines whether the application is the active one
		\return true if active, false if not.
	*/
	bool IsActive(void) const { return _isactive; }

	void Activate(bool value);
	bool PingTarget(void);
	
	void PostMessage(int32 code, size_t size=0, int8 *buffer=NULL);

	void SetAppCursor(void);
protected:
	friend class AppServer;
	friend class ServerWindow;
	void _DispatchMessage(PortMessage *msg);
	ServerBitmap *_FindBitmap(int32 token);

	port_id _sender,_receiver;
	BString _signature;
	thread_id _monitor_thread;
	team_id _target_id;
	PortLink *_applink;
	BList *_winlist, *_bmplist, *_piclist;
	DisplayDriver *_driver;
	ServerCursor *_appcursor;
	sem_id _lock;
	bool _cursorhidden;
	bool _isactive;
	int32 _handlertoken;
};

#endif
