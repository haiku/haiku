// RequestHandler.cpp

#include "Compatibility.h"
#include "RequestHandler.h"

// constructor
RequestHandler::RequestHandler()
	: RequestVisitor(),
	  fChannels()
{
}

// destructor
RequestHandler::~RequestHandler()
{
}

// HandleRequest
status_t
RequestHandler::HandleRequest(Request* request, RequestChannel* channel)
{
	if (!request)
		return B_BAD_VALUE;
	status_t error = fChannels.Set(channel);
	if (error != B_OK)
		return error;
	ThreadLocalUnsetter _(fChannels);
	return request->Accept(this);
}

// VisitAny
status_t
RequestHandler::VisitAny(Request* request)
{
	// unexpected request
	return B_BAD_DATA;
}

// GetChannel
RequestChannel*
RequestHandler::GetChannel() const
{
	return (RequestChannel*)fChannels.Get();
}

