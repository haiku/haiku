//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
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
//	File Name:		Registrar.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Registrar application.
//------------------------------------------------------------------------------
#ifndef REGISTRAR_H
#define REGISTRAR_H

#include <Server.h>


class AuthenticationManager;
class ClipboardHandler;
class DiskDeviceManager;
class EventQueue;
class MessageEvent;
class MessageRunnerManager;
class MIMEManager;
class PackageWatchingManager;
class ShutdownProcess;

class TRoster;


class Registrar : public BServer {
public:
	Registrar(status_t *error);
	virtual ~Registrar();

	virtual void MessageReceived(BMessage *message);
	virtual void ReadyToRun();
	virtual bool QuitRequested();

	EventQueue *GetEventQueue() const;

	static Registrar *App();

private:
	void _MessageReceived(BMessage *message);
	void _HandleShutDown(BMessage *message);
	void _HandleIsShutDownInProgress(BMessage *message);

	TRoster					*fRoster;
	ClipboardHandler		*fClipboardHandler;
	MIMEManager				*fMIMEManager;
	EventQueue				*fEventQueue;
	MessageRunnerManager	*fMessageRunnerManager;
	ShutdownProcess			*fShutdownProcess;
	AuthenticationManager	*fAuthenticationManager;
	PackageWatchingManager	*fPackageWatchingManager;
};

#endif	// REGISTRAR_H
