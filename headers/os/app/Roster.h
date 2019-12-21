/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ROSTER_H
#define _ROSTER_H


#include <Entry.h>
#include <Messenger.h>
#include <OS.h>

class BFile;
class BMimeType;
class BNodeInfo;


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

class BList;


class BRoster {
public:
								BRoster();
								~BRoster();

	// running apps
			bool				IsRunning(const char* signature) const;
			bool				IsRunning(entry_ref* ref) const;

			team_id				TeamFor(const char* signature) const;
			team_id				TeamFor(entry_ref* ref) const;

			void				GetAppList(BList* teamIDList) const;
			void				GetAppList(const char* signature,
									BList* teamIDList) const;

	// app infos
			status_t			GetAppInfo(const char* signature,
									app_info* info) const;
			status_t			GetAppInfo(entry_ref* ref,
									app_info* info) const;

			status_t			GetRunningAppInfo(team_id team,
									app_info* info) const;
			status_t			GetActiveAppInfo(app_info* info) const;

	// find app
			status_t			FindApp(const char* mimeType,
									entry_ref* app) const;
			status_t			FindApp(entry_ref* ref, entry_ref* app) const;

	// broadcast
			status_t			Broadcast(BMessage* message) const;
			status_t			Broadcast(BMessage* message,
									BMessenger replyTo) const;

	// watching
			status_t			StartWatching(BMessenger target,
									uint32 eventMask
										= B_REQUEST_LAUNCHED
											| B_REQUEST_QUIT) const;
			status_t			StopWatching(BMessenger target) const;

			status_t			ActivateApp(team_id team) const;

	// launch app
			status_t			Launch(const char* mimeType,
									BMessage* initialMessage = NULL,
									team_id* _appTeam = NULL) const;
			status_t			Launch(const char* mimeType, BList* messageList,
									team_id* _appTeam = NULL) const;
			status_t			Launch(const char* mimeType, int argc,
									const char* const* args,
									team_id* _appTeam = NULL) const;
			status_t			Launch(const entry_ref* ref,
									const BMessage* initialMessage = NULL,
									team_id* _appTeam = NULL) const;
			status_t			Launch(const entry_ref* ref,
									const BList* messageList,
									team_id* _appTeam = NULL) const;
			status_t			Launch(const entry_ref* ref, int argc,
									const char* const* args,
									team_id* _appTeam = NULL) const;

	// recent documents, folders, apps
			void				GetRecentDocuments(BMessage* refList,
									int32 maxCount, const char* fileType = NULL,
									const char* signature = NULL) const;
			void				GetRecentDocuments(BMessage* refList,
									int32 maxCount, const char* fileTypes[],
									int32 fileTypesCount,
									const char* signature = NULL) const;

			void				GetRecentFolders(BMessage* refList,
									int32 maxCount,
									const char* signature = NULL) const;

			void				GetRecentApps(BMessage* refList,
									int32 maxCount) const;

			void				AddToRecentDocuments(const entry_ref* document,
									const char* signature = NULL) const;
			void				AddToRecentFolders(const entry_ref* folder,
									const char* signature = NULL) const;

	// private/reserved stuff starts here
	class Private;

private:
	class ArgVector;
	friend class Private;

			status_t			_ShutDown(bool reboot, bool confirm,
									bool synchronous);
			status_t			_IsShutDownInProgress(bool* inProgress);

			status_t			_AddApplication(const char* signature,
									const entry_ref* ref, uint32 flags,
									team_id team, thread_id thread,
									port_id port, bool fullRegistration,
									uint32* pToken, team_id* otherTeam) const;

			status_t			_SetSignature(team_id team,
									const char* signature) const;

			void				_SetThread(team_id team,
									thread_id thread) const;

			status_t			_SetThreadAndTeam(uint32 entryToken,
									thread_id thread, team_id team,
									port_id* _port) const;

			status_t			_CompleteRegistration(team_id team,
									thread_id thread, port_id port) const;

			bool				_IsAppPreRegistered(const entry_ref* ref,
									team_id team, app_info* info) const;

			status_t			_IsAppRegistered(const entry_ref* ref,
									team_id team, uint32 token,
									bool* preRegistered, app_info* info) const;

			status_t			_RemovePreRegApp(uint32 entryToken) const;
			status_t			_RemoveApp(team_id team) const;

			void				_ApplicationCrashed(team_id team);

			status_t			_LaunchApp(const char* mimeType,
									const entry_ref* ref,
									const BList* messageList, int argc,
									const char* const* args,
									const char** environment,
									team_id* _appTeam, thread_id* _appThread,
									port_id* _appPort, uint32* _appToken,
									bool launchSuspended) const;

			status_t			_UpdateActiveApp(team_id team) const;

			void				_SetAppFlags(team_id team, uint32 flags) const;

			void				_DumpRoster() const;

			status_t			_ResolveApp(const char* inType, entry_ref* ref,
									entry_ref* appRef,
									char* signature,
									uint32* appFlags,
									bool* wasDocument) const;

			status_t			_TranslateRef(entry_ref* ref,
									BMimeType* appMeta, entry_ref* appRef,
									BFile* appFile, bool* wasDocument) const;

			status_t			_TranslateType(const char* mimeType,
									BMimeType* appMeta, entry_ref* appRef,
									BFile* appFile) const;

			status_t			_GetFileType(const entry_ref* file,
									BNodeInfo* nodeInfo, char* mimeType) const;
			status_t			_SendToRunning(team_id team, int argc,
									const char* const* args,
									const BList* messageList,
									const entry_ref* ref,
									bool readyToRun) const;

			void				_SetWithoutRegistrar(bool noRegistrar);

			void				_InitMessenger();

	static	status_t			_InitMimeMessenger(void* data);

			BMessenger&			_MimeMessenger();

			void				_AddToRecentApps(const char* signature) const;

			void				_ClearRecentDocuments() const;
			void				_ClearRecentFolders() const;
			void				_ClearRecentApps() const;
			void				_LoadRecentLists(const char* filename) const;
			void				_SaveRecentLists(const char* filename) const;

			BMessenger			fMessenger;
			BMessenger			fMimeMessenger;
			int32				fMimeMessengerInitOnce;
			bool				fNoRegistrar;
			uint32				_reserved[1];
};

// global BRoster instance
extern const BRoster* be_roster;


#endif	// _ROSTER_H
