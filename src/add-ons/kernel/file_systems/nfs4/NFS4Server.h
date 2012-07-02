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


class Filesystem;
class OpenFileCookie;

class NFS4Server : public RPC::ProgramData {
public:
							NFS4Server(RPC::Server* serv);
	virtual					~NFS4Server();

			uint64			ServerRebooted(uint64 clientId);
			status_t		FilesystemMigrated();

			void			AddFilesystem(Filesystem* fs);
			void			RemoveFilesystem(Filesystem* fs);

	inline	void			IncUsage();
	inline	void			DecUsage();

			uint64			ClientId(uint64 prevId = 0, bool forceNew = false);

	inline	uint32			LeaseTime();
private:
			status_t		_ReclaimOpen(OpenFileCookie* cookie);
			status_t		_ReclaimLocks(OpenFileCookie* cookie);

			status_t		_GetLeaseTime();

			status_t		_StartRenewing();
			status_t		_Renewal();
	static	status_t		_RenewalThreadStart(void* ptr);
			thread_id		fThread;
			bool			fThreadCancel;

			uint32			fLeaseTime;

			uint64			fClientId;
			bool			fClientIdInit;
			time_t			fClientIdLastUse;
			mutex			fClientIdLock;

			uint32			fUseCount;
			Filesystem*		fFilesystems;
			mutex			fFSLock;

			RPC::Server*	fServer;
};


inline void
NFS4Server::IncUsage()
{
	MutexLocker _(fFSLock);
	fUseCount++;
	if (fThreadCancel)
		_StartRenewing();
	fClientIdLastUse = time(NULL);
}


inline void
NFS4Server::DecUsage()
{
	MutexLocker _(fFSLock);
	fClientIdLastUse = time(NULL);
	fUseCount--;
}


inline uint32
NFS4Server::LeaseTime()
{
	if (fLeaseTime == 0)
		_GetLeaseTime();

	return fLeaseTime;
}


#endif	// NFS4SERVER_H

