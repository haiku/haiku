/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef REQUESTBUILDER_H
#define REQUESTBUILDER_H


#include <SupportDefs.h>

#include "NFS4Defs.h"
#include "RPCCall.h"
#include "XDR.h"


class RequestBuilder {
public:
									RequestBuilder(Procedure proc);
									~RequestBuilder();

			status_t				Access();
			status_t				GetAttr(Attribute* attrs, uint32 count);
			status_t				GetFH();
			status_t				LookUp(const char* name);
			status_t				PutRootFH();

			RPC::Call*				Request();

private:
			uint32					fOpCount;
			XDR::Stream::Position	fOpCountPosition;

			Procedure				fProcedure;

			RPC::Call*				fRequest;
};


#endif	// REQUESTBUILDER_H

