/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef NFS4SERVER_H
#define NFS4SERVER_H


#include <lock.h>

#include "RPCServer.h"


class NFS4Server : public RPC::ProgramData {
public:
							NFS4Server(RPC::Server* serv);
	virtual					~NFS4Server();

			uint64			ClientId(uint64 prevId = 0, bool forceNew = false);
			void			ReleaseCID(uint64 cid);

	inline	uint32			SequenceId();
private:
			uint64			fClientId;
			uint32			fCIDUseCount;
			mutex			fLock;

			vint32			fSequenceId;

			RPC::Server*	fServer;
};

uint32
NFS4Server::SequenceId()
{
	return static_cast<uint32>(atomic_add(&fSequenceId, 1));
}


#endif	// NFS4SERVER_H

