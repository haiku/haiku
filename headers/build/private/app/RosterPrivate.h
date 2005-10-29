//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _ROSTER_PRIVATE_H
#define _ROSTER_PRIVATE_H

#include <Messenger.h>
#include <Roster.h>

class BRoster::Private {
public:
	Private() : fRoster(const_cast<BRoster*>(be_roster)) {}
	Private(BRoster &roster) : fRoster(&roster) {}
	Private(BRoster *roster) : fRoster(roster) {}

	void SetTo(BMessenger mainMessenger, BMessenger mimeMessenger);

	status_t SendTo(BMessage *message, BMessage *reply, bool mime);
	bool IsMessengerValid(bool mime) const;

	status_t ShutDown(bool reboot, bool confirm, bool synchronous)
		{ return fRoster->ShutDown(reboot, confirm, synchronous); }

	// needed by BApplication

	status_t AddApplication(const char *mimeSig, const entry_ref *ref,
							uint32 flags, team_id team, thread_id thread,
							port_id port, bool fullReg, uint32 *token,
							team_id *otherTeam) const
		{ return fRoster->AddApplication(mimeSig, ref, flags, team, thread,
										 port, fullReg, token, otherTeam); }

	status_t SetSignature(team_id team, const char *mimeSig) const
		{ return fRoster->SetSignature(team, mimeSig); }

	status_t CompleteRegistration(team_id team, thread_id thread,
								  port_id port) const
		{ return fRoster->CompleteRegistration(team, thread, port); }

	bool IsAppPreRegistered(const entry_ref *ref, team_id team,
							app_info *info) const
		{ return fRoster->IsAppPreRegistered(ref, team, info); }

	status_t RemoveApp(team_id team) const
		{ return fRoster->RemoveApp(team); }

	// needed by GetRecentTester

	void AddToRecentApps(const char *appSig) const
		{ fRoster->AddToRecentApps(appSig); }

	void ClearRecentDocuments() const
		{ fRoster->ClearRecentDocuments(); }

	void ClearRecentFolders() const
		{ fRoster->ClearRecentFolders(); }

	void ClearRecentApps() const
		{ fRoster->ClearRecentApps(); }

	void LoadRecentLists(const char *file) const
		{ fRoster->LoadRecentLists(file); }

	void SaveRecentLists(const char *file) const
		{ fRoster->SaveRecentLists(file); }

	static void InitBeRoster();
	static void DeleteBeRoster();

private:
	BRoster	*fRoster;
};

#endif	// _ROSTER_PRIVATE_H
