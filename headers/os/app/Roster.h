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
//	File Name:		Roster.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	BRoster class lets you launch apps and keeps
//					track of apps that are running. 
//					Global be_roster represents the default BRoster.
//					app_info structure provides info for a running app.
//------------------------------------------------------------------------------

#ifndef _ROSTER_H
#define _ROSTER_H

#include <BeBuild.h>
#include <Messenger.h>
#include <Mime.h>
#include <Clipboard.h>

class BList;
class BMessage;
class BNodeInfo;

/*-------------------------------------------------------------*/
/* --------- app_info Struct and Values ------------------------ */

struct app_info {
	app_info();
	~app_info();

	thread_id	thread;
	team_id		team;
	port_id		port;
	uint32		flags;
	entry_ref	ref;
	char		signature[B_MIME_TYPE_LENGTH];
};

#define B_LAUNCH_MASK				(0x3)

#define B_SINGLE_LAUNCH				(0x0)
#define B_MULTIPLE_LAUNCH			(0x1)
#define B_EXCLUSIVE_LAUNCH			(0x2)

#define B_BACKGROUND_APP			(0x4)
#define B_ARGV_ONLY					(0x8)
#define _B_APP_INFO_RESERVED1_		(0x10000000)


enum {
	B_REQUEST_LAUNCHED	= 0x00000001,
	B_REQUEST_QUIT		= 0x00000002,
	B_REQUEST_ACTIVATED	= 0x00000004
};

enum {
	B_SOME_APP_LAUNCHED		= 'BRAS',
	B_SOME_APP_QUIT			= 'BRAQ',
	B_SOME_APP_ACTIVATED	= 'BRAW'
};

/*-------------------------------------------------------------*/
/* --------- BRoster class----------------------------------- */

class BRoster {
public:
	BRoster();
	~BRoster();
		
	/* Querying for apps */
	bool IsRunning(const char *mimeSig) const;
	bool IsRunning(entry_ref *ref) const;
	team_id TeamFor(const char *mimeSig) const;
	team_id TeamFor(entry_ref *ref) const;
	void GetAppList(BList *teamIDList) const;
	void GetAppList(const char *sig, BList *teamIDList) const;
	status_t GetAppInfo(const char *sig, app_info *info) const;
	status_t GetAppInfo(entry_ref *ref, app_info *info) const;
	status_t GetRunningAppInfo(team_id team, app_info *info) const;
	status_t GetActiveAppInfo(app_info *info) const;
	status_t FindApp(const char *mimeType, entry_ref *app) const;
	status_t FindApp(entry_ref *ref, entry_ref *app) const;

	/* Launching, activating, and broadcasting to apps */
	status_t Broadcast(BMessage *message) const;
	status_t Broadcast(BMessage *message, BMessenger replyTo) const;
	status_t StartWatching(BMessenger target,
		uint32 eventMask = B_REQUEST_LAUNCHED | B_REQUEST_QUIT) const;
	status_t StopWatching(BMessenger target) const;
	status_t ActivateApp(team_id team) const;

	status_t Launch(const char *mimeType, BMessage *initialMessage = NULL,
					team_id *appTeam = NULL) const;
	status_t Launch(const char *mimeType, BList *messageList,
					team_id *appTeam = NULL) const;
	status_t Launch(const char *mimeType, int argc, char **args,
					team_id *appTeam = NULL) const;
	status_t Launch(const entry_ref *ref,
					const BMessage *initialMessage = NULL,
					team_id *app_team = NULL) const;
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam = NULL) const;
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam = NULL) const;

	/* Recent document and app support */
	void GetRecentDocuments(BMessage *refList, int32 maxCount,
							const char *fileType = NULL,
							const char *appSig = NULL) const;
	void GetRecentDocuments(BMessage *refList, int32 maxCount,
							const char *fileTypes[], int32 fileTypesCount,
							const char *appSig = NULL) const;
	void GetRecentFolders(BMessage *refList, int32 maxCount,
						  const char *appSig = NULL) const;
	void GetRecentApps(BMessage *refList, int32 maxCount) const;

	void AddToRecentDocuments(const entry_ref *doc,
							  const char *appSig = NULL) const;
	void AddToRecentFolders(const entry_ref *folder,
							const char *appSig = NULL) const;
		
/*----- Private or reserved ------------------------------*/
	class Private;

private:
	friend class Private;

	class ArgVector;

	enum mtarget {
		MAIN_MESSENGER,
		MIME_MESSENGER,
		USE_GIVEN
	};

	status_t _StartWatching(mtarget target, BMessenger *rosterMess,
							uint32 what, BMessenger notify,
							uint32 event_mask) const;
	status_t _StopWatching(mtarget target, BMessenger *rosterMess, uint32 what,
						   BMessenger notify) const;
	status_t AddApplication(const char *mimeSig, const entry_ref *ref,
							uint32 flags, team_id team, thread_id thread,
							port_id port, bool fullReg, uint32 *token,
							team_id *otherTeam) const;
	status_t SetSignature(team_id team, const char *mimeSig) const;
	void SetThread(team_id team, thread_id thread) const;
	status_t SetThreadAndTeam(uint32 entryToken, thread_id thread,
							  team_id team) const;
	status_t CompleteRegistration(team_id team, thread_id thread,
								  port_id port) const;
	bool IsAppPreRegistered(const entry_ref *ref, team_id team,
							app_info *info) const;
	status_t RemovePreRegApp(uint32 entryToken) const;
	status_t RemoveApp(team_id team) const;

	status_t xLaunchAppPrivate(const char *mimeType, const entry_ref *ref,
							   const BList *messageList, int argc,
							   const char *const* args,
							   team_id *appTeam) const;
	bool UpdateActiveApp(team_id team) const;
	void SetAppFlags(team_id team, uint32 flags) const;
	void DumpRoster() const;
	status_t resolve_app(const char *inType, entry_ref *ref,
						 entry_ref *appRef, char *appSig, uint32 *appFlags,
						 bool *wasDocument) const;
	status_t translate_ref(entry_ref *ref, BMimeType *appMeta,
						   entry_ref *appRef, BFile *appFile,
						   bool *wasDocument) const;
	status_t translate_type(const char *mimeType, BMimeType *appMeta,
							entry_ref *appRef, BFile *appFile) const;
	status_t sniff_file(const entry_ref *file, BNodeInfo *nodeInfo,
						char *mimeType) const;
	status_t send_to_running(team_id team, int argc, const char *const *args,
							 const BList *messageList, const entry_ref *ref,
							 bool readyToRun) const;
	void InitMessengers();
	
	void AddToRecentApps(const char *appSig) const;
	void ClearRecentDocuments() const;
	void ClearRecentFolders() const;
	void ClearRecentApps() const;
	void LoadRecentLists(const char *file) const;
	void SaveRecentLists(const char *file) const;

	BMessenger	fMess;
	BMessenger	fMimeMess;
	uint32		_fReserved[3];
};

/*-----------------------------------------------------*/
/*----- Global be_roster ------------------------------*/

extern _IMPEXP_BE const BRoster *be_roster;

/*-----------------------------------------------------*/
/*-----------------------------------------------------*/

#endif /* _ROSTER_H */
