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
//	File Name:		TRoster.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	TRoster is the incarnation of The Roster. It manages the
//					running applications.
//------------------------------------------------------------------------------

#ifndef T_ROSTER_H
#define T_ROSTER_H

#include <hash_set>
#include <map>

#include <Locker.h>
#include <MessageQueue.h>
#include <SupportDefs.h>

#include "AppInfoList.h"
#include "RecentApps.h"
#include "RecentEntries.h"
#include "WatchingService.h"

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
	virtual ~TRoster();

	void HandleAddApplication(BMessage *request);
	void HandleCompleteRegistration(BMessage *request);
	void HandleIsAppRegistered(BMessage *request);
	void HandleRemovePreRegApp(BMessage *request);
	void HandleRemoveApp(BMessage *request);
	void HandleSetThreadAndTeam(BMessage *request);
	void HandleSetSignature(BMessage *request);
	void HandleGetAppInfo(BMessage *request);
	void HandleGetAppList(BMessage *request);
	void HandleActivateApp(BMessage *request);
	void HandleBroadcast(BMessage *request);
	void HandleStartWatching(BMessage *request);
	void HandleStopWatching(BMessage *request);
	void HandleGetRecentDocuments(BMessage *request);
	void HandleGetRecentFolders(BMessage *request);
	void HandleGetRecentApps(BMessage *request);
	void HandleAddToRecentDocuments(BMessage *request);
	void HandleAddToRecentFolders(BMessage *request);
	void HandleAddToRecentApps(BMessage *request);
	void HandleLoadRecentLists(BMessage *request);
	void HandleSaveRecentLists(BMessage *request);

	void ClearRecentDocuments();
	void ClearRecentFolders();
	void ClearRecentApps();

	status_t Init();

	status_t AddApp(RosterAppInfo *info);
	void RemoveApp(RosterAppInfo *info);
	void ActivateApp(RosterAppInfo *info);

	void CheckSanity();

	void SetShuttingDown(bool shuttingDown);
	status_t GetShutdownApps(AppInfoList &userApps, AppInfoList &systemApps,
		AppInfoList &backgroundApps, hash_set<team_id> &vitalSystemApps,
		RosterAppInfo &inputServer);

	status_t AddWatcher(Watcher *watcher);
	void RemoveWatcher(Watcher *watcher);

private:
	// hook functions
	void _AppAdded(RosterAppInfo *info);
	void _AppRemoved(RosterAppInfo *info);
	void _AppActivated(RosterAppInfo *info);
	void _AppDeactivated(RosterAppInfo *info);

	// helper functions
	static status_t _AddMessageAppInfo(BMessage *message,
									   const app_info *info);
	static status_t _AddMessageWatchingInfo(BMessage *message,
											const app_info *info);
	uint32 _NextToken();

	void _AddIARRequest(IARRequestMap& map, int32 key, BMessage* request);
	void _ReplyToIARRequests(BMessageQueue *requests,
		const RosterAppInfo *info);
	void _ReplyToIARRequest(BMessage *request, const RosterAppInfo *info);

	void _HandleGetRecentEntries(BMessage *request);

	bool _IsSystemApp(RosterAppInfo *info) const;
	
	status_t _LoadRosterSettings(const char *path = NULL);
	status_t _SaveRosterSettings(const char *path = NULL);
	static const char *kDefaultRosterSettingsFile;
	
private:
	BLocker			fLock;
	AppInfoList		fRegisteredApps;
	AppInfoList		fEarlyPreRegisteredApps;
	IARRequestMap	fIARRequestsByID;
	IARRequestMap	fIARRequestsByToken;
	RosterAppInfo	*fActiveApp;
	WatchingService	fWatchingService;
	RecentApps		fRecentApps;
	RecentEntries	fRecentDocuments;
	RecentEntries	fRecentFolders;
	uint32			fLastToken;
	bool			fShuttingDown;
};

#endif	// T_ROSTER_H
