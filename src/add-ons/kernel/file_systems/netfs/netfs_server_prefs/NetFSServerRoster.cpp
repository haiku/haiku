// NetFSServerRoster.cpp

#include <string.h>

#include <Message.h>
#include <OS.h>
#include <Roster.h>

#include "NetFSServerRoster.h"
#include "NetFSServerRosterDefs.h"

// constructor
NetFSServerRoster::NetFSServerRoster()
	: fServerMessenger()
{
}

// destructor
NetFSServerRoster::~NetFSServerRoster()
{
}

// IsServerRunning
bool
NetFSServerRoster::IsServerRunning()
{
	return (_InitMessenger() == B_OK);
}

// LaunchServer
status_t
NetFSServerRoster::LaunchServer()
{
	status_t error = BRoster().Launch(kNetFSServerSignature);
	if (error != B_OK)
		return error;

	return _InitMessenger();
}

// TerminateServer
status_t
NetFSServerRoster::TerminateServer(bool force, bigtime_t timeout)
{
	// get the server team
	team_id team = BRoster().TeamFor(kNetFSServerSignature);
	if (team < 0)
		return team;

	// create a semaphore an transfer its ownership to the server team
	sem_id deathSem = create_sem(0, "netfs server death");
	set_sem_owner(deathSem, team);

	status_t error = B_OK;
	if (force) {
		// terminate it the hard way
		kill_team(team);

		// wait the specified time
		error = acquire_sem_etc(deathSem, 1, B_RELATIVE_TIMEOUT, timeout);
	} else {
		// get a messenger
		BMessenger messenger(NULL, team);
		if (messenger.IsValid()) {
			// tell the server to quit
			messenger.SendMessage(B_QUIT_REQUESTED);
	
			// wait the specified time
			error = acquire_sem_etc(deathSem, 1, B_RELATIVE_TIMEOUT, timeout);
		} else
			error = B_ERROR;
	}

	delete_sem(deathSem);

	// if the semaphore is gone, the server is gone, as well
	return (error == B_BAD_SEM_ID ? B_OK : B_ERROR);
}

// SaveServerSettings
status_t
NetFSServerRoster::SaveServerSettings()
{
	// prepare the request
	BMessage request(NETFS_REQUEST_SAVE_SETTINGS);

	// send the request
	return _SendRequest(&request);
}


// #pragma mark -

// AddUser
status_t
NetFSServerRoster::AddUser(const char* user, const char* password)
{
	// check parameters
	if (!user || strlen(user) < 1)
		return B_BAD_VALUE;

	// prepare the request
	BMessage request(NETFS_REQUEST_ADD_USER);
	if (request.AddString("user", user) != B_OK
		|| (password && request.AddString("password", password) != B_OK)) {
		return B_ERROR;
	}

	// send the request
	return _SendRequest(&request);
}

// RemoveUser
status_t
NetFSServerRoster::RemoveUser(const char* user)
{
	// check parameters
	if (!user || strlen(user) < 1)
		return B_BAD_VALUE;

	// prepare the request
	BMessage request(NETFS_REQUEST_REMOVE_USER);
	if (request.AddString("user", user) != B_OK)
		return B_ERROR;

	// send the request
	return _SendRequest(&request);
}

// GetUsers
status_t
NetFSServerRoster::GetUsers(BMessage* users)
{
	// check parameters
	if (!users)
		return B_BAD_VALUE;

	// prepare the request
	BMessage request(NETFS_REQUEST_GET_USERS);

	// send the request
	BMessage reply;
	status_t error = _SendRequest(&request, &reply);
	if (error != B_OK)
		return error;

	// get the result
	if (reply.FindMessage("users", users) != B_OK)
		return B_ERROR;

	return B_OK;
}

// GetUserStatistics
status_t
NetFSServerRoster::GetUserStatistics(const char* user, BMessage* statistics)
{
	// check parameters
	if (!user || strlen(user) < 1 || !statistics)
		return B_BAD_VALUE;

	// prepare the request
	BMessage request(NETFS_REQUEST_GET_USER_STATISTICS);
	if (request.AddString("user", user) != B_OK)
		return B_ERROR;

	// send the request
	BMessage reply;
	status_t error = _SendRequest(&request, &reply);
	if (error != B_OK)
		return error;

	// get the result
	if (reply.FindMessage("statistics", statistics) != B_OK)
		return B_ERROR;

	return B_OK;
}


// #pragma mark -

// AddShare
status_t
NetFSServerRoster::AddShare(const char* share, const char* path)
{
	// check parameters
	if (!share || strlen(share) < 1 || !path || strlen(path) < 1)
		return B_BAD_VALUE;

	// prepare the request
	BMessage request(NETFS_REQUEST_ADD_SHARE);
	if (request.AddString("share", share) != B_OK
		|| request.AddString("path", path) != B_OK) {
		return B_ERROR;
	}

	// send the request
	return _SendRequest(&request);
}

