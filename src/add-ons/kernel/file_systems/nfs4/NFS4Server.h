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

#include "ReplyBuilder.h"
#include "RequestInterpreter.h"
#include "RPCServer.h"


class FileSystem;
class OpenFileCookie;

class NFS4Server : public RPC::ProgramData {
public:
							NFS4Server(RPC::Server* serv);
	virtual					~NFS4Server();

			uint64			ServerRebooted(uint64 clientId);
			status_t		FileSystemMigrated();

			void			AddFileSystem(FileSystem* fs);
			void			RemoveFileSystem(FileSystem* fs);

	inline	void			IncUsage();
	inline	void			DecUsage();

			uint64			ClientId(uint64 prevId = 0, bool forceNew = false);

	inline	uint32			LeaseTime();

	virtual	status_t		ProcessCallback(RPC::CallbackRequest* request,
								Connection* connection);

			status_t		CallbackRecall(RequestInterpreter* request,
								ReplyBuilder* reply);
			status_t		RecallAll();
private:
			status_t		_GetLeaseTime();

			status_t		_StartRenewing();
			status_t		_Renewal();
	static	status_t		_RenewalThreadStart(void* ptr);

			thread_id		fThread;
			bool			fThreadCancel;
			sem_id			fWaitCancel;
			mutex			fThreadStartLock;

			uint32			fLeaseTime;

			uint64			fClientId;
			bool			fClientIdInit;
			time_t			fClientIdLastUse;
			mutex			fClientIdLock;

			uint32			fUseCount;
			FileSystem*		fFileSystems;
			mutex			fFSLock;

			RPC::Server*	fServer;
};


inline void
NFS4Server::IncUsage()
{
	MutexLocker _(fFSLock);
	fUseCount++;
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

