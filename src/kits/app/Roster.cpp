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


/*-------------------------------------------------------------*/
/* --------- app_info Struct and Values ------------------------ */

// constructor
app_info::app_info()
		: thread(-1),
		  team(-1),
		  port(-1),
		  flags(B_MULTIPLE_LAUNCH | B_ARGV_ONLY | _B_APP_INFO_RESERVED1_),
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
	return false; // not implemented
}

// IsRunning
bool
BRoster::IsRunning(entry_ref *ref) const
{
	return false; // not implemented
}

// TeamFor
team_id
BRoster::TeamFor(const char *mimeSig) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// TeamFor
team_id
BRoster::TeamFor(entry_ref *ref) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// GetAppList
void
BRoster::GetAppList(BList *teamIDList) const
{
}

// GetAppList
void
BRoster::GetAppList(const char *sig, BList *teamIDList) const
{
}

// GetAppInfo
status_t
BRoster::GetAppInfo(const char *sig, app_info *info) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// GetAppInfo
status_t
BRoster::GetAppInfo(entry_ref *ref, app_info *info) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// GetRunningAppInfo
status_t
BRoster::GetRunningAppInfo(team_id team, app_info *info) const
{
	return NOT_IMPLEMENTED; // not implemented
}

// GetActiveAppInfo
status_t
BRoster::GetActiveAppInfo(app_info *info) const
{
	return NOT_IMPLEMENTED; // not implemented
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
	return NOT_IMPLEMENTED; // not implemented
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
uint32
BRoster::AddApplication(const char *mimeSig, entry_ref *ref, uint32 flags,
						team_id team, thread_id thread, port_id port,
						bool fullReg) const
{
	return 0; // not implemented
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
void
BRoster::SetThreadAndTeam(uint32 entry_token, thread_id thread,
						  team_id team) const
{
}

// CompleteRegistration
void
BRoster::CompleteRegistration(team_id team, thread_id, port_id port) const
{
}

// IsAppPreRegistered
bool
BRoster::IsAppPreRegistered(entry_ref *ref, team_id team, app_info *info) const
{
	return false; // not implemented
}

// RemovePreRegApp
void
BRoster::RemovePreRegApp(uint32 entryToken) const
{
}

// RemoveApp
void
BRoster::RemoveApp(team_id team) const
{
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