// RemoveShare
status_t
NetFSServerRoster::RemoveShare(const char* share)
{
	// check parameters
	if (!share || strlen(share) < 1)
		return B_BAD_VALUE;

	// prepare the request
	BMessage request(NETFS_REQUEST_REMOVE_SHARE);
	if (request.AddString("share", share) != B_OK)
		return B_ERROR;

	// send the request
	return _SendRequest(&request);
}

// GetShares
status_t
NetFSServerRoster::GetShares(BMessage* shares)
{
	// check parameters
	if (!shares)
		return B_BAD_VALUE;

	// prepare the request
	BMessage request(NETFS_REQUEST_GET_SHARES);

	// send the request
	BMessage reply;
	status_t error = _SendRequest(&request, &reply);
	if (error != B_OK)
		return error;

	// get the result
	if (reply.FindMessage("shares", shares) != B_OK)
		return B_ERROR;

	return B_OK;
}

// GetShareUsers
status_t
NetFSServerRoster::GetShareUsers(const char* share, BMessage* users)
{
	// check parameters
	if (!share || strlen(share) < 1 || !users)
		return B_BAD_VALUE;

	// prepare the request
	BMessage request(NETFS_REQUEST_GET_SHARE_USERS);
	if (request.AddString("share", share) != B_OK)
		return B_ERROR;

	// send the request
	BMessage reply;
	status_t error = _SendRequest(&request, &reply);
	if (error != B_OK)
		return error;

	// get the result
	if (reply.FindMessage("users", users) != B_OK)
		return B_ERROR;

	return B_OK;
}

// GetShareStatistics
status_t
NetFSServerRoster::GetShareStatistics(const char* share, BMessage* statistics)
{
	// check parameters
	if (!share || strlen(share) < 1 || !statistics)
		return B_BAD_VALUE;

	// prepare the request
	BMessage request(NETFS_REQUEST_GET_SHARE_STATISTICS);
	if (request.AddString("share", share) != B_OK)
		return B_ERROR;

	// send the request
	BMessage reply;
	status_t error = _SendRequest(&request, &reply);
	if (error != B_OK)
		return error;

	// get the result
	if (reply.FindMessage("statistics", statistics) != B_OK)
		return B_ERROR;

	return B_OK;
}


// #pragma mark -

// SetUserPermissions
status_t
NetFSServerRoster::SetUserPermissions(const char* share, const char* user,
	uint32 permissions)
{
	// check parameters
	if (!share || strlen(share) < 1 || !user || strlen(user) < 1)
		return B_BAD_VALUE;

	// prepare the request
	BMessage request(NETFS_REQUEST_SET_USER_PERMISSIONS);
	if (request.AddString("share", share) != B_OK
		|| request.AddString("user", user) != B_OK
		|| request.AddInt32("permissions", (int32)permissions)) {
		return B_ERROR;
	}

	// send the request
	return _SendRequest(&request);
}

// GetUserPermissions
status_t
NetFSServerRoster::GetUserPermissions(const char* share, const char* user,
	uint32* permissions)
{
	// check parameters
	if (!share || strlen(share) < 1 || !user || strlen(user) < 1
		|| !permissions) {
		return B_BAD_VALUE;
	}

	// prepare the request
	BMessage request(NETFS_REQUEST_GET_USER_PERMISSIONS);
	if (request.AddString("share", share) != B_OK
		|| request.AddString("user", user) != B_OK) {
		return B_ERROR;
	}

	// send the request
	BMessage reply;
	status_t error = _SendRequest(&request, &reply);
	if (error != B_OK)
		return error;

	// get the result
	if (reply.FindInt32("permissions", (int32*)permissions) != B_OK)
		return B_ERROR;

	return B_OK;
}

// #pragma mark -

// _InitMessenger
status_t
NetFSServerRoster::_InitMessenger()
{
	// do we already have a valid messenger?
	if (fServerMessenger.IsValid())
		return B_OK;

	// get a messenger to the server application
	BMessenger appMessenger(kNetFSServerSignature);
	if (!appMessenger.IsValid())
		return B_NO_INIT;

	// send a request to get the real messenger
	BMessage request(NETFS_REQUEST_GET_MESSENGER);
	BMessage reply;
	if (appMessenger.SendMessage(&request, &reply) != B_OK)
		return B_NO_INIT;

	// check the result
	status_t error;
	if (reply.FindInt32("error", &error) != B_OK)
		return B_ERROR;
	if (error != B_OK)
		return error;

	// get the messenger
	if (reply.FindMessenger("messenger", &fServerMessenger) != B_OK)
		return B_NO_INIT;

	return (fServerMessenger.IsValid() ? B_OK : B_NO_INIT);
}

// _SendRequest
status_t
NetFSServerRoster::_SendRequest(BMessage* request, BMessage* reply)
{
	// if no reply data are expected, create a reply message on the stack
	BMessage stackReply;
	if (!reply)
		reply = &stackReply;

	// make sure the messenger is initialized
	status_t error = _InitMessenger();
	if (error != B_OK)
		return error;

	// send the request
	error = fServerMessenger.SendMessage(request, reply);
	if (error != B_OK)
		return error;

	// check the reply result
	status_t result;
	if (reply->FindInt32("error", &result) != B_OK)
		return B_ERROR;
	return result;
}
