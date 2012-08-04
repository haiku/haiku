/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef NFS4OBJECT_H
#define NFS4OBJECT_H

#include "FileInfo.h"
#include "NFS4Defs.h"
#include "RPCServer.h"


class OpenFileCookie;
class OpenState;

class NFS4Object {
public:
	bool		HandleErrors(uint32 nfs4Error, RPC::Server* serv,
					OpenFileCookie* cookie = NULL);

	status_t	ConfirmOpen(const FileHandle& fileHandle, OpenState* state);

	FileInfo	fInfo;
	FileSystem*	fFileSystem;
};


#endif	// NFS4OBJECT_H

