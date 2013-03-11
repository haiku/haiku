/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef RPCCALLBACKREPLY_H
#define RPCCALLBACKREPLY_H


#include "RPCDefs.h"
#include "XDR.h"


namespace RPC {

class CallbackReply {
public:
	static	CallbackReply*		Create(uint32 xid,
									AcceptStat rpcError = SUCCESS);
								~CallbackReply();

	inline	XDR::WriteStream&	Stream();

private:
								CallbackReply();

			XDR::WriteStream	fStream;
};


inline XDR::WriteStream&
CallbackReply::Stream()
{
	return fStream;
}

}		// namespace RPC


#endif	//	RPCCALLBACKREPLY_H
