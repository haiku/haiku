/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef RPCCALL_H
#define RPCCALL_H


#include "RPCAuth.h"
#include "XDR.h"


namespace RPC {

class Call {
public:
	static	Call*					Create(uint32 proc, const Auth* creds,
										const Auth* ver);
									~Call();

			void					SetXID(uint32 x);

	inline	XDR::WriteStream&		GetStream();

private:
									Call();

			XDR::Stream::Position	fXIDPosition;

			XDR::WriteStream		fStream;
};


inline XDR::WriteStream&
Call::GetStream()
{
	return fStream;
}

}		// namespace RPC


#endif	//	RPCCALL_H
