// RequestHandler.cpp

#include "RequestHandler.h"

// constructor
RequestHandler::RequestHandler()
	: fPort(NULL),
	  fDone(false)
{
}

// destructor
RequestHandler::~RequestHandler()
{
}

// SetPort
void
RequestHandler::SetPort(RequestPort* port)
{
	fPort = port;
}

// IsDone
bool
RequestHandler::IsDone() const
{
	return fDone;
}

