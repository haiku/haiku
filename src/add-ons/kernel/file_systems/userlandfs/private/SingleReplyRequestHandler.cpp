// SingleReplyRequestHandler.cpp

#include "SingleReplyRequestHandler.h"

#include "Compatibility.h"
#include "Debug.h"
#include "Request.h"

// constructor
SingleReplyRequestHandler::SingleReplyRequestHandler()
	: RequestHandler(),
	  fAcceptAnyRequest(true),
	  fExpectedReply(0)
{
}

// constructor
SingleReplyRequestHandler::SingleReplyRequestHandler(uint32 expectedReply)
	: RequestHandler(),
	  fAcceptAnyRequest(false),
	  fExpectedReply(expectedReply)
{
}

// HandleRequest
status_t
SingleReplyRequestHandler::HandleRequest(Request* request)
{
	if (!fAcceptAnyRequest && request->GetType() != fExpectedReply) {
PRINT(("SingleReplyRequestHandler::HandleRequest(): unexpected request: %lu "
"expected was: %lu\n", request->GetType(), fExpectedReply));
#if USER
debugger("SingleReplyRequestHandler::HandleRequest(): unexpected request!");
#endif
		return B_BAD_DATA;
	}
	fDone = true;
	return B_OK;
}

