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

#include <map>

#include <SupportDefs.h>

#include "AppInfoList.h"
#include "RecentApps.h"
#include "RecentEntries.h"
#include "WatchingService.h"

class BMessage;
class WatchingService;

struct IAPRRequest {
	entry_ref	ref;
	team_id		team;
	BMessage	*request;
};

typedef map<team_id, IAPRRequest>	IAPRRequestMap;

// For strategic reasons, as TRoster appears in the BMessenger header.
namespace BPrivate {

class TRoster {
public:
	TRoster();
	virtual ~TRoster();

	void HandleAddApplication(BMessage *request);
	void HandleCompleteRegistration(BMessage *request);
	void HandleIsAppPreRegistered(BMessage *request);
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
	void HandleClearRecentDocuments(BMessage *request);
	void HandleClearRecentFolders(BMessage *request);
	void HandleClearRecentApps(BMessage *request);

	status_t Init();

	status_t AddApp(RosterAppInfo *info);
	void RemoveApp(RosterAppInfo *info);
	void ActivateApp(RosterAppInfo *info);

	void CheckSanity();

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
	void _ReplyToIAPRRequest(BMessage *request, const RosterAppInfo *info);

	void _HandleGetRecentEntries(BMessage *request);

private:
	AppInfoList		fRegisteredApps;
	AppInfoList		fEarlyPreRegisteredApps;
	IAPRRequestMap	fIAPRRequests;
	RosterAppInfo	*fActiveApp;
	WatchingService	fWatchingService;
	RecentApps		fRecentApps;
	RecentEntries	fRecentDocuments;
	RecentEntries	fRecentFolders;
	uint32			fLastToken;
};

};	// namespace BPrivate

// Only the registrar code uses this header. No need to hide TRoster in the
// namespace.
using BPrivate::TRoster;

#endif	// T_ROSTER_H
