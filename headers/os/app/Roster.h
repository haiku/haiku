/******************************************************************************
/
/	File:			Roster.h
/
/	Description:	BRoster class lets you launch apps and keeps
/					track of apps that are running. 
/					Global be_roster represents the default BRoster.
/					app_info structure provides info for a running app.
/
/	Copyright 1995-98, Be Incorporated, All Rights Reserved.
/
******************************************************************************/

#ifndef _ROSTER_H
#define _ROSTER_H

#include <BeBuild.h>
#include <Messenger.h>
#include <Mime.h>
#include <Clipboard.h>

class BList;
class BMessage;
class BNodeInfo;

extern "C" int	_init_roster_();

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
	B_REQUEST_LAUNCHED = 0x00000001,
	B_REQUEST_QUIT = 0x00000002,
	B_REQUEST_ACTIVATED = 0x00000004
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
		bool		IsRunning(const char *mime_sig) const;
		bool		IsRunning(entry_ref *ref) const;
		team_id		TeamFor(const char *mime_sig) const;
		team_id		TeamFor(entry_ref *ref) const;
		void		GetAppList(BList *team_id_list) const;
		void		GetAppList(const char *sig, BList *team_id_list) const;
		status_t	GetAppInfo(const char *sig, app_info *info) const;
		status_t	GetAppInfo(entry_ref *ref, app_info *info) const;
		status_t	GetRunningAppInfo(team_id team, app_info *info) const;
		status_t	GetActiveAppInfo(app_info *info) const;
		status_t	FindApp(const char *mime_type, entry_ref *app) const;
		status_t	FindApp(entry_ref *ref, entry_ref *app) const;

/* Launching, activating, and broadcasting to apps */
		status_t	Broadcast(BMessage *msg) const;
		status_t	Broadcast(BMessage *msg, BMessenger reply_to) const;
		status_t	StartWatching(BMessenger target,
						uint32 event_mask = B_REQUEST_LAUNCHED | B_REQUEST_QUIT) const;
		status_t	StopWatching(BMessenger target) const;
		status_t	ActivateApp(team_id team) const;
		status_t	Launch(const char *mime_type, BMessage *initial_msgs = NULL,
						team_id *app_team = NULL) const;
		status_t	Launch(const char *mime_type, BList *message_list,
						team_id *app_team = NULL) const;
		status_t	Launch(const char *mime_type, int argc, char **args,
						team_id *app_team = NULL) const;

		status_t	Launch(const entry_ref *ref, const BMessage *initial_message = NULL,
						team_id *app_team = NULL) const;
		status_t	Launch(const entry_ref *ref, const BList *message_list,
						team_id *app_team = NULL) const;
		status_t	Launch(const entry_ref *ref, int argc, const char * const *args,
						team_id *app_team = NULL) const;

/* Recent document and app support */
		void		GetRecentDocuments(BMessage *refList, int32 maxCount,
						const char *ofType = NULL,
						const char *openedByAppSig = NULL) const;
		void		GetRecentDocuments(BMessage *refList, int32 maxCount,
						const char *ofTypeList[], int32 ofTypeListCount,
						const char *openedByAppSig = NULL) const;
		void		GetRecentFolders(BMessage *refList, int32 maxCount,
						const char *openedByAppSig = NULL) const;
		void		GetRecentApps(BMessage *refList, int32 maxCount) const;

		void		AddToRecentDocuments(const entry_ref *doc,
						const char *appSig = NULL) const;
		void		AddToRecentFolders(const entry_ref *folder,
						const char *appSig = NULL) const;
		
/*----- Private or reserved ------------------------------*/
private:

friend class BApplication;
friend class BWindow;
friend class _BAppCleanup_;
friend int	_init_roster_();
friend status_t _send_to_roster_(BMessage *, BMessage *, bool);
friend bool _is_valid_roster_mess_(bool);
friend status_t BMimeType::StartWatching(BMessenger);
friend status_t BMimeType::StopWatching(BMessenger);
friend status_t BClipboard::StartWatching(BMessenger);
friend status_t BClipboard::StopWatching(BMessenger);

		enum mtarget {
			MAIN_MESSENGER,
			MIME_MESSENGER,
			USE_GIVEN
		};

		status_t	_StartWatching(mtarget t, BMessenger *roster_mess, uint32 what,
									BMessenger notify, uint32 event_mask) const;
		status_t	_StopWatching(mtarget t, BMessenger *roster_mess, uint32 what,
									BMessenger notify) const;
		uint32		AddApplication(	const char *mime_sig,
									entry_ref *ref,
									uint32 flags,
									team_id team,
									thread_id thread,
									port_id port,
									bool full_reg) const;
		void		SetSignature(team_id team, const char *mime_sig) const;
		void		SetThread(team_id team, thread_id tid) const;
		void		SetThreadAndTeam(uint32 entry_token,
									 thread_id tid,
									 team_id team) const;
		void		CompleteRegistration(team_id team,
										thread_id,
										port_id port) const;
		bool		IsAppPreRegistered(	entry_ref *ref,
										team_id team,
										app_info *info) const;
		void		RemovePreRegApp(uint32 entry_token) const;
		void		RemoveApp(team_id team) const;

		status_t	xLaunchAppPrivate(	const char *mime_sig,
										const entry_ref *ref,
										BList* msg_list,
										int cargs,
										char **args,
										team_id *app_team) const;
		bool		UpdateActiveApp(team_id team) const;
		void		SetAppFlags(team_id team, uint32 flags) const;
		void		DumpRoster() const;
		status_t	resolve_app(const char *in_type,
								const entry_ref *ref,
								entry_ref *app_ref,
								char *app_sig,
								uint32 *app_flags,
								bool *was_document) const;
		status_t	translate_ref(const entry_ref *ref,
								BMimeType *app_meta,
								entry_ref *app_ref,
								BFile *app_file,
								char *app_sig,
								bool *was_document) const;
		status_t	translate_type(const char *mime_type,
								BMimeType *meta,
								entry_ref *app_ref,
								BFile *app_file,
								char *app_sig) const;
		status_t	sniff_file(const entry_ref *file,
								BNodeInfo *finfo,
								char *mime_type) const;
		bool		is_wildcard(const char *sig) const;
		status_t	get_unique_supporting_app(const BMessage *apps,
											char *out_sig) const;
		status_t	get_random_supporting_app(const BMessage *apps,
											char *out_sig) const;
		char		**build_arg_vector(char **args, int *pargs,
										const entry_ref *app_ref,
										const entry_ref *doc_ref) const;
		status_t	send_to_running(team_id tema,
									const entry_ref *app_ref,
									int cargs, char **args,
									const BList *msg_list,
									const entry_ref *ref) const;
		void		InitMessengers();

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
