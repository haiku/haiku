//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		TRoster.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	TRoster is the incarnation of The Roster. It manages the
//					running applications.
//------------------------------------------------------------------------------

#include <new.h>

#include <Application.h>

#include "Debug.h"
#include "RegistrarDefs.h"
#include "RosterAppInfo.h"
#include "TRoster.h"

// constructor
TRoster::TRoster()
	   : fRegisteredApps(),
		 fEarlyPreRegisteredApps(),
		 fIAPRRequests(),
		 fActiveApp(-1),
		 fLastToken(0)
{
}

// destructor
TRoster::~TRoster()
{
}

// HandleAddApplication
void
TRoster::HandleAddApplication(BMessage *request)
{
	FUNCTION_START();

	status_t error = B_OK;
	// get the parameters
	const char *signature;
	entry_ref ref;
	uint32 flags;
	team_id team;
	thread_id thread;
	port_id port;
	bool fullReg;
	if (request->FindString("signature", &signature) != B_OK)
		signature = NULL;
	if (request->FindRef("ref", &ref) != B_OK)
		SET_ERROR(error, B_BAD_VALUE);
	if (request->FindInt32("flags", (int32*)&flags) != B_OK)
		flags = B_REG_DEFAULT_APP_FLAGS;
	if (request->FindInt32("team", &team) != B_OK)
		team = -1;
	if (request->FindInt32("thread", &thread) != B_OK)
		thread = -1;
	if (request->FindInt32("port", &port) != B_OK)
		port = -1;
	if (request->FindBool("full_registration", &fullReg) != B_OK)
		fullReg = false;
	// check the parameters
	team_id otherTeam = -1;
	uint32 launchFlags = flags & B_LAUNCH_MASK;
	// entry_ref
	if (error == B_OK) {
		// the entry_ref must be valid
		if (BEntry(&ref).Exists()) {
PRINT(("flags: %lx\n", flags));
PRINT(("ref: %ld, %lld, %s\n", ref.device, ref.directory, ref.name));
			// check single/exclusive launchers
			RosterAppInfo *info = NULL;
			if ((launchFlags == B_SINGLE_LAUNCH
				 || launchFlags ==  B_EXCLUSIVE_LAUNCH)
				&& (((info = fRegisteredApps.InfoFor(&ref)))
					|| ((info = fEarlyPreRegisteredApps.InfoFor(&ref))))) {
				SET_ERROR(error, B_ALREADY_RUNNING);
				otherTeam = info->team;
			}
		} else
			SET_ERROR(error, B_ENTRY_NOT_FOUND);
	}
	// signature
	if (error == B_OK && signature) {
		// check exclusive launchers
		RosterAppInfo *info = NULL;
		if (launchFlags == B_EXCLUSIVE_LAUNCH
			&& (((info = fRegisteredApps.InfoFor(signature)))
				|| ((info = fEarlyPreRegisteredApps.InfoFor(signature))))) {
			SET_ERROR(error, B_ALREADY_RUNNING);
			otherTeam = info->team;
		}
	}
	// If no team ID is given, full registration isn't possible.
	if (error == B_OK) {
		if (team < 0) {
			if (fullReg)
				SET_ERROR(error, B_BAD_VALUE);
		} else if (fRegisteredApps.InfoFor(team))
			SET_ERROR(error, B_REG_ALREADY_REGISTERED);
	}
	// Add the application info.
	uint32 token = 0;
	if (error == B_OK) {
		// alloc and init the info
		RosterAppInfo *info = new(nothrow) RosterAppInfo;
		if (info) {
			info->Init(thread, team, port, flags, &ref, signature);
			if (fullReg)
				info->state = APP_STATE_REGISTERED;
			else
				info->state = APP_STATE_PRE_REGISTERED;
			info->registration_time = system_time();
			// add it to the right list
			bool addingSuccess = false;
			if (team >= 0)
{
PRINT(("added ref: %ld, %lld, %s\n", info->ref.device, info->ref.directory, info->ref.name));
				addingSuccess = fRegisteredApps.AddInfo(info);
}
			else {
				token = info->token = _NextToken();
				addingSuccess = fEarlyPreRegisteredApps.AddInfo(info);
			}
			if (!addingSuccess)
				SET_ERROR(error, B_NO_MEMORY);
		} else
			SET_ERROR(error, B_NO_MEMORY);
		// delete the info on failure
		if (error != B_OK && info)
			delete info;
	}
	// reply to the request
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		// The token is valid only when no team ID has been supplied.
		if (team < 0)
			reply.AddInt32("token", (int32)token);
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		if (otherTeam >= 0)
			reply.AddInt32("other_team", otherTeam);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}

