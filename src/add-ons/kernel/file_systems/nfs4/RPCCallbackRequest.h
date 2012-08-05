/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef RPCCALLBACKREQUEST_H
#define RPCCALLBACKREQUEST_H


#include "XDR.h"


namespace RPC {

class CallbackRequest {
public:
								CallbackRequest(void *buffer, int size);
								~CallbackRequest();

	inline	uint32				XID();
	inline	uint32				ID();

	inline	uint32				Procedure();

	inline	status_t			Error();
	inline	XDR::ReadStream&	Stream();

private:
			uint32				fXID;
			uint32				fID;

			uint32				fProcedure;

			status_t			fError;

			XDR::ReadStream		fStream;
			void*				fBuffer;
};


inline uint32
CallbackRequest::XID()
{
	return fXID;
}


inline uint32
CallbackRequest::ID()
{
	return fID;
}


inline uint32
CallbackRequest::Procedure()
{
	return fProcedure;
}


inline status_t
CallbackRequest::Error()
{
	return fError;
}


inline XDR::ReadStream&
CallbackRequest::Stream()
{
	return fStream;
}


}		// namespace RPC


#endif	// RPCCALLBACKREQUEST_H

