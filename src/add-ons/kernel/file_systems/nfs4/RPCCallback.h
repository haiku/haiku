/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef RPCCALLBACK_H
#define RPCCALLBACK_H


#include "Connection.h"


namespace RPC {

class CallbackRequest;
class Server;

class Callback {
public:
						Callback(Server* server);

	inline	void		SetID(int32 id);
	inline	int32		ID();

			status_t	EnqueueRequest(CallbackRequest* request,
							Connection* connection);

private:
			Server*		fServer;
			int32		fID;
};


inline void
Callback::SetID(int32 id)
{
	fID = id;
}


inline int32
Callback::ID()
{
	return fID;
}


}		// namespace RPC


#endif	// RPCCALLBACK_H

