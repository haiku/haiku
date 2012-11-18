/*
 * Copyright 2001-2008, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */


/*!	TRoster is the incarnation of The Roster. It manages the running
	applications.
*/


#include "TRoster.h"

#include <new>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <AutoDeleter.h>
#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

#include <AppMisc.h>
#include <MessagePrivate.h>
#include <MessengerPrivate.h>
#include <RosterPrivate.h>
#include <ServerProtocol.h>
#include <storage_support.h>

#include "AppInfoListMessagingTargetSet.h"
#include "Debug.h"
#include "EventMaskWatcher.h"
#include "MessageDeliverer.h"
#include "RegistrarDefs.h"
#include "RosterAppInfo.h"
#include "RosterSettingsCharStream.h"

using std::nothrow;
using namespace BPrivate;


/*!	\class TRoster
	\brief Implements the application roster.

	This class handles the BRoster requests. For each kind a hook method is
	implemented to which the registrar looper dispatches the request messages.

	Registered and pre-registered are managed via AppInfoLists.
	\a fEarlyPreRegisteredApps contains the infos for those application that
	are pre-registered and currently have no team ID assigned to them yet,
	whereas the infos of registered and pre-registered applications with a
	team ID are to be found in \a fRegisteredApps.

	When an application asks whether it is pre-registered or not and there
	are one or more instances of the application that are pre-registered, but
	have no team ID assigned yet, the reply to the request has to be
	postponed until the status of the requesting team is clear. The request
	message is dequeued from the registrar's message queue and added to
	\a fIARRequestsByID for a later reply.

	The field \a fActiveApp identifies the currently active application
	and \a fLastToken is a counter used to generate unique tokens for
	pre-registered applications.
*/

//! The maximal period of time an app may be early pre-registered (60 s).
const bigtime_t kMaximalEarlyPreRegistrationPeriod = 60000000LL;


//	#pragma mark - Private local functions


/*!	\brief Returns the path to the default roster settings.

	\param path BPath to be set to the roster settings path.
	\param createDirectory makes sure the target directory exists if \c true.

	\return the settings path as C string (\code path.Path() \endcode).
*/
static const char*
get_default_roster_settings_path(BPath& path, bool createDirectory)
{
	// get the path of the settings dir and append the subpath of our file
	status_t error = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (error == B_OK)
		error = path.Append("system/registrar");
	if (error == B_OK && createDirectory)
		error = create_directory(path.Path(), 0777);
	if (error == B_OK)
		error = path.Append("RosterSettings");

	return path.Path();
}


/*! \brief Returns true if entry1's index is larger than entry2's index.

	Also returns true if either entry is \c NULL.

	Used for sorting the recent entry lists loaded from disk into the
	proper order.
*/
bool
larger_index(const recent_entry* entry1, const recent_entry* entry2)
{
	if (entry1 && entry2)
		return entry1->index > entry2->index;

	return true;
}


//	#pragma mark -


/*!	\brief Creates a new roster.

	The object is completely initialized and ready to handle requests.
*/
TRoster::TRoster()
	:
	fLock("roster"),
	fRegisteredApps(),
	fEarlyPreRegisteredApps(),
	fIARRequestsByID(),
	fIARRequestsByToken(),
	fActiveApp(NULL),
	fWatchingService(),
	fRecentApps(),
	fRecentDocuments(),
	fRecentFolders(),
	fLastToken(0),
	fShuttingDown(false)
{
	find_directory(B_SYSTEM_DIRECTORY, &fSystemAppPath);
	find_directory(B_SYSTEM_SERVERS_DIRECTORY, &fSystemServerPath);
}


/*!	\brief Frees all resources associated with this object.
*/
TRoster::~TRoster()
{
}


