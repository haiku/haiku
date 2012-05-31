/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef FILESYSTEM_H
#define FILESYSTEM_H


#include "NFS4Defs.h"
#include "RPCServer.h"


class Inode;

class Filesystem {
public:
	static	status_t		Mount(Filesystem** pfs, RPC::Server* serv,
								const char* path, dev_t id);
							~Filesystem();

			Inode*			CreateRootInode();

	inline	uint32			FHExpiryType() const;
	inline	RPC::Server*	Server();
	inline	uint64			GetId();

	inline	dev_t			DevId() const;
private:
							Filesystem();

			uint32			fFHExpiryType;

			Filehandle		fRootFH;

			RPC::Server*	fServer;

			vint64			fId;
			dev_t			fDevId;
};


inline RPC::Server*
Filesystem::Server()
{
	return fServer;
}


inline uint64
Filesystem::GetId()
{
	return atomic_add64(&fId, 1);
}


inline dev_t
Filesystem::DevId() const
{
	return fDevId;
}


#endif	// FILESYSTEM_H

