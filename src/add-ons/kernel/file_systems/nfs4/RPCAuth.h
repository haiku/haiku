/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef RPCAUTH_H
#define RPCAUTH_H


#include "XDR.h"


namespace RPC {

class Auth {
public:
	inline	const XDR::WriteStream&		Stream() const;

	static	const Auth*					CreateNone();
	static	const Auth*					CreateSys();

private:
										Auth();

			XDR::WriteStream			fStream;
};


inline const XDR::WriteStream&
Auth::Stream() const
{
	return fStream;
}

}		// namespace RPC


#endif	// RPCAUTH_H

