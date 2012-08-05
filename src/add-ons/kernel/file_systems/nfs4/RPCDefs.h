/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef RPCDEFS_H
#define RPCDEFS_H


namespace RPC {

enum {
	VERSION			= 2
};

enum {
	PROGRAM_NFS		= 100003,
	PROGRAM_NFS_CB	= 0x40000000
};

enum {
	NFS_VERSION		= 4,
	NFS_CB_VERSION	= 1
};

enum {
	CALL		= 0,
	REPLY		= 1
};

enum {
	MSG_ACCEPTED	= 0,
	MSG_DENIED		= 1
};

enum AcceptStat {
	SUCCESS       = 0, /* RPC executed successfully       */
	PROG_UNAVAIL  = 1, /* remote hasn't exported program  */
	PROG_MISMATCH = 2, /* remote can't support version #  */
	PROC_UNAVAIL  = 3, /* program can't support procedure */
	GARBAGE_ARGS  = 4, /* procedure can't decode params   */
	SYSTEM_ERR    = 5  /* e.g. memory allocation failure  */
};

enum RejectStat {
	RPC_MISMATCH = 0, /* RPC version number != 2          */
	AUTH_ERROR = 1    /* remote can't authenticate caller */
};

enum AuthFlavour {
	AUTH_NONE	= 0,
	AUTH_SYS	= 1
};

}		// namespace RPC


#endif	// RPCDEFS_H

