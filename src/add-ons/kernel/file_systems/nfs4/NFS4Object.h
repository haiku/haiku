/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef NFS4OBJECT_H
#define NFS4OBJECT_H


#include "Cookie.h"
#include "FileSystem.h"
#include "NFS4Defs.h"


class NFS4Object {
public:
	bool		HandleErrors(uint32 nfs4Error, RPC::Server* serv,
					OpenFileCookie* cookie = NULL);

protected:
	FileInfo	fInfo;
	FileSystem*	fFileSystem;
};


#endif	// NFS4OBJECT_H

