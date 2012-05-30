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
								const char* path);
							~Filesystem();

			Inode*			CreateRootInode();

	inline	uint32			FHExpiryType() const;
	inline	RPC::Server*	Server();
	inline	uint64			GetId();

private:
							Filesystem();

			uint32			fFHExpiryType;

			Filehandle		fRootFH;

			RPC::Server*	fServer;

			vint64			fId;
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


#endif	// FILESYSTEM_H

