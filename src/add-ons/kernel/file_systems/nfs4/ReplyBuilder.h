/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef REPLYBUILDER_H
#define REPLYBUILDER_H


#include <SupportDefs.h>

#include "RPCCallbackReply.h"
#include "XDR.h"


class ReplyBuilder {
public:
									ReplyBuilder(uint32 xid);
									~ReplyBuilder();

			RPC::CallbackReply*		Reply();

			status_t				Recall(status_t status);

private:
			void					_InitHeader();

	static	uint32					_HaikuErrorToNFS4(status_t error);

			status_t				fStatus;
			XDR::Stream::Position	fStatusPosition;

			uint32					fOpCount;
			XDR::Stream::Position	fOpCountPosition;

			RPC::CallbackReply*		fReply;
};


#endif	// REPLYBUILDER_H