// HandleCompleteRegistration
void
TRoster::HandleCompleteRegistration(BMessage *request)
{
	FUNCTION_START();

	status_t error = B_OK;
	// get the parameters
	team_id team;
	thread_id thread;
	port_id port;
	if (request->FindInt32("team", &team) != B_OK)
		team = -1;
	if (request->FindInt32("thread", &thread) != B_OK)
		thread = -1;
	if (request->FindInt32("port", &port) != B_OK)
		port = -1;
	// check the parameters
	// port
	if (error == B_OK && port < 0)
		SET_ERROR(error, B_BAD_VALUE);
	// thread
	if (error == B_OK && thread < 0)
		SET_ERROR(error, B_BAD_VALUE);
	// team
	if (error == B_OK) {
		if (team >= 0) {
			// everything is fine -- set the values
			if (RosterAppInfo *info = fRegisteredApps.InfoFor(team)) {
				info->thread = thread;
				info->port = port;
				info->state = APP_STATE_REGISTERED;
			} else
				SET_ERROR(error, B_REG_APP_NOT_REGISTERED);
		} else
			SET_ERROR(error, B_BAD_VALUE);
	}
	// reply to the request
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}

// HandleIsAppPreRegistered
void
TRoster::HandleIsAppPreRegistered(BMessage *request)
{
	FUNCTION_START();

	status_t error = B_OK;
	// get the parameters
	entry_ref ref;
	team_id team;
	thread_id thread;
	if (request->FindRef("ref", &ref) != B_OK)
		SET_ERROR(error, B_BAD_VALUE);
	if (request->FindInt32("team", &team) != B_OK)
		team = -1;
	if (request->FindInt32("thread", &thread) != B_OK)
		thread = -1;
	// check the parameters
	// entry_ref
	if (error == B_OK & !BEntry(&ref).Exists())
		SET_ERROR(error, B_ENTRY_NOT_FOUND);
	// team
	if (error == B_OK && team < 0)
		SET_ERROR(error, B_BAD_VALUE);
	// loop the information up
	RosterAppInfo *info = NULL;
	if (error == B_OK) {
		if ((info = fRegisteredApps.InfoFor(team)) != NULL) {
			_ReplyToIAPRRequest(request, info);
		} else if ((info = fEarlyPreRegisteredApps.InfoFor(&ref)) != NULL) {
			// pre-registered and has no team ID assigned yet -- queue the
			// request
			be_app->DetachCurrentMessage();
			IAPRRequest queuedRequest = { ref, team, request };
			fIAPRRequests[team] = queuedRequest;
		}
	}
	// reply to the request
	if (error == B_OK)
		_ReplyToIAPRRequest(request, info);
	else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}

// HandleRemovePreRegApp
void
TRoster::HandleRemovePreRegApp(BMessage *request)
{
	FUNCTION_START();

	status_t error = B_OK;
	// get the parameters
	uint32 token;
	if (request->FindInt32("token", (int32*)&token) != B_OK)
		SET_ERROR(error, B_BAD_VALUE);
	// remove the app
	if (error == B_OK) {
		RosterAppInfo *info = fEarlyPreRegisteredApps.InfoForToken(token);
		if (info) {
			fEarlyPreRegisteredApps.RemoveInfo(info);
			delete info;
		} else
			SET_ERROR(error, B_REG_APP_NOT_PRE_REGISTERED);
	}
	// reply to the request
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}

// HandleRemoveApp
void
TRoster::HandleRemoveApp(BMessage *request)
{
	FUNCTION_START();

	status_t error = B_OK;
	// get the parameters
	team_id team;
	if (request->FindInt32("team", &team) != B_OK)
		team = -1;
	// remove the app
	if (error == B_OK) {
		if (RosterAppInfo *info = fRegisteredApps.InfoFor(team)) {
			fRegisteredApps.RemoveInfo(info);
			// Cleanup activities go here (e.g. removing message runners)
			// ...
			delete info;
		} else
			SET_ERROR(error, B_REG_APP_NOT_REGISTERED);
	}
	// reply to the request
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}

