/*
 * Copyright 2001-2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef T_ROSTER_H
#define T_ROSTER_H


#include "AppInfoList.h"
#include "RecentApps.h"
#include "RecentEntries.h"
#include "WatchingService.h"

#include <Locker.h>
#include <MessageQueue.h>
#include <Path.h>
#include <Roster.h>
#include <SupportDefs.h>

#include <hash_set>
#include <map>


#if __GNUC__ >= 4
using __gnu_cxx::hash_set;
#endif

using std::map;

class BMessage;
class WatchingService;

typedef map<int32, BMessageQueue*>	IARRequestMap;

class TRoster {
public:
							TRoster();
	virtual					~TRoster();

			void			HandleAddApplication(BMessage* request);
			void			HandleCompleteRegistration(BMessage* request);
			void			HandleIsAppRegistered(BMessage* request);
			void			HandleRemovePreRegApp(BMessage* request);
			void			HandleRemoveApp(BMessage* request);
			void			HandleSetThreadAndTeam(BMessage* request);
			void			HandleSetSignature(BMessage* request);
			void			HandleGetAppInfo(BMessage* request);
			void			HandleGetAppList(BMessage* request);
			void			HandleUpdateActiveApp(BMessage* request);
			void			HandleBroadcast(BMessage* request);
			void			HandleStartWatching(BMessage* request);
			void			HandleStopWatching(BMessage* request);
			void			HandleGetRecentDocuments(BMessage* request);
			void			HandleGetRecentFolders(BMessage* request);
			void			HandleGetRecentApps(BMessage* request);
			void			HandleAddToRecentDocuments(BMessage* request);
			void			HandleAddToRecentFolders(BMessage* request);
			void			HandleAddToRecentApps(BMessage* request);
			void			HandleLoadRecentLists(BMessage* request);
			void			HandleSaveRecentLists(BMessage* request);

			void			HandleRestartAppServer(BMessage* request);

			void			ClearRecentDocuments();
			void			ClearRecentFolders();
			void			ClearRecentApps();

			status_t		Init();

			status_t		AddApp(RosterAppInfo* info);
			void			RemoveApp(RosterAppInfo* info);
			void			UpdateActiveApp(RosterAppInfo* info);

			void			CheckSanity();

			void			SetShuttingDown(bool shuttingDown);
			status_t		GetShutdownApps(AppInfoList& userApps,
								AppInfoList& systemApps,
								AppInfoList& backgroundApps,
								hash_set<team_id>& vitalSystemApps);

			status_t		AddWatcher(Watcher* watcher);
			void			RemoveWatcher(Watcher* watcher);

private:
	// hook functions
			void			_AppAdded(RosterAppInfo* info);
			void			_AppRemoved(RosterAppInfo* info);
			void			_AppActivated(RosterAppInfo* info);
			void			_AppDeactivated(RosterAppInfo* info);

	// helper functions
	static	status_t		_AddMessageAppInfo(BMessage* message,
								const app_info* info);
	static	status_t		_AddMessageWatchingInfo(BMessage* message,
								const app_info* info);
			uint32			_NextToken();

			void			_AddIARRequest(IARRequestMap& map, int32 key,
								BMessage* request);
			void			_ReplyToIARRequests(BMessageQueue* requests,
								const RosterAppInfo* info);
			void			_ReplyToIARRequest(BMessage* request,
								const RosterAppInfo* info);

			void			_HandleGetRecentEntries(BMessage* request);

			void			_ValidateRunning(const entry_ref& ref,
								const char* signature);
			bool			_IsSystemApp(RosterAppInfo* info) const;

			status_t		_LoadRosterSettings(const char* path = NULL);
			status_t		_SaveRosterSettings(const char* path = NULL);
	static	const char*		kDefaultRosterSettingsFile;

private:
			BLocker			fLock;
			AppInfoList		fRegisteredApps;
			AppInfoList		fEarlyPreRegisteredApps;
			IARRequestMap	fIARRequestsByID;
			IARRequestMap	fIARRequestsByToken;
			RosterAppInfo*	fActiveApp;
			WatchingService	fWatchingService;
			RecentApps		fRecentApps;
			RecentEntries	fRecentDocuments;
			RecentEntries	fRecentFolders;
			uint32			fLastToken;
			bool			fShuttingDown;
			BPath			fSystemAppPath;
			BPath			fSystemServerPath;
};

#endif	// T_ROSTER_H
