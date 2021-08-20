//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
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

#include <Entry.h>
#include <Messenger.h>
#include <OS.h>

// app_info
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

// app flags
#define B_SINGLE_LAUNCH			(0x0)
#define B_MULTIPLE_LAUNCH		(0x1)
#define B_EXCLUSIVE_LAUNCH		(0x2)
#define B_LAUNCH_MASK			(0x3)
#define B_BACKGROUND_APP		(0x4)
#define B_ARGV_ONLY				(0x8)
#define _B_APP_INFO_RESERVED1_	(0x10000000)

// watching request flags
enum {
	B_REQUEST_LAUNCHED	= 0x00000001,
	B_REQUEST_QUIT		= 0x00000002,
	B_REQUEST_ACTIVATED	= 0x00000004,
};

// notification message "what"
enum {
	B_SOME_APP_LAUNCHED		= 'BRAS',
	B_SOME_APP_QUIT			= 'BRAQ',
	B_SOME_APP_ACTIVATED	= 'BRAW',
};

class BFile;
class BList;
class BMimeType;
class BNodeInfo;

// BRoster
class BRoster {
public:
	BRoster();
	~BRoster();

	// running apps
	bool IsRunning(const char *mimeSig) const;
	bool IsRunning(entry_ref *ref) const;
	team_id TeamFor(const char *mimeSig) const;
	team_id TeamFor(entry_ref *ref) const;
	void GetAppList(BList *teamIDList) const;
	void GetAppList(const char *sig, BList *teamIDList) const;

	// app infos
	status_t GetAppInfo(const char *sig, app_info *info) const;
	status_t GetAppInfo(entry_ref *ref, app_info *info) const;
	status_t GetRunningAppInfo(team_id team, app_info *info) const;
	status_t GetActiveAppInfo(app_info *info) const;

	// find app
	status_t FindApp(const char *mimeType, entry_ref *app) const;
	status_t FindApp(entry_ref *ref, entry_ref *app) const;

	// broadcast
	status_t Broadcast(BMessage *message) const;
	status_t Broadcast(BMessage *message, BMessenger replyTo) const;

	// watching
	status_t StartWatching(BMessenger target,
						   uint32 eventMask = B_REQUEST_LAUNCHED
											  | B_REQUEST_QUIT) const;
	status_t StopWatching(BMessenger target) const;

	status_t ActivateApp(team_id team) const;

	// launch app
	status_t Launch(const char *mimeType, BMessage *initialMessage = 0,
					team_id *appTeam = 0) const;
	status_t Launch(const char *mimeType, BList *messageList,
					team_id *appTeam = 0) const;
	status_t Launch(const char *mimeType, int argc, char **args,
					team_id *appTeam = 0) const;
	status_t Launch(const entry_ref *ref, const BMessage *initialMessage = 0,
					team_id *appTeam = 0) const;
	status_t Launch(const entry_ref *ref, const BList *messageList,
					team_id *appTeam = 0) const;
	status_t Launch(const entry_ref *ref, int argc, const char * const *args,
					team_id *appTeam = 0) const;

	// recent documents, folders, apps
	void GetRecentDocuments(BMessage *refList, int32 maxCount,
							const char *fileType = 0,
							const char *appSig = 0) const;
	void GetRecentDocuments(BMessage *refList, int32 maxCount,
							const char *fileTypes[], int32 fileTypesCount,
							const char *appSig = 0) const;
	void GetRecentFolders(BMessage *refList, int32 maxCount,
						  const char *appSig = 0) const;
	void GetRecentApps(BMessage *refList, int32 maxCount) const;
	void AddToRecentDocuments(const entry_ref *doc,
							  const char *appSig = 0) const;
	void AddToRecentFolders(const entry_ref *folder,
							const char *appSig = 0) const;

	// private/reserved stuff starts here
	class Private;

private:
	class ArgVector;
	friend class Private;

	status_t ShutDown(bool reboot, bool confirm, bool synchronous);

	status_t AddApplication(const char *mimeSig, const entry_ref *ref,
							uint32 flags, team_id team, thread_id thread,
							port_id port, bool fullReg, uint32 *pToken,
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
							   const char *const *args,
							   team_id *appTeam) const;
	bool UpdateActiveApp(team_id team) const;
	void SetAppFlags(team_id team, uint32 flags) const;
	void DumpRoster() const;
	status_t resolve_app(const char *inType, entry_ref *ref, entry_ref *appRef,
						 char *appSig, uint32 *appFlags,
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
	void LoadRecentLists(const char *filename) const;
	void SaveRecentLists(const char *filename) const;

	BMessenger	fMess;
	BMessenger	fMimeMess;
	uint32		_reserved[3];
};

// global BRoster instance
extern const BRoster *be_roster;

#endif	// _ROSTER_H
