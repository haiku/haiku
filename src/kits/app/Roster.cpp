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
//	File Name:		Roster.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	BRoster class lets you launch apps and keeps
//					track of apps that are running. 
//					Global be_roster represents the default BRoster.
//					app_info structure provides info for a running app.
//------------------------------------------------------------------------------
#include <new.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <List.h>
#include <OS.h>
#include <RegistrarDefs.h>
#include <Roster.h>

// debugging
//#define DBG(x) x
#define DBG(x)
#define OUT	printf

enum {
	NOT_IMPLEMENTED	= B_ERROR,
};

// _init_roster_
int
_init_roster_()
{
	be_roster = new BRoster;
	return 0;
}

// _delete_roster_
int
_delete_roster_()
{
	delete be_roster;
	return 0;
}

// find_message_app_info
static
status_t
find_message_app_info(BMessage *message, app_info *info)
{
	status_t error = B_OK;
	const flat_app_info *flatInfo = NULL;
	ssize_t size = 0;
	error = message->FindData("app_info", B_REG_APP_INFO_TYPE,
							  (const void**)&flatInfo, &size);
	if (error == B_OK) {
		if (size == sizeof(flat_app_info)) {
			memcpy(info, &flatInfo->info, sizeof(app_info));
			info->ref.name = NULL;
			if (strlen(flatInfo->ref_name) > 0)
				info->ref.set_name(flatInfo->ref_name);
		} else
			error = B_ERROR;
	}
	return error;
 }


/*-------------------------------------------------------------*/
/* --------- app_info Struct and Values ------------------------ */

// constructor
app_info::app_info()
		: thread(-1),
		  team(-1),
		  port(-1),
		  flags(B_REG_DEFAULT_APP_FLAGS),
		  ref()
{
	signature[0] = '\0';
}

// destructor
app_info::~app_info()
{
}


/*-------------------------------------------------------------*/
/* --------- BRoster class----------------------------------- */

// constructor
BRoster::BRoster()
	   : fMess(),
		 fMimeMess()
{
	InitMessengers();
}

// destructor
BRoster::~BRoster()
{
}


/* Querying for apps */

// IsRunning
bool
BRoster::IsRunning(const char *mimeSig) const
{
	return (TeamFor(mimeSig) >= 0);
}

// IsRunning
bool
BRoster::IsRunning(entry_ref *ref) const
{
	return (TeamFor(ref) >= 0);
}

// TeamFor
team_id
BRoster::TeamFor(const char *mimeSig) const
{
	team_id team;
	app_info info;
	status_t error = GetAppInfo(mimeSig, &info);
	if (error == B_OK)
		team = info.team;
	else
		team = error;
	return team;
}

// TeamFor
team_id
BRoster::TeamFor(entry_ref *ref) const
{
	team_id team;
	app_info info;
	status_t error = GetAppInfo(ref, &info);
	if (error == B_OK)
		team = info.team;
	else
		team = error;
	return team;
}

// GetAppList
void
BRoster::GetAppList(BList *teamIDList) const
{
	status_t error = (teamIDList ? B_OK : B_BAD_VALUE);
	// compose the request message
	BMessage request(B_REG_GET_APP_LIST);
	// send the request
	BMessage reply;
	if (error == B_OK)
		error = fMess.SendMessage(&request, &reply);
	// evaluate the reply
	if (error == B_OK) {
		if (reply.what == B_REG_SUCCESS) {
			team_id team;
			for (int32 i = 0; reply.FindInt32("teams", i, &team) == B_OK; i++)
				teamIDList->AddItem((void*)team);
		} else
			reply.FindInt32("error", &error);
	}
}

// GetAppList
//
// The list is not emptied.
void
BRoster::GetAppList(const char *sig, BList *teamIDList) const
{
	status_t error = (sig && teamIDList ? B_OK : B_BAD_VALUE);
	// compose the request message
	BMessage request(B_REG_GET_APP_LIST);
	if (error == B_OK)
		error = request.AddString("signature", sig);
	// send the request
	BMessage reply;
	if (error == B_OK)
		error = fMess.SendMessage(&request, &reply);
	// evaluate the reply
	if (error == B_OK) {
		if (reply.what == B_REG_SUCCESS) {
			team_id team;
			for (int32 i = 0; reply.FindInt32("teams", i, &team) == B_OK; i++)
				teamIDList->AddItem((void*)team);
		} else
			reply.FindInt32("error", &error);
	}
}

