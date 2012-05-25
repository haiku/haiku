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
							Server(Connection* conn);
	virtual					~Server();

			status_t		SendCall(Call* call, Reply** reply);
	inline	void			CancelCall(uint32 xid);

protected:
	inline	uint32			_GetXID();
			status_t		_StartListening();

private:
			status_t		_Listener();
	static	status_t		_ListenerThreadStart(void* ptr);

			thread_id		fThread;
			bool			fThreadCancel;

			RequestManager	fRequests;
			Connection*		fConnection;

			vint32			fXID;
	static	const bigtime_t	kWaitTime	= 1000000;
};

struct ServerNode {
	ServerAddress	fID;
	Server*		fServer;
	int			fRefCount;

	ServerNode*	fLeft;
	ServerNode* fRight;
};

class ServerManager {
public:
						ServerManager();
						~ServerManager();

			status_t	Acquire(Server** pserv, uint32 ip, uint16 port,
								Transport proto);
			void		Release(Server* serv);

private:


			ServerNode*	_Find(ServerAddress& id);
			void		_Delete(ServerNode* node);
			ServerNode*	_Insert(ServerNode* node);

			ServerNode*	fRoot;
			mutex		fLock;
};

}		// namespace RPC


#endif	// RPCSERVER_H

