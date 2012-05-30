/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef REPLYINTERPRETER_H
#define REPLYINTERPRETER_H


#include <SupportDefs.h>

#include "NFS4Defs.h"
#include "RPCReply.h"


struct AttrValue {
	uint8	fAttribute;
	union {
			uint32		fValue32;
			uint64		fValue64;
			void*		fPointer;
	} fData;
};

class ReplyInterpreter {
public:
						ReplyInterpreter(RPC::Reply* reply);
						~ReplyInterpreter();

			status_t	Access(uint32* supported, uint32* allowed);
			status_t	GetAttr(AttrValue** attrs, uint32* count);
			status_t	GetFH(Filehandle* fh);
	inline	status_t	LookUp();
	inline	status_t	PutFH();
	inline	status_t	PutRootFH();

private:
			status_t	_OperationError(Opcode op);

	static	status_t	_NFS4ErrorToHaiku(uint32 x);

			RPC::Reply*	fReply;
};


inline status_t
ReplyInterpreter::LookUp()
{
	return _OperationError(OpLookUp);
}


inline status_t
ReplyInterpreter::PutFH()
{
	return _OperationError(OpPutFH);
}


inline status_t
ReplyInterpreter::PutRootFH()
{
	return _OperationError(OpPutRootFH);
}


#endif	// REPLYINTERPRETER_H

