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


class OpenFileCookie;

class NFS4Server : public RPC::ProgramData {
public:
							NFS4Server(RPC::Server* serv);
	virtual					~NFS4Server();

			uint64			ServerRebooted(uint64 clientId);
			void			AddOpenFile(OpenFileCookie* cookie);
			void			RemoveOpenFile(OpenFileCookie* cookie);

			uint64			ClientId(uint64 prevId = 0, bool forceNew = false);
			void			ReleaseCID(uint64 cid);

	inline	uint32			SequenceId();

	inline	uint32			LeaseTime();
private:
			status_t		_ReclaimOpen(OpenFileCookie* cookie);

			status_t		_GetLeaseTime();

			status_t		_StartRenewing();
			status_t		_Renewal();
	static	status_t		_RenewalThreadStart(void* ptr);
			thread_id		fThread;
			bool			fThreadCancel;

			uint32			fLeaseTime;

			uint64			fClientId;
			uint32			fCIDUseCount;
			mutex			fLock;

			vint32			fSequenceId;

			OpenFileCookie*	fOpenFiles;
			mutex			fOpenLock;

			RPC::Server*	fServer;
};

inline uint32
NFS4Server::SequenceId()
{
	return static_cast<uint32>(atomic_add(&fSequenceId, 1));
}


inline uint32
NFS4Server::LeaseTime()
{
	if (fLeaseTime == 0)
		_GetLeaseTime();

	return fLeaseTime;
}


#endif	// NFS4SERVER_H