// GetAppInfo
status_t
BRoster::GetAppInfo(const char *sig, app_info *info) const
{
	status_t error = (sig && info ? B_OK : B_BAD_VALUE);
	// compose the request message
	BMessage request(B_REG_GET_APP_INFO);
	if (error == B_OK)
		error = request.AddString("signature", sig);
	// send the request
	BMessage reply;
	if (error == B_OK)
		error = fMess.SendMessage(&request, &reply);
	// evaluate the reply
	if (error == B_OK) {
		if (reply.what == B_REG_SUCCESS)
			error = find_message_app_info(&reply, info);
		else
			reply.FindInt32("error", &error);
	}
	return error;
}

// GetAppInfo
status_t
BRoster::GetAppInfo(entry_ref *ref, app_info *info) const
{
	status_t error = (ref && info ? B_OK : B_BAD_VALUE);
	// compose the request message
	BMessage request(B_REG_GET_APP_INFO);
	if (error == B_OK)
		error = request.AddRef("ref", ref);
	// send the request
	BMessage reply;
	if (error == B_OK)
		error = fMess.SendMessage(&request, &reply);
	// evaluate the reply
	if (error == B_OK) {
		if (reply.what == B_REG_SUCCESS)
			error = find_message_app_info(&reply, info);
		else
			reply.FindInt32("error", &error);
	}
	return error;
}

// GetRunningAppInfo
status_t
BRoster::GetRunningAppInfo(team_id team, app_info *info) const
{
	status_t error = (team >= 0 && info ? B_OK : B_BAD_VALUE);
	// compose the request message
	BMessage request(B_REG_GET_APP_INFO);
	if (error == B_OK)
		error = request.AddInt32("team", team);
	// send the request
	BMessage reply;
	if (error == B_OK)
		error = fMess.SendMessage(&request, &reply);
	// evaluate the reply
	if (error == B_OK) {
		if (reply.what == B_REG_SUCCESS)
			error = find_message_app_info(&reply, info);
		else
			reply.FindInt32("error", &error);
	}
	return error;
}

// GetActiveAppInfo
status_t
BRoster::GetActiveAppInfo(app_info *info) const
{
	status_t error = (info ? B_OK : B_BAD_VALUE);
	// compose the request message
	BMessage request(B_REG_GET_APP_INFO);
	// send the request
	BMessage reply;
	if (error == B_OK)
		error = fMess.SendMessage(&request, &reply);
	// evaluate the reply
	if (error == B_OK) {
		if (reply.what == B_REG_SUCCESS)
			error = find_message_app_info(&reply, info);
		else
			reply.FindInt32("error", &error);
	}
	return error;
}

// FindApp
status_t
BRoster::FindApp(const char *mimeType, entry_ref *app) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// FindApp
status_t
BRoster::FindApp(entry_ref *ref, entry_ref *app) const
{
	return NOT_IMPLEMENTED; // not implemented
}


/* Launching, activating, and broadcasting to apps */
// Broadcast
status_t
BRoster::Broadcast(BMessage *msg) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// Broadcast
status_t
BRoster::Broadcast(BMessage *msg, BMessenger replyTo) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// StartWatching
status_t
BRoster::StartWatching(BMessenger target, uint32 eventMask) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// StopWatching
status_t
BRoster::StopWatching(BMessenger target) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// ActivateApp
status_t
BRoster::ActivateApp(team_id team) const
{
	status_t error = (team >= 0 ? B_OK : B_BAD_TEAM_ID);
	// compose the request message
	BMessage request(B_REG_ACTIVATE_APP);
	if (error == B_OK)
		error = request.AddInt32("team", team);
	// send the request
	BMessage reply;
	if (error == B_OK)
		error = fMess.SendMessage(&request, &reply);
	// evaluate the reply
	if (error == B_OK && reply.what != B_REG_SUCCESS)
		reply.FindInt32("error", &error);
	return error;
}

// Launch
status_t
BRoster::Launch(const char *mimeType, BMessage *initialMsgs,
				team_id *appTeam) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// Launch
status_t
BRoster::Launch(const char *mimeType, BList *messageList,
				team_id *appTeam) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// Launch
status_t
BRoster::Launch(const char *mimeType, int argc, char **args,
				team_id *appTeam) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// Launch
status_t
BRoster::Launch(const entry_ref *ref, const BMessage *initialMessage,
				team_id *app_team) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// Launch
status_t
BRoster::Launch(const entry_ref *ref, const BList *messageList,
				team_id *appTeam) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// Launch
status_t
BRoster::Launch(const entry_ref *ref, int argc, const char * const *args,
				team_id *appTeam) const
{
	return NOT_IMPLEMENTED; // not implemented
}


/* Recent document and app support */

// GetRecentDocuments
void
BRoster::GetRecentDocuments(BMessage *refList, int32 maxCount,
							const char *ofType,
							const char *openedByAppSig) const
{
}

// GetRecentDocuments
void
BRoster::GetRecentDocuments(BMessage *refList, int32 maxCount,
							const char *ofTypeList[], int32 ofTypeListCount,
							const char *openedByAppSig) const
{
}

