/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef RPCSERVER_H
#define RPCSERVER_H


#include <condition_variable.h>
#include <lock.h>

#include "Connection.h"
#include "RPCCall.h"
#include "RPCReply.h"


namespace RPC {

struct Request {
	uint32				fXID;
	ConditionVariable	fEvent;
	Reply**				fReply;

	Request*			fNext;
};

class RequestManager {
public:
						RequestManager();
						~RequestManager();

			void		AddRequest(Request* req);
			Request*	FindRequest(uint32 xid);

private:
			mutex		fLock;
			Request*	fQueueHead;
			Request*	fQueueTail;

};

class Server {
public:
									Server(Connection* conn,
										ServerAddress* addr);
	virtual							~Server();

			status_t				SendCall(Call* call, Reply** reply);

			status_t				SendCallAsync(Call* call, Reply** reply,
										Request** request);
	inline	status_t				WaitCall(Request* request,
										bigtime_t time = kWaitTime);
	inline	status_t				CancelCall(Request* request);

			status_t				Repair();

	inline	const ServerAddress&	ID() const;
	inline	ServerAddress			LocalID() const;

private:
	inline	uint32					_GetXID();

			status_t				_StartListening();

			status_t				_Listener();
	static	status_t				_ListenerThreadStart(void* ptr);

			thread_id				fThread;
			bool					fThreadCancel;
			status_t				fThreadError;

			RequestManager			fRequests;
			Connection*				fConnection;
			const ServerAddress*	fAddress;

			vint32					fXID;
	static	const bigtime_t			kWaitTime	= 1000000;
};


inline status_t
Server::WaitCall(Request* request, bigtime_t time)
{
	return request->fEvent.Wait(B_RELATIVE_TIMEOUT, time);
}


inline status_t
Server::CancelCall(Request* request)
{
	fRequests.FindRequest(request->fXID);
	return B_OK;
}


inline const ServerAddress&
Server::ID() const
{
	return *fAddress;
}


inline ServerAddress
Server::LocalID() const
{
	ServerAddress addr;
	memset(&addr, 0, sizeof(addr));
	fConnection->GetLocalID(&addr);
	return addr;
}


struct ServerNode {
	ServerAddress	fID;
	Server*			fServer;
	int				fRefCount;

	ServerNode*		fLeft;
	ServerNode* 	fRight;
};

class ServerManager {
public:
						ServerManager();
						~ServerManager();

			status_t	Acquire(Server** pserv, uint32 ip, uint16 port,
								Transport proto);
			void		Release(Server* serv);

private:

			ServerNode*	_Find(const ServerAddress& id);
			void		_Delete(ServerNode* node);
			ServerNode*	_Insert(ServerNode* node);

			ServerNode*	fRoot;
			mutex		fLock;
};

}		// namespace RPC


#endif	// RPCSERVER_H