// HandleSetThreadAndTeam
void
TRoster::HandleSetThreadAndTeam(BMessage *request)
{
	FUNCTION_START();

	status_t error = B_OK;
	// get the parameters
	team_id team;
	thread_id thread;
	uint32 token;
	if (request->FindInt32("team", &team) != B_OK)
		team = -1;
	if (request->FindInt32("thread", &thread) != B_OK)
		thread = -1;
	if (request->FindInt32("token", (int32*)&token) != B_OK)
		SET_ERROR(error, B_BAD_VALUE);
	// check the parameters
	// team
	if (error == B_OK && team < 0)
		SET_ERROR(error, B_BAD_VALUE);
	// update the app_info
	if (error == B_OK) {
		RosterAppInfo *info = fEarlyPreRegisteredApps.InfoForToken(token);
		if (info) {
			// Set thread and team, create a port for the application and
			// move the app_info from the list of the early pre-registered
			// apps to the list of the (pre-)registered apps.
			fEarlyPreRegisteredApps.RemoveInfo(info);
			info->team = team;
			info->thread = thread;
			// create and transfer the port
			info->port = create_port(B_REG_APP_LOOPER_PORT_CAPACITY,
									 kRAppLooperPortName);
			if (info->port < 0)
				SET_ERROR(error, info->port);
			if (error == B_OK)
				SET_ERROR(error, set_port_owner(info->port, team));
			// add the info to the registered apps list
			if (error == B_OK && !fRegisteredApps.AddInfo(info))
				SET_ERROR(error, B_NO_MEMORY);
			// cleanup on failure
			if (error != B_OK) {
				if (info->port >= 0)
					delete_port(info->port);
				delete info;
			}
			// handle a pending IsAppPreRegistered() request
			IAPRRequestMap::iterator it = fIAPRRequests.find(team);
			if (it != fIAPRRequests.end()) {
				IAPRRequest &request = it->second;
				if (error == B_OK)
					_ReplyToIAPRRequest(request.request, info);
				delete request.request;
				fIAPRRequests.erase(it);
			}
		} else
			SET_ERROR(error, B_REG_APP_NOT_PRE_REGISTERED);
	}
	// reply to the request
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}

// HandleGetRunningAppInfo
void
TRoster::HandleGetRunningAppInfo(BMessage *request)
{
	FUNCTION_START();

	status_t error = B_OK;
	// get the parameters
	team_id team;
	if (request->FindInt32("team", &team) != B_OK)
		team = -1;
	// check the parameters
	// team -- in case no team is supplied, the active team is meant
	if (error == B_OK && team < 0) {
		if (fActiveApp >= 0)
			team = fActiveApp;
		else
			error = B_ENTRY_NOT_FOUND;
	}
	// get the info
	RosterAppInfo *info = NULL;
	if (error == B_OK) {
		info = fRegisteredApps.InfoFor(team);
		if (info == NULL)
			error = B_ENTRY_NOT_FOUND;
else
PRINT(("found ref: %ld, %lld, %s\n", info->ref.device, info->ref.directory, info->ref.name));
	}
	// reply to the request
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		_AddMessageAppInfo(&reply, info);
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}


// _AddMessageAppInfo
status_t
TRoster::_AddMessageAppInfo(BMessage *message, const app_info *info)
{
	// An app_info is not completely flat. The entry_ref contains a string
	// pointer. Therefore we flatten the info.
	flat_app_info flatInfo;
	flatInfo.info = *info;
	// set the ref name to NULL and copy it into the flat structure
	flatInfo.info.ref.name = NULL;
	flatInfo.ref_name[0] = '\0';
	if (info->ref.name)
		strcpy(flatInfo.ref_name, info->ref.name);
	// add the flat info
	return message->AddData("app_info", B_REG_APP_INFO_TYPE, &flatInfo,
							sizeof(flat_app_info));
}

// _NextToken
uint32
TRoster::_NextToken()
{
	return ++fLastToken;
}

// _ReplyToIAPRRequest
void
TRoster::_ReplyToIAPRRequest(BMessage *request, const RosterAppInfo *info)
{
	// pre-registered or registered?
	bool preRegistered = false;
	if (info) {
		switch (info->state) {
			case APP_STATE_PRE_REGISTERED:
				preRegistered = true;
				break;
			case APP_STATE_INVALID:
			case APP_STATE_REGISTERED:
				preRegistered = false;
				break;
		}
	}
	// send reply
	BMessage reply(B_REG_SUCCESS);
	reply.AddBool("pre-registered", preRegistered);
	if (preRegistered)
		_AddMessageAppInfo(&reply, info);
	request->SendReply(&reply);
}