// GetRecentFolders
void
BRoster::GetRecentFolders(BMessage *refList, int32 maxCount,
						  const char *openedByAppSig) const
{
}

// GetRecentApps
void
BRoster::GetRecentApps(BMessage *refList, int32 maxCount) const
{
}

// AddToRecentDocuments
void
BRoster::AddToRecentDocuments(const entry_ref *doc, const char *appSig) const
{
}

// AddToRecentFolders
void
BRoster::AddToRecentFolders(const entry_ref *folder, const char *appSig) const
{
}


/*----- Private or reserved ------------------------------*/

// _StartWatching
status_t
BRoster::_StartWatching(mtarget target, BMessenger *rosterMess, uint32 what,
						BMessenger notify, uint32 event_mask) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// _StopWatching
status_t
BRoster::_StopWatching(mtarget target, BMessenger *rosterMess, uint32 what,
					   BMessenger notify) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// AddApplication
status_t
BRoster::AddApplication(const char *mimeSig, const entry_ref *ref,
						uint32 flags, team_id team, thread_id thread,
						port_id port, bool fullReg, uint32 *_token,
						team_id *otherTeam) const
{
	status_t error = B_OK;
	// compose the request message
	BMessage request(B_REG_ADD_APP);
	if (error == B_OK && mimeSig)
		error = request.AddString("signature", mimeSig);
	if (error == B_OK && ref)
		error = request.AddRef("ref", ref);
	if (error == B_OK)
		error = request.AddInt32("flags", (int32)flags);
	if (error == B_OK && team >= 0)
		error = request.AddInt32("team", team);
	if (error == B_OK && thread >= 0)
		error = request.AddInt32("thread", thread);
	if (error == B_OK && port >= 0)
		error = request.AddInt32("port", port);
	if (error == B_OK)
		error = request.AddBool("full_registration", fullReg);
	// send the request
	BMessage reply;
	if (error == B_OK)
		error = fMess.SendMessage(&request, &reply);
	// evaluate the reply
	if (error == B_OK) {
		if (reply.what == B_REG_SUCCESS) {
			if (!fullReg && team < 0) {
				uint32 token;
				if (request.FindInt32("token", (int32)&token) == B_OK)
					*_token = token;
				else
					error = B_ERROR;
			}
		} else {
			reply.FindInt32("error", &error);
			if (otherTeam)
				reply.FindInt32("other_team", otherTeam);
		}
	}
	return error;
}

// SetSignature
void
BRoster::SetSignature(team_id team, const char *mimeSig) const
{
}

// SetThread
void
BRoster::SetThread(team_id team, thread_id thread) const
{
}

// SetThreadAndTeam
status_t
BRoster::SetThreadAndTeam(uint32 entryToken, thread_id thread,
						  team_id team) const
{
	status_t error = B_OK;
	// compose the request message
	BMessage request(B_REG_SET_THREAD_AND_TEAM);
	if (error == B_OK)
		error = request.AddInt32("token", (int32)entryToken);
	if (error == B_OK && team >= 0)
		error = request.AddInt32("team", team);
	if (error == B_OK && thread >= 0)
		error = request.AddInt32("thread", thread);
	// send the request
	BMessage reply;
	if (error == B_OK)
		error = fMess.SendMessage(&request, &reply);
	// evaluate the reply
	if (error == B_OK && reply.what != B_REG_SUCCESS)
		reply.FindInt32("error", &error);
	return error;
}

// CompleteRegistration
status_t
BRoster::CompleteRegistration(team_id team, thread_id thread,
							  port_id port) const
{
	status_t error = B_OK;
	// compose the request message
	BMessage request(B_REG_COMPLETE_REGISTRATION);
	if (error == B_OK && team >= 0)
		error = request.AddInt32("team", team);
	if (error == B_OK && thread >= 0)
		error = request.AddInt32("thread", thread);
	if (error == B_OK && port >= 0)
		error = request.AddInt32("port", port);
	// send the request
	BMessage reply;
	if (error == B_OK)
		error = fMess.SendMessage(&request, &reply);
	// evaluate the reply
	if (error == B_OK && reply.what != B_REG_SUCCESS)
		reply.FindInt32("error", &error);
	return error;
}

