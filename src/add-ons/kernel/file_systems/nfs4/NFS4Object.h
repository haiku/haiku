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


class OpenStateCookie;
class OpenState;

class NFS4Object {
public:
			bool		HandleErrors(uint32 nfs4Error, RPC::Server* serv,
							OpenStateCookie* cookie = NULL,
							OpenState* state = NULL, uint32* sequence = NULL);

			status_t	ConfirmOpen(const FileHandle& fileHandle,
							OpenState* state, uint32* sequence);

	static	uint32		IncrementSequence(uint32 error);

	inline				NFS4Object();

			FileInfo	fInfo;
			FileSystem*	fFileSystem;
};


inline
NFS4Object::NFS4Object()
	:
	fFileSystem(NULL)
{
}


#endif	// NFS4OBJECT_H

