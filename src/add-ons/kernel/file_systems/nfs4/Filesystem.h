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


class Filesystem {
public:
	static	status_t		Mount(Filesystem** pfs, RPC::Server* serv,
								const char* path);
							~Filesystem();

	inline	uint32			FHExpiryType() const;

private:
							Filesystem();

			uint32			fFHExpiryType;

			Filehandle		fRootFH;

			RPC::Server*	fServer;
};


#endif	// FILESYSTEM_H