// IsAppPreRegistered
bool
BRoster::IsAppPreRegistered(const entry_ref *ref, team_id team,
							app_info *info) const
{
	status_t error = B_OK;
	// compose the request message
	BMessage request(B_REG_IS_APP_PRE_REGISTERED);
	if (error == B_OK && ref)
		error = request.AddRef("ref", ref);
	if (error == B_OK && team >= 0)
		error = request.AddInt32("team", team);
	// send the request
	BMessage reply;
	if (error == B_OK)
		error = fMess.SendMessage(&request, &reply);
	// evaluate the reply
	bool isPreRegistered = false;
	if (error == B_OK) {
		if (reply.what == B_REG_SUCCESS) {
			if (request.FindBool("pre-registered", isPreRegistered) != B_OK)
				error = B_ERROR;
			if (error == B_OK && isPreRegistered && info)
				error = find_message_app_info(&reply, info);
		} else
			reply.FindInt32("error", &error);
	}
	return (error == B_OK && isPreRegistered);
}

// RemovePreRegApp
status_t
BRoster::RemovePreRegApp(uint32 entryToken) const
{
	status_t error = B_OK;
	// compose the request message
	BMessage request(B_REG_REMOVE_PRE_REGISTERED_APP);
	if (error == B_OK)
		error = request.AddInt32("token", (int32)entryToken);
	// send the request
	BMessage reply;
	if (error == B_OK)
		error = fMess.SendMessage(&request, &reply);
	// evaluate the reply
	if (error == B_OK && reply.what != B_REG_SUCCESS)
		reply.FindInt32("error", &error);
	return error;
}

// RemoveApp
status_t
BRoster::RemoveApp(team_id team) const
{
	status_t error = B_OK;
	// compose the request message
	BMessage request(B_REG_REMOVE_APP);
	if (error == B_OK && team >= 0)
		error = request.AddInt32("team", team);
	// send the request
	BMessage reply;
	if (error == B_OK)
		error = fMess.SendMessage(&request, &reply);
	// evaluate the reply
	if (error == B_OK && reply.what != B_REG_SUCCESS)
		reply.FindInt32("error", &error);
	return error;
}

// xLaunchAppPrivate
status_t
BRoster::xLaunchAppPrivate(const char *mimeSig, const entry_ref *ref,
						   BList* msgList, int cargs, char **args,
						   team_id *appTeam) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// UpdateActiveApp
bool
BRoster::UpdateActiveApp(team_id team) const
{
	return false; // not implemented
}

// SetAppFlags
void
BRoster::SetAppFlags(team_id team, uint32 flags) const
{
}

// DumpRoster
void
BRoster::DumpRoster() const
{
}

// resolve_app
status_t
BRoster::resolve_app(const char *inType, const entry_ref *ref,
					 entry_ref *appRef, char *appSig, uint32 *appFlags,
					 bool *wasDocument) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// translate_ref
status_t
BRoster::translate_ref(const entry_ref *ref, BMimeType *appMeta,
					   entry_ref *appRef, BFile *appFile, char *appSig,
					   bool *wasDocument) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// translate_type
status_t
BRoster::translate_type(const char *mimeType, BMimeType *meta,
						entry_ref *appRef, BFile *appFile,
						char *appSig) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// sniff_file
status_t
BRoster::sniff_file(const entry_ref *file, BNodeInfo *finfo,
					char *mimeType) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// is_wildcard
bool
BRoster::is_wildcard(const char *sig) const
{
	return false; // not implemented
}

// get_unique_supporting_app
status_t
BRoster::get_unique_supporting_app(const BMessage *apps, char *outSig) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// get_random_supporting_app
status_t
BRoster::get_random_supporting_app(const BMessage *apps, char *outSig) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// build_arg_vector
char **
BRoster::build_arg_vector(char **args, int *pargs, const entry_ref *appRef,
						  const entry_ref *docRef) const
{
	return NULL; // not implemented
}

// send_to_running
status_t
BRoster::send_to_running(team_id team, const entry_ref *appRef, int cargs,
						 char **args, const BList *msgList,
						 const entry_ref *ref) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// InitMessengers
void
BRoster::InitMessengers()
{
DBG(OUT("BRoster::InitMessengers()\n"));
	// find the registrar port
	port_id rosterPort = find_port(kRosterPortName);
	port_info info;
	if (rosterPort >= 0 && get_port_info(rosterPort, &info) == B_OK) {
DBG(OUT("  found roster port\n"));
		// ask for the MIME messenger
		fMess = BMessenger(info.team, rosterPort, 0, true);
		BMessage reply;
		status_t error = fMess.SendMessage(B_REG_GET_MIME_MESSENGER, &reply);
		if (error == B_OK && reply.what == B_REG_SUCCESS) {
DBG(OUT("  got reply from roster\n"));
			reply.FindMessenger("messenger", &fMimeMess);
		} else {
DBG(OUT("  no (useful) reply from roster: error: %lx: %s\n", error,
strerror(error)));
		}
	}
DBG(OUT("BRoster::InitMessengers() done\n"));
}


/*-----------------------------------------------------*/
/*----- Global be_roster ------------------------------*/

const BRoster *be_roster;