/*!	\brief Handles an AddApplication() request.
	\param request The request message
*/
void
TRoster::HandleAddApplication(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	status_t error = B_OK;
	// get the parameters
	const char* signature;
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

	PRINT("team: %ld, signature: %s\n", team, signature);
	PRINT("full registration: %d\n", fullReg);

	if (fShuttingDown)
		error = B_SHUTTING_DOWN;

	// check the parameters
	team_id otherTeam = -1;
	uint32 token = 0;

	uint32 launchFlags = flags & B_LAUNCH_MASK;
	BEntry entry(&ref);
	if (!entry.Exists())
		SET_ERROR(error, B_ENTRY_NOT_FOUND);

	if (error == B_OK)
		_ValidateRunning(ref, signature);

	// entry_ref
	if (error == B_OK) {
		PRINT("flags: %lx\n", flags);
		PRINT("ref: %ld, %lld, %s\n", ref.device, ref.directory, ref.name);
		// check single/exclusive launchers
		RosterAppInfo* info = NULL;
		if ((launchFlags == B_SINGLE_LAUNCH
			 || launchFlags ==  B_EXCLUSIVE_LAUNCH)
			&& ((info = fRegisteredApps.InfoFor(&ref)) != NULL
				|| (info = fEarlyPreRegisteredApps.InfoFor(&ref)) != NULL)) {
			SET_ERROR(error, B_ALREADY_RUNNING);
			otherTeam = info->team;
			token = info->token;
		}
	}

	// signature
	if (error == B_OK && signature) {
		// check exclusive launchers
		RosterAppInfo* info = NULL;
		if (launchFlags == B_EXCLUSIVE_LAUNCH
			&& (((info = fRegisteredApps.InfoFor(signature)))
				|| ((info = fEarlyPreRegisteredApps.InfoFor(signature))))) {
			SET_ERROR(error, B_ALREADY_RUNNING);
			otherTeam = info->team;
			token = info->token;
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
	if (error == B_OK) {
		// alloc and init the info
		RosterAppInfo* info = new(nothrow) RosterAppInfo;
		if (info) {
			info->Init(thread, team, port, flags, &ref, signature);
			if (fullReg)
				info->state = APP_STATE_REGISTERED;
			else
				info->state = APP_STATE_PRE_REGISTERED;
			info->registration_time = system_time();
			// add it to the right list
			bool addingSuccess = false;
			if (team >= 0) {
				PRINT("added ref: %ld, %lld, %s\n", info->ref.device,
					info->ref.directory, info->ref.name);
				addingSuccess = (AddApp(info) == B_OK);
				if (addingSuccess && fullReg)
					_AppAdded(info);
			} else {
				token = info->token = _NextToken();
				addingSuccess = fEarlyPreRegisteredApps.AddInfo(info);
				PRINT("added to early pre-regs, token: %lu\n", token);
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
		// add to recent apps if successful
		if (signature && signature[0] != '\0')
			fRecentApps.Add(signature, flags);
		else
			fRecentApps.Add(&ref, flags);

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
		if (token > 0)
			reply.AddInt32("token", (int32)token);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}


/*!	\brief Handles a CompleteRegistration() request.
	\param request The request message
*/
void
TRoster::HandleCompleteRegistration(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

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

	if (fShuttingDown)
		error = B_SHUTTING_DOWN;

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
			RosterAppInfo* info = fRegisteredApps.InfoFor(team);
			if (info && info->state == APP_STATE_PRE_REGISTERED) {
				info->thread = thread;
				info->port = port;
				info->state = APP_STATE_REGISTERED;
				_AppAdded(info);
			} else
				SET_ERROR(error, B_REG_APP_NOT_PRE_REGISTERED);
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


/*!	\brief Handles an IsAppRegistered() request.
	\param request The request message
*/
void
TRoster::HandleIsAppRegistered(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	status_t error = B_OK;
	// get the parameters
	entry_ref ref;
	team_id team;
	uint32 token;
	if (request->FindRef("ref", &ref) != B_OK)
		SET_ERROR(error, B_BAD_VALUE);
	if (request->FindInt32("team", &team) != B_OK)
		team = -1;
	if (request->FindInt32("token", (int32*)&token) != B_OK)
		token = 0;

	PRINT("team: %ld, token: %lu\n", team, token);
	PRINT("ref: %ld, %lld, %s\n", ref.device, ref.directory, ref.name);

	// check the parameters
	// entry_ref
	if (error == B_OK && !BEntry(&ref).Exists())
		SET_ERROR(error, B_ENTRY_NOT_FOUND);
	// team/token
	if (error == B_OK && team < 0 && token == 0)
		SET_ERROR(error, B_BAD_VALUE);

	// look up the information
	RosterAppInfo* info = NULL;
	if (error == B_OK) {
		if ((info = fRegisteredApps.InfoFor(team)) != NULL) {
			PRINT("found team in fRegisteredApps\n");
			_ReplyToIARRequest(request, info);
		} else if (token > 0
			&& (info = fEarlyPreRegisteredApps.InfoForToken(token)) != NULL) {
			PRINT("found ref in fEarlyRegisteredApps (by token)\n");
			// pre-registered and has no team ID assigned yet -- queue the
			// request
			be_app->DetachCurrentMessage();
			_AddIARRequest(fIARRequestsByToken, token, request);
		} else if (team >= 0
			&& (info = fEarlyPreRegisteredApps.InfoFor(&ref)) != NULL) {
			PRINT("found ref in fEarlyRegisteredApps (by ref)\n");
			// pre-registered and has no team ID assigned yet -- queue the
			// request
			be_app->DetachCurrentMessage();
			_AddIARRequest(fIARRequestsByID, team, request);
		} else {
			PRINT("didn't find team or ref\n");
			// team not registered, ref/token not early pre-registered
			_ReplyToIARRequest(request, NULL);
		}
	} else {
		// reply to the request on error
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}


/*!	\brief Handles a RemovePreRegApp() request.
	\param request The request message
*/
void
TRoster::HandleRemovePreRegApp(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	status_t error = B_OK;
	// get the parameters
	uint32 token;
	if (request->FindInt32("token", (int32*)&token) != B_OK)
		SET_ERROR(error, B_BAD_VALUE);
	// remove the app
	if (error == B_OK) {
		RosterAppInfo* info = fEarlyPreRegisteredApps.InfoForToken(token);
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


/*!	\brief Handles a RemoveApp() request.
	\param request The request message
*/
void
TRoster::HandleRemoveApp(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	status_t error = B_OK;
	// get the parameters
	team_id team;
	if (request->FindInt32("team", &team) != B_OK)
		team = -1;

	PRINT("team: %ld\n", team);

	// remove the app
	if (error == B_OK) {
		if (RosterAppInfo* info = fRegisteredApps.InfoFor(team)) {
			RemoveApp(info);
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


/*!	\brief Handles a SetThreadAndTeam() request.
	\param request The request message
*/
void
TRoster::HandleSetThreadAndTeam(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

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

	PRINT("team: %ld, thread: %ld, token: %lu\n", team, thread, token);

	// update the app_info
	if (error == B_OK) {
		RosterAppInfo* info = fEarlyPreRegisteredApps.InfoForToken(token);
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
			if (error == B_OK)
				SET_ERROR(error, AddApp(info));
			// cleanup on failure
			if (error != B_OK) {
				if (info->port >= 0)
					delete_port(info->port);
				delete info;
				info = NULL;
			}
			// handle pending IsAppRegistered() requests
			IARRequestMap::iterator it = fIARRequestsByID.find(team);
			if (it != fIARRequestsByID.end()) {
				BMessageQueue* requests = it->second;
				if (error == B_OK)
					_ReplyToIARRequests(requests, info);
				delete requests;
				fIARRequestsByID.erase(it);
			}

			it = fIARRequestsByToken.find((int32)token);
			if (it != fIARRequestsByToken.end()) {
				BMessageQueue* requests = it->second;
				if (error == B_OK)
					_ReplyToIARRequests(requests, info);
				delete requests;
				fIARRequestsByToken.erase(it);
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


/*!	\brief Handles a SetSignature() request.
	\param request The request message
*/
void
TRoster::HandleSetSignature(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	status_t error = B_OK;
	// get the parameters
	team_id team;
	const char* signature;
	if (request->FindInt32("team", &team) != B_OK)
		error = B_BAD_VALUE;
	if (request->FindString("signature", &signature) != B_OK)
		error = B_BAD_VALUE;
	// find the app and set the signature
	if (error == B_OK) {
		if (RosterAppInfo* info = fRegisteredApps.InfoFor(team))
			strcpy(info->signature, signature);
		else
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


/*!	\brief Handles a Get{Running,Active,}AppInfo() request.
	\param request The request message
*/
void
TRoster::HandleGetAppInfo(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	status_t error = B_OK;
	// get the parameters
	team_id team;
	entry_ref ref;
	const char* signature;
	bool hasTeam = true;
	bool hasRef = true;
	bool hasSignature = true;
	if (request->FindInt32("team", &team) != B_OK)
		hasTeam = false;
	if (request->FindRef("ref", &ref) != B_OK)
		hasRef = false;
	if (request->FindString("signature", &signature) != B_OK)
		hasSignature = false;

if (hasTeam)
PRINT("team: %ld\n", team);
if (hasRef)
PRINT("ref: %ld, %lld, %s\n", ref.device, ref.directory, ref.name);
if (hasSignature)
PRINT("signature: %s\n", signature);

	// get the info
	RosterAppInfo* info = NULL;
	if (error == B_OK) {
		if (hasTeam) {
			info = fRegisteredApps.InfoFor(team);
			if (info == NULL)
				SET_ERROR(error, B_BAD_TEAM_ID);
		} else if (hasRef) {
			info = fRegisteredApps.InfoFor(&ref);
			if (info == NULL)
				SET_ERROR(error, B_ERROR);
		} else if (hasSignature) {
			info = fRegisteredApps.InfoFor(signature);
			if (info == NULL)
				SET_ERROR(error, B_ERROR);
		} else {
			// If neither of those has been supplied, the active application
			// info is requested.
			if (fActiveApp)
				info = fActiveApp;
			else
				SET_ERROR(error, B_ERROR);
		}
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


/*!	\brief Handles a GetAppList() request.
	\param request The request message
*/
void
TRoster::HandleGetAppList(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	status_t error = B_OK;
	// get the parameters
	const char* signature;
	if (request->FindString("signature", &signature) != B_OK)
		signature = NULL;
	// reply to the request
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		// get the list
		for (AppInfoList::Iterator it(fRegisteredApps.It());
			 RosterAppInfo* info = *it;
			 ++it) {
			if (info->state != APP_STATE_REGISTERED)
				continue;
			if (!signature || !strcasecmp(signature, info->signature))
				reply.AddInt32("teams", info->team);
		}
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}

	FUNCTION_END();
}


/*!	\brief Handles a _UpdateActiveApp() request.

	This is sent from the app_server when the current active application
	is changed.

	\param request The request message
*/
void
TRoster::HandleUpdateActiveApp(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	// get the parameters
	status_t error = B_OK;
	team_id team;
	if (request->FindInt32("team", &team) != B_OK)
		error = B_BAD_VALUE;

	// activate the app
	if (error == B_OK) {
		if (RosterAppInfo* info = fRegisteredApps.InfoFor(team))
			UpdateActiveApp(info);
		else
			error = B_BAD_TEAM_ID;
	}

	// reply to the request
	if (request->IsSourceWaiting()) {
		if (error == B_OK) {
			BMessage reply(B_REG_SUCCESS);
			request->SendReply(&reply);
		} else {
			BMessage reply(B_REG_ERROR);
			reply.AddInt32("error", error);
			request->SendReply(&reply);
		}
	}

	FUNCTION_END();
}


/*!	\brief Handles a Broadcast() request.
	\param request The request message
*/
void
TRoster::HandleBroadcast(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	status_t error = B_OK;
	// get the parameters
	team_id team;
	BMessage message;
	BMessenger replyTarget;
	if (request->FindInt32("team", &team) != B_OK)
		team = -1;
	if (error == B_OK && request->FindMessage("message", &message) != B_OK)
		error = B_BAD_VALUE;
	if (error == B_OK
		&& request->FindMessenger("reply_target", &replyTarget) != B_OK) {
		error = B_BAD_VALUE;
	}

	// reply to the request -- do this first, don't let the inquirer wait
	if (error == B_OK) {
		BMessage reply(B_REG_SUCCESS);
		request->SendReply(&reply);
	} else {
		BMessage reply(B_REG_ERROR);
		reply.AddInt32("error", error);
		request->SendReply(&reply);
	}

	// broadcast the message
	if (error == B_OK) {
		// the target set (excludes the registrar and the requesting team)
		class BroadcastMessagingTargetSet
			: public AppInfoListMessagingTargetSet {
			public:
				BroadcastMessagingTargetSet(AppInfoList& list, team_id team)
					: AppInfoListMessagingTargetSet(list, true),
					  fTeam(team)
				{
				}

				virtual bool Filter(const RosterAppInfo* info)
				{
					return AppInfoListMessagingTargetSet::Filter(info)
						&& (info->team != fTeam);
				}

			private:
				team_id	fTeam;
		} targetSet(fRegisteredApps, team);

		if (targetSet.HasNext()) {
			// set the reply target
			BMessage::Private(message).SetReply(replyTarget);

			// send the messages
			MessageDeliverer::Default()->DeliverMessage(&message, targetSet);
		}
	}

	FUNCTION_END();
}


/*!	\brief Handles a StartWatching() request.
	\param request The request message
*/
void
TRoster::HandleStartWatching(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	status_t error = B_OK;
	// get the parameters
	BMessenger target;
	uint32 events;
	if (error == B_OK && request->FindMessenger("target", &target) != B_OK)
		error = B_BAD_VALUE;
	if (request->FindInt32("events", (int32*)&events) != B_OK)
		error = B_BAD_VALUE;
	// add the new watcher
	if (error == B_OK) {
		Watcher* watcher = new(nothrow) EventMaskWatcher(target, events);
		if (watcher) {
			if (!fWatchingService.AddWatcher(watcher)) {
				error = B_NO_MEMORY;
				delete watcher;
			}
		} else
			error = B_NO_MEMORY;
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


/*!	\brief Handles a StopWatching() request.
	\param request The request message
*/
void
TRoster::HandleStopWatching(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	status_t error = B_OK;
	// get the parameters
	BMessenger target;
	if (error == B_OK && request->FindMessenger("target", &target) != B_OK)
		error = B_BAD_VALUE;
	// remove the watcher
	if (error == B_OK) {
		if (!fWatchingService.RemoveWatcher(target))
			error = B_BAD_VALUE;
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


/*!	\brief Handles a GetRecentDocuments() request.
	\param request The request message
*/
void
TRoster::HandleGetRecentDocuments(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	_HandleGetRecentEntries(request);

	FUNCTION_END();
}


/*!	\brief Handles a GetRecentFolders() request.
	\param request The request message
*/
void
TRoster::HandleGetRecentFolders(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	_HandleGetRecentEntries(request);

	FUNCTION_END();
}


/*!	\brief Handles a GetRecentApps() request.
	\param request The request message
*/
void
TRoster::HandleGetRecentApps(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	if (!request) {
		D(PRINT("WARNING: TRoster::HandleGetRecentApps(NULL) called\n"));
		return;
	}

	int32 maxCount;
	BMessage reply(B_REG_RESULT);

	status_t error = request->FindInt32("max count", &maxCount);
	if (!error)
		error = fRecentApps.Get(maxCount, &reply);
	reply.AddInt32("result", error);
	request->SendReply(&reply);

	FUNCTION_END();
}


/*!	\brief Handles an AddToRecentDocuments() request.
	\param request The request message
*/
void
TRoster::HandleAddToRecentDocuments(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	if (!request) {
		D(PRINT("WARNING: TRoster::HandleAddToRecentDocuments(NULL) called\n"));
		return;
	}

	entry_ref ref;
	const char* appSig;
	BMessage reply(B_REG_RESULT);

	status_t error = request->FindRef("ref", &ref);
	if (!error)
		error = request->FindString("app sig", &appSig);
	if (!error)
		error = fRecentDocuments.Add(&ref, appSig);
	reply.AddInt32("result", error);
	request->SendReply(&reply);

	FUNCTION_END();
}


/*!	\brief Handles an AddToRecentFolders() request.
	\param request The request message
*/
void
TRoster::HandleAddToRecentFolders(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	if (!request) {
		D(PRINT("WARNING: TRoster::HandleAddToRecentFolders(NULL) called\n"));
		return;
	}

	entry_ref ref;
	const char* appSig;
	BMessage reply(B_REG_RESULT);

	status_t error = request->FindRef("ref", &ref);
	if (!error)
		error = request->FindString("app sig", &appSig);
	if (!error)
		error = fRecentFolders.Add(&ref, appSig);
	reply.AddInt32("result", error);
	request->SendReply(&reply);

	FUNCTION_END();
}


/*!	\brief Handles an AddToRecentApps() request.
	\param request The request message
*/
void
TRoster::HandleAddToRecentApps(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	if (!request) {
		D(PRINT("WARNING: TRoster::HandleAddToRecentApps(NULL) called\n"));
		return;
	}

	const char* appSig;
	BMessage reply(B_REG_RESULT);

	status_t error = request->FindString("app sig", &appSig);
	if (!error)
		error = fRecentApps.Add(appSig);
	reply.AddInt32("result", error);
	request->SendReply(&reply);

	FUNCTION_END();
}


void
TRoster::HandleLoadRecentLists(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	if (!request) {
		D(PRINT("WARNING: TRoster::HandleLoadRecentLists(NULL) called\n"));
		return;
	}

	const char* filename;
	BMessage reply(B_REG_RESULT);

	status_t error = request->FindString("filename", &filename);
	if (!error)
		error = _LoadRosterSettings(filename);
	reply.AddInt32("result", error);
	request->SendReply(&reply);

	FUNCTION_END();
}


void
TRoster::HandleSaveRecentLists(BMessage* request)
{
	FUNCTION_START();

	BAutolock _(fLock);

	if (!request) {
		D(PRINT("WARNING: TRoster::HandleSaveRecentLists(NULL) called\n"));
		return;
	}

	const char* filename;
	BMessage reply(B_REG_RESULT);

	status_t error = request->FindString("filename", &filename);
	if (!error)
		error = _SaveRosterSettings(filename);
	reply.AddInt32("result", error);
	request->SendReply(&reply);

	FUNCTION_END();
}


void
TRoster::HandleRestartAppServer(BMessage* request)
{
	BAutolock _(fLock);

	// TODO: if an app_server is still running, stop it first

	const char* pathString;
	if (request->FindString("path", &pathString) != B_OK)
		pathString = "/boot/system/servers";
	BPath path(pathString);
	path.Append("app_server");
	// NOTE: its required at some point that the binary name is "app_server"

	const char **argv = new const char * [2];
	argv[0] = strdup(path.Path());
	argv[1] = NULL;

	thread_id threadId = load_image(1, argv, (const char**)environ);
	int i;
	for (i = 0; i < 1; i++)
		delete argv[i];
	delete [] argv;

	resume_thread(threadId);
	// give the server some time to create the server port
	snooze(100000);

	// notify all apps
	// TODO: whats about ourself?
	AppInfoListMessagingTargetSet targetSet(fRegisteredApps);
	if (targetSet.HasNext()) {
		// send the messages
		BMessage message(kMsgAppServerRestarted);
		MessageDeliverer::Default()->DeliverMessage(&message, targetSet);
	}
}


/*!	\brief Clears the current list of recent documents
*/
void
TRoster::ClearRecentDocuments()
{
	BAutolock _(fLock);

	fRecentDocuments.Clear();
}


/*!	\brief Clears the current list of recent folders
*/
void
TRoster::ClearRecentFolders()
{
	BAutolock _(fLock);

	fRecentFolders.Clear();
}


/*!	\brief Clears the current list of recent apps
*/
void
TRoster::ClearRecentApps()
{
	BAutolock _(fLock);

	fRecentApps.Clear();
}


/*!	\brief Initializes the roster.

	Currently only adds the registrar to the roster.
	The application must already be running, more precisly Run() must have
	been called.

	\return
	- \c B_OK: Everything went fine.
	- an error code
*/
status_t
TRoster::Init()
{
	// check lock initialization
	if (fLock.Sem() < 0)
		return fLock.Sem();

	// create the info
	RosterAppInfo* info = new(nothrow) RosterAppInfo;
	if (info == NULL)
		return B_NO_MEMORY;

	// get the app's ref
	entry_ref ref;
	status_t error = get_app_ref(&ref);

	// init and add the info
	if (error == B_OK) {
		info->Init(be_app->Thread(), be_app->Team(),
			BMessenger::Private(be_app_messenger).Port(),
			B_EXCLUSIVE_LAUNCH | B_BACKGROUND_APP, &ref, kRegistrarSignature);
		info->state = APP_STATE_REGISTERED;
		info->registration_time = system_time();
		error = AddApp(info);
	}

	if (error == B_OK)
		_LoadRosterSettings();

	// cleanup on error
	if (error != B_OK)
		delete info;

	return error;
}


/*!	\brief Add the supplied app info to the list of (pre-)registered apps.

	\param info The app info to be added
*/
status_t
TRoster::AddApp(RosterAppInfo* info)
{
	BAutolock _(fLock);

	status_t error = (info ? B_OK : B_BAD_VALUE);
	if (info) {
		if (!fRegisteredApps.AddInfo(info))
			error = B_NO_MEMORY;
	}
	return error;
}


/*!	\brief Removes the supplied app info from the list of (pre-)registered
	apps.

	\param info The app info to be removed
*/
void
TRoster::RemoveApp(RosterAppInfo* info)
{
	BAutolock _(fLock);

	if (info) {
		if (fRegisteredApps.RemoveInfo(info)) {
			if (info->state == APP_STATE_REGISTERED) {
				info->state = APP_STATE_UNREGISTERED;
				_AppRemoved(info);
			}
		}
	}
}


/*!	\brief Activates the application identified by \a info.

	The currently active application is deactivated and the one whose
	info is supplied is activated. \a info may be \c NULL, which only
	deactivates the currently active application.

	\param info The info of the app to be activated
*/
void
TRoster::UpdateActiveApp(RosterAppInfo* info)
{
	BAutolock _(fLock);

	if (info != fActiveApp) {
		// deactivate the currently active app
		RosterAppInfo* oldActiveApp = fActiveApp;
		fActiveApp = NULL;
		if (oldActiveApp)
			_AppDeactivated(oldActiveApp);

		// activate the new app
		if (info) {
			fActiveApp = info;
			_AppActivated(info);
		}
	}
}


/*!	\brief Checks whether the (pre-)registered applications are still running.

	This is necessary, since killed applications don't unregister properly.
*/
void
TRoster::CheckSanity()
{
	BAutolock _(fLock);

	// not early (pre-)registered applications
	AppInfoList obsoleteApps;
	for (AppInfoList::Iterator it = fRegisteredApps.It(); it.IsValid(); ++it) {
		if (!(*it)->IsRunning())
			obsoleteApps.AddInfo(*it);
	}

	// remove the apps
	for (AppInfoList::Iterator it = obsoleteApps.It(); it.IsValid(); ++it) {
		RemoveApp(*it);
		delete *it;
	}
	obsoleteApps.MakeEmpty(false);
		// don't delete infos a second time

	// early pre-registered applications
	bigtime_t timeLimit = system_time() - kMaximalEarlyPreRegistrationPeriod;
	for (AppInfoList::Iterator it = fEarlyPreRegisteredApps.It();
		 it.IsValid();
		 ++it) {
		if ((*it)->registration_time < timeLimit)
			obsoleteApps.AddInfo(*it);
	}

	// remove the apps
	for (AppInfoList::Iterator it = obsoleteApps.It(); it.IsValid(); ++it) {
		fEarlyPreRegisteredApps.RemoveInfo(*it);
		delete *it;
	}
	obsoleteApps.MakeEmpty(false);
		// don't delete infos a second time
}


/*!	\brief Tells the roster whether a shutdown process is in progess at the
		   moment.

	After this method is called with \a shuttingDown == \c true, no more
	applications can be created.

	\param shuttingDown \c true, to indicate the start of the shutdown process,
		   \c false to signalling its end.
*/
void
TRoster::SetShuttingDown(bool shuttingDown)
{
	BAutolock _(fLock);

	fShuttingDown = shuttingDown;

	if (shuttingDown)
		_SaveRosterSettings();
}


/*!	\brief Returns lists of applications to be asked to quit on shutdown.

	\param userApps List of RosterAppInfos identifying the user applications.
		   Those will be ask to quit first.
	\param systemApps List of RosterAppInfos identifying the system applications
		   (like Tracker and Deskbar), which will be asked to quit after the
		   user applications are gone.
	\param vitalSystemApps A set of team_ids identifying teams that must not
		   be terminated (app server and registrar).
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
TRoster::GetShutdownApps(AppInfoList& userApps, AppInfoList& systemApps,
	AppInfoList& backgroundApps, hash_set<team_id>& vitalSystemApps)
{
	BAutolock _(fLock);

	status_t error = B_OK;

	// get the vital system apps:
	// * ourself
	// * kernel team
	// * app server
	// * debug server

	// ourself
	vitalSystemApps.insert(be_app->Team());

	// kernel team
	team_info teamInfo;
	if (get_team_info(B_SYSTEM_TEAM, &teamInfo) == B_OK)
		vitalSystemApps.insert(teamInfo.team);

	// app server
	port_id appServerPort = find_port(SERVER_PORT_NAME);
	port_info portInfo;
	if (appServerPort >= 0
		&& get_port_info(appServerPort, &portInfo) == B_OK) {
		vitalSystemApps.insert(portInfo.team);
	}

	// debug server
	RosterAppInfo* info =
		fRegisteredApps.InfoFor("application/x-vnd.haiku-debug_server");
	if (info)
		vitalSystemApps.insert(info->team);

	// populate the other groups
	for (AppInfoList::Iterator it(fRegisteredApps.It());
		 RosterAppInfo* info = *it;
		 ++it) {
		if (vitalSystemApps.find(info->team) == vitalSystemApps.end()) {
			RosterAppInfo* clonedInfo = info->Clone();
			if (clonedInfo) {
				if (_IsSystemApp(info)) {
					if (!systemApps.AddInfo(clonedInfo))
						error = B_NO_MEMORY;
				} else if (info->flags & B_BACKGROUND_APP) {
					if (!backgroundApps.AddInfo(clonedInfo))
						error = B_NO_MEMORY;
				} else {
					if (!userApps.AddInfo(clonedInfo))
						error = B_NO_MEMORY;
				}

				if (error != B_OK)
					delete clonedInfo;
			} else
				error = B_NO_MEMORY;
		}

		if (error != B_OK)
			break;
	}

	// Special case, we add the input server to vital apps here so it is
	// not excluded in the lists above
	info = fRegisteredApps.InfoFor("application/x-vnd.Be-input_server");
 	if (info)
 		vitalSystemApps.insert(info->team);

	// clean up on error
	if (error != B_OK) {
		userApps.MakeEmpty(true);
		systemApps.MakeEmpty(true);
	}

	return error;
}


status_t
TRoster::AddWatcher(Watcher* watcher)
{
	BAutolock _(fLock);

	if (!watcher)
		return B_BAD_VALUE;

	if (!fWatchingService.AddWatcher(watcher))
		return B_NO_MEMORY;

	return B_OK;
}


void
TRoster::RemoveWatcher(Watcher* watcher)
{
	BAutolock _(fLock);

	if (watcher)
		fWatchingService.RemoveWatcher(watcher, false);
}


/*!	\brief Hook method invoked, when an application has been fully registered.
	\param info The RosterAppInfo of the added application.
*/
void
TRoster::_AppAdded(RosterAppInfo* info)
{
	// notify the watchers
	BMessage message(B_SOME_APP_LAUNCHED);
	_AddMessageWatchingInfo(&message, info);
	EventMaskWatcherFilter filter(B_REQUEST_LAUNCHED);
	fWatchingService.NotifyWatchers(&message, &filter);
}


/*!	\brief Hook method invoked, when a fully registered application has been
		removed.
	\param info The RosterAppInfo of the removed application.
*/
void
TRoster::_AppRemoved(RosterAppInfo* info)
{
	if (info) {
		// deactivate the app, if it was the active one
		if (info == fActiveApp)
			UpdateActiveApp(NULL);

		// notify the watchers
		BMessage message(B_SOME_APP_QUIT);
		_AddMessageWatchingInfo(&message, info);
		EventMaskWatcherFilter filter(B_REQUEST_QUIT);
		fWatchingService.NotifyWatchers(&message, &filter);
	}
}


/*!	\brief Hook method invoked, when an application has been activated.
	\param info The RosterAppInfo of the activated application.
*/
void
TRoster::_AppActivated(RosterAppInfo* info)
{
	if (info != NULL && info->state == APP_STATE_REGISTERED) {
		// send B_APP_ACTIVATED to the app
		BMessenger messenger;
		BMessenger::Private messengerPrivate(messenger);
		messengerPrivate.SetTo(info->team, info->port, B_NULL_TOKEN);
		BMessage message(B_APP_ACTIVATED);
		message.AddBool("active", true);
		// not sure, if it makes sense to use the MessageDeliverer here
		MessageDeliverer::Default()->DeliverMessage(&message, messenger);

		// notify the watchers
		BMessage watcherMessage(B_SOME_APP_ACTIVATED);
		_AddMessageWatchingInfo(&watcherMessage, info);
		EventMaskWatcherFilter filter(B_REQUEST_ACTIVATED);
		fWatchingService.NotifyWatchers(&watcherMessage, &filter);
	}
}


/*!	\brief Hook method invoked, when an application has been deactivated.
	\param info The RosterAppInfo of the deactivated application.
*/
void
TRoster::_AppDeactivated(RosterAppInfo* info)
{
	if (info != NULL && info->state == APP_STATE_REGISTERED) {
		// send B_APP_ACTIVATED to the app
		BMessenger messenger;
		BMessenger::Private messengerPrivate(messenger);
		messengerPrivate.SetTo(info->team, info->port, B_NULL_TOKEN);
		BMessage message(B_APP_ACTIVATED);
		message.AddBool("active", false);
		// not sure, if it makes sense to use the MessageDeliverer here
		MessageDeliverer::Default()->DeliverMessage(&message, messenger);
	}
}


/*!	\brief Adds an app_info to a message.

	The info is added as a flat_app_info to a field "app_info" with the type
	\c B_REG_APP_INFO_TYPE.

	\param message The message
	\param info The app_info.
	\return \c B_OK if everything went fine, an error code otherwise.
*/
status_t
TRoster::_AddMessageAppInfo(BMessage* message, const app_info* info)
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


/*!	\brief Adds application monitoring related fields to a message.
	\param message The message.
	\param info The app_info of the concerned application.
	\return \c B_OK if everything went fine, an error code otherwise.
*/
status_t
TRoster::_AddMessageWatchingInfo(BMessage* message, const app_info* info)
{
	status_t error = B_OK;
	if (error == B_OK)
		error = message->AddString("be:signature", info->signature);
	if (error == B_OK)
		error = message->AddInt32("be:team", info->team);
	if (error == B_OK)
		error = message->AddInt32("be:thread", info->thread);
	if (error == B_OK)
		error = message->AddInt32("be:flags", (int32)info->flags);
	if (error == B_OK)
		error = message->AddRef("be:ref", &info->ref);
	return error;
}


/*!	\brief Returns the next available token.
	\return The token.
*/
uint32
TRoster::_NextToken()
{
	return ++fLastToken;
}


/*!	\brief Adds an IsAppRegistered() request to the given map.

	If something goes wrong, the method deletes the request.

	\param map The map the request shall be added to.
	\param key The key under which to add the request.
	\param request The request message to be added.
*/
void
TRoster::_AddIARRequest(IARRequestMap& map, int32 key, BMessage* request)
{
	IARRequestMap::iterator it = map.find(key);
	BMessageQueue* requests = NULL;
	if (it == map.end()) {
		requests = new(nothrow) BMessageQueue();
		if (!requests) {
			delete request;
			return;
		}

		map[key] = requests;
	} else
		requests = it->second;

	requests->AddMessage(request);
}


/*!	\brief Invokes _ReplyToIARRequest() for all messages in the given
		   message queue.

	\param requests The request messages to be replied to
	\param info The RosterAppInfo of the application in question
		   (may be \c NULL)
*/
void
TRoster::_ReplyToIARRequests(BMessageQueue* requests, const RosterAppInfo* info)
{
	while (BMessage* request = requests->NextMessage()) {
		_ReplyToIARRequest(request, info);
		delete request;
	}
}


/*!	\brief Sends a reply message to an IsAppRegistered() request.

	The message to be sent is a simple \c B_REG_SUCCESS message containing
	a "pre-registered" field, that says whether or not the application is
	pre-registered. It will be set to \c false, unless an \a info is supplied
	and the application this info refers to is pre-registered.

	\param request The request message to be replied to
	\param info The RosterAppInfo of the application in question
		   (may be \c NULL)
*/
void
TRoster::_ReplyToIARRequest(BMessage* request, const RosterAppInfo* info)
{
	// pre-registered or registered?
	bool preRegistered = false;
	if (info) {
		switch (info->state) {
			case APP_STATE_PRE_REGISTERED:
				preRegistered = true;
				break;
			case APP_STATE_UNREGISTERED:
			case APP_STATE_REGISTERED:
				preRegistered = false;
				break;
		}
	}
	// send reply
	BMessage reply(B_REG_SUCCESS);
	reply.AddBool("registered", (bool)info);
	reply.AddBool("pre-registered", preRegistered);
	PRINT("_ReplyToIARRequest(): pre-registered: %d\n", preRegistered);
	if (info)
		_AddMessageAppInfo(&reply, info);
	request->SendReply(&reply);
}


/*! \brief Handles requests for both GetRecentDocuments() and
	GetRecentFolders().
*/
void
TRoster::_HandleGetRecentEntries(BMessage* request)
{
	FUNCTION_START();
	if (!request) {
		D(PRINT("WARNING: TRoster::HandleGetRecentFolders(NULL) called\n"));
		return;
	}

	int32 maxCount;
	BMessage reply(B_REG_RESULT);
	char** fileTypes = NULL;
	int32 fileTypesCount = 0;
	char* appSig = NULL;

	status_t error = request->FindInt32("max count", &maxCount);
	// Look for optional file type(s)
	if (!error) {
		type_code typeFound;
		status_t typeError = request->GetInfo("file type", &typeFound,
			&fileTypesCount);
		if (!typeError)
			typeError = typeFound == B_STRING_TYPE ? B_OK : B_BAD_TYPE;
		if (!typeError) {
			fileTypes = new(nothrow) char*[fileTypesCount];
			typeError = fileTypes ? B_OK : B_NO_MEMORY;
		}
		if (!typeError) {
			for (int i = 0; !error && i < fileTypesCount; i++) {
				const char* type;
				if (request->FindString("file type", i, &type) == B_OK) {
					fileTypes[i] = new(nothrow) char[B_MIME_TYPE_LENGTH];
					error = fileTypes[i] ? B_OK : B_NO_MEMORY;
						// Yes, I do mean to use "error" here, not "typeError"
					BPrivate::Storage::to_lower(type, fileTypes[i]);
						// Types are expected to be lowercase
				}
			}
		}
	}
	// Look for optional app sig
	if (!error) {
		const char* sig;
		error = request->FindString("app sig", &sig);
		if (!error) {
			appSig = new(nothrow) char[B_MIME_TYPE_LENGTH];
			error = appSig ? B_OK : B_NO_MEMORY;
			BPrivate::Storage::to_lower(sig, appSig);
		} else if (error == B_NAME_NOT_FOUND)
			error = B_OK;
	}
	if (!error) {
		switch (request->what) {
			case B_REG_GET_RECENT_DOCUMENTS:
				error = fRecentDocuments.Get(maxCount, (const char**)fileTypes,
					fileTypesCount, appSig, &reply);
				D(fRecentDocuments.Print());
		   	    break;

			case B_REG_GET_RECENT_FOLDERS:
				error = fRecentFolders.Get(maxCount, (const char**)fileTypes,
					fileTypesCount, appSig, &reply);
				D(fRecentFolders.Print());
			    break;

			default:
				D(PRINT("WARNING: TRoster::_HandleGetRecentEntries(): "
					"unexpected request->what value of 0x%lx\n",
					request->what));
				error = B_BAD_VALUE;
				break;
		}
	}
	reply.AddInt32("result", error);
	// Clean up before sending a reply
	delete [] appSig;
	if (fileTypes) {
		for (int i = 0; i < fileTypesCount; i++)
			delete [] fileTypes[i];
		delete[] fileTypes;
		fileTypes = NULL;
	}
	request->SendReply(&reply);

	FUNCTION_END();
}


/*!
	\brief Checks all registered apps for \a ref and \a signature if
		they are still alive, and removes those that aren't.
*/
void
TRoster::_ValidateRunning(const entry_ref& ref, const char* signature)
{
	while (true) {
		// get info via ref or signature
		RosterAppInfo* info = fRegisteredApps.InfoFor(&ref);
		if (info == NULL && signature != NULL)
			info = fRegisteredApps.InfoFor(signature);

		// if app is alive or does not exist, we can exit
		if (info == NULL || info->IsRunning())
			return;

		RemoveApp(info);
		delete info;
	}
}


bool
TRoster::_IsSystemApp(RosterAppInfo* info) const
{
	BPath path;
	if (path.SetTo(&info->ref) != B_OK || path.GetParent(&path) != B_OK)
		return false;

	return !strcmp(path.Path(), fSystemAppPath.Path())
		|| !strcmp(path.Path(), fSystemServerPath.Path());
}


status_t
TRoster::_LoadRosterSettings(const char* path)
{
	BPath _path;
	const char* settingsPath
		= path ? path : get_default_roster_settings_path(_path, false);

	RosterSettingsCharStream stream;
	status_t error;
	BFile file;

	error = file.SetTo(settingsPath, B_READ_ONLY);
	off_t size;
	if (!error)
		error = file.GetSize(&size);

	char* data = NULL;

	if (!error) {
		data = new(nothrow) char[size + 1];
		error = data ? B_OK : B_NO_MEMORY;
	}
	if (!error) {
		ssize_t bytes = file.Read(data, size);
		error = bytes < 0 ? bytes : (bytes == size ? B_OK : B_FILE_ERROR);
	}
	if (!error) {
		data[size] = 0;
		error = stream.SetTo(std::string(data));
	}

	delete[] data;

	if (!error) {
		// Clear the current lists as
		// we'll be manually building them up
		fRecentDocuments.Clear();
		fRecentFolders.Clear();
		fRecentApps.Clear();

		// Now we just walk through the file and read in the info
		while (true) {
			status_t streamError;
			char str[B_PATH_NAME_LENGTH];

			// (RecentDoc | RecentFolder | RecentApp)
			streamError = stream.GetString(str);
			if (!streamError) {
				enum EntryType {
					etDoc,
					etFolder,
					etApp,
					etSomethingIsAmiss,
				} type;

				if (strcmp(str, "RecentDoc") == 0)
					type = etDoc;
				else if (strcmp(str, "RecentFolder") == 0)
					type = etFolder;
				else if (strcmp(str, "RecentApp") == 0)
					type = etApp;
				else
					type = etSomethingIsAmiss;

				switch (type) {
					case etDoc:
					case etFolder:
					{
						// For curing laziness
						std::list<recent_entry*>* list = type == etDoc
							? &fRecentDocuments.fEntryList
							: &fRecentFolders.fEntryList;

						char path[B_PATH_NAME_LENGTH];
						char app[B_PATH_NAME_LENGTH];
						char rank[B_PATH_NAME_LENGTH];
						entry_ref ref;
						ulong index = 0;

						// Convert the given path to an entry ref
						streamError = stream.GetString(path);
						if (!streamError)
							streamError = get_ref_for_path(path, &ref);

						// Add a new entry to the list for each application
						// signature and rank we find
						while (!streamError) {
							if (!streamError)
								streamError = stream.GetString(app);
							if (!streamError) {
								BPrivate::Storage::to_lower(app);
								streamError = stream.GetString(rank);
							}
							if (!streamError) {
								index = strtoul(rank, NULL, 10);
								if (index == ULONG_MAX)
									streamError = errno;
							}
							recent_entry* entry = NULL;
							if (!streamError) {
								entry = new(nothrow) recent_entry(&ref, app,
									index);
								streamError = entry ? B_OK : B_NO_MEMORY;
							}
							if (!streamError) {
								D(printf("pushing entry, leaf == '%s', app == "
									"'%s', index == %ld\n", entry->ref.name,
									entry->sig.c_str(), entry->index));

								list->push_back(entry);
							}
						}

						if (streamError) {
							D(printf("entry error 0x%lx\n", streamError));
							if (streamError
									!= RosterSettingsCharStream::kEndOfLine
							    && streamError
							    	!= RosterSettingsCharStream::kEndOfStream)
							stream.SkipLine();
						}

						break;
					}


					case etApp:
					{
						char app[B_PATH_NAME_LENGTH];
						streamError = stream.GetString(app);
						if (!streamError) {
							BPrivate::Storage::to_lower(app);
							fRecentApps.fAppList.push_back(app);
						} else
							stream.SkipLine();
						break;
					}

					default:
						// Something was amiss; skip to the next line
						stream.SkipLine();
						break;
				}

			}

			if (streamError == RosterSettingsCharStream::kEndOfStream)
				break;
		}

		// Now we must sort our lists of documents and folders by the
		// indicies we read for each entry (largest index first)
		fRecentDocuments.fEntryList.sort(larger_index);
		fRecentFolders.fEntryList.sort(larger_index);

		D(
			printf("----------------------------------------------------------------------\n");
			fRecentDocuments.Print();
			printf("----------------------------------------------------------------------\n");
			fRecentFolders.Print();
			printf("----------------------------------------------------------------------\n");
			fRecentApps.Print();
			printf("----------------------------------------------------------------------\n");
		);
	}
	if (error) {
		D(PRINT("WARNING: TRoster::_LoadRosterSettings(): error loading roster "
			"settings from '%s', 0x%lx\n", settingsPath, error));
	}
	return error;
}


status_t
TRoster::_SaveRosterSettings(const char* path)
{
	BPath _path;
	const char* settingsPath
		= path != NULL ? path : get_default_roster_settings_path(_path, true);

	status_t error;
	FILE* file;

	file = fopen(settingsPath, "w+");
	error = file ? B_OK : errno;
	if (!error) {
		status_t saveError;
		saveError = fRecentDocuments.Save(file, "Recent documents", "RecentDoc");
		if (saveError) {
			D(PRINT("TRoster::_SaveRosterSettings(): recent documents save "
				"failed with error 0x%lx\n", saveError));
		}
		saveError = fRecentFolders.Save(file, "Recent folders", "RecentFolder");
		if (saveError) {
			D(PRINT("TRoster::_SaveRosterSettings(): recent folders save "
				"failed with error 0x%lx\n", saveError));
		}
		saveError = fRecentApps.Save(file);
		if (saveError) {
			D(PRINT("TRoster::_SaveRosterSettings(): recent folders save "
				"failed with error 0x%lx\n", saveError));
		}
		fclose(file);
	}

	return error;
}
