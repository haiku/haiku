/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef RPCCALLBACKSERVER_H
#define RPCCALLBACKSERVER_H


#include <util/AutoLock.h>

#include "Connection.h"


namespace RPC {

class Callback;

struct ConnectionEntry {
	Connection*			fConnection;
	thread_id			fThread;

	ConnectionEntry*	fNext;
	ConnectionEntry*	fPrev;
};

union CallbackSlot {
	Callback*	fCallback;
	int32		fNext;
};

class CallbackServer {
public:
							CallbackServer();
							~CallbackServer();

			status_t		RegisterCallback(Callback* callback);
			status_t		UnregisterCallback(Callback* callback);

	inline	PeerAddress	LocalID();

protected:
			status_t		StartServer();
			status_t		StopServer();

			status_t		NewConnection(Connection* connection);
			status_t		ReleaseConnection(ConnectionEntry* entry);

	static	status_t		ListenerThreadLauncher(void* object);
			status_t		ListenerThread();

	static	status_t		ConnectionThreadLauncher(void* object);
			status_t		ConnectionThread(ConnectionEntry* entry);

	inline	Callback*		GetCallback(int32 id);

private:
			mutex			fConnectionLock;
			ConnectionEntry*	fConnectionList;
			ConnectionListener*	fListener;

			mutex			fThreadLock;
			thread_id		fThread;
			bool			fThreadRunning;

			rw_lock			fArrayLock;
			CallbackSlot*	fCallbackArray;
			uint32			fArraySize;
			int32			fFreeSlot;
};


inline PeerAddress
CallbackServer::LocalID()
{
	PeerAddress address;
	fListener->GetLocalAddress(&address);
	return address;
}


inline Callback*
CallbackServer::GetCallback(int32 id)
{
	ReadLocker _(fArrayLock);
	if (id >= 0 && static_cast<uint32>(id) < fArraySize)
		return fCallbackArray[id].fCallback;
	return NULL;
}


}		// namespace RPC


extern RPC::CallbackServer* gRPCCallbackServer;


#endif	// RPCCALLBACKSERVER_H

