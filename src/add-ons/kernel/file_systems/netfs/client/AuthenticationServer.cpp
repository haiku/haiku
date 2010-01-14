// AuthenticationServer.cpp

#include "AuthenticationServer.h"

#include <string.h>

#include <util/KMessage.h>

#include "AuthenticationServerDefs.h"
#include "Compatibility.h"

// constructor
AuthenticationServer::AuthenticationServer()
	: fServerPort(-1)
{
	fServerPort = find_port(kAuthenticationServerPortName);
}

// destructor
AuthenticationServer::~AuthenticationServer()
{
}

// InitCheck
status_t
AuthenticationServer::InitCheck() const
{
	return (fServerPort >= 0 ? B_OK : fServerPort);
}

// GetAuthentication
status_t
AuthenticationServer::GetAuthentication(const char* context, const char* server,
	const char* share, uid_t uid, bool badPassword,
	bool* _cancelled, char* _foundUser, int32 foundUserSize,
	char* _foundPassword, int32 foundPasswordSize)
{
	// check parameters/initialization
	if (!context || !server || !share || !_foundPassword)
		return B_BAD_VALUE;
	if (InitCheck() != B_OK)
		return InitCheck();
	// prepare the request
	KMessage request;
	status_t error = request.AddInt32("uid", uid);
	if (error != B_OK)
		return error;
	error = request.AddString("context", context);
	if (error != B_OK)
		return error;
	error = request.AddString("server", server);
	if (error != B_OK)
		return error;
	error = request.AddString("share", share);
	if (error != B_OK)
		return error;
	error = request.AddBool("badPassword", badPassword);
	if (error != B_OK)
		return error;
	// send the request
	KMessage reply;
	error = request.SendTo(fServerPort, -1, &reply);
	if (error != B_OK)
		return error;
	// process the reply
	// error
	if (reply.FindInt32("error", &error) != B_OK)
		return B_ERROR;
	if (error != B_OK)
		return error;
	// cancelled
	bool cancelled = false;
	if (reply.FindBool("cancelled", &cancelled) != B_OK)
		return B_ERROR;
	if (_cancelled)
		*_cancelled = cancelled;
	if (cancelled)
		return B_OK;
	// user/password
	const char* foundUser = NULL;
	const char* foundPassword = NULL;
	if (reply.FindString("user", &foundUser) != B_OK
		|| reply.FindString("password", &foundPassword) != B_OK) {
		return B_ERROR;
	}
	// set results
	if (_foundUser) {
		if (foundUserSize <= (int32)strlen(foundUser))
			return B_BUFFER_OVERFLOW;
		strcpy(_foundUser, foundUser);
	}
	if (_foundPassword) {
		if (foundPasswordSize <= (int32)strlen(foundPassword))
			return B_BUFFER_OVERFLOW;
		strcpy(_foundPassword, foundPassword);
	}
	return B_OK;
}

