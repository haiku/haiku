#ifndef ZOIDBERG_MAIL_SETTINGS_H
#define ZOIDBERG_MAIL_SETTINGS_H
/* Settings - the mail daemon's settings
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Archivable.h>
#include <List.h>
#include <Message.h>

class BPath;

typedef enum
{
	B_MAIL_SHOW_STATUS_WINDOW_NEVER         = 0,
	B_MAIL_SHOW_STATUS_WINDOW_WHEN_SENDING	= 1,
	B_MAIL_SHOW_STATUS_WINDOW_WHEN_ACTIVE	= 2,
	B_MAIL_SHOW_STATUS_WINDOW_ALWAYS        = 3
} b_mail_status_window_option;

typedef enum
{
	B_MAIL_STATUS_LOOK_TITLED                = 0,
	B_MAIL_STATUS_LOOK_NORMAL_BORDER         = 1,
	B_MAIL_STATUS_LOOK_FLOATING              = 2,
	B_MAIL_STATUS_LOOK_THIN_BORDER           = 3,
	B_MAIL_STATUS_LOOK_NO_BORDER             = 4
} b_mail_status_window_look;

typedef enum {
	inbound,
	outbound
} b_mail_chain_direction;

class BMailStatusWindow;
class BMailChain;

BMailChain* NewMailChain();
BMailChain* GetMailChain(uint32 id);

status_t GetOutboundMailChains(BList *list);
status_t GetInboundMailChains(BList *list);

class BMailChain : public BArchivable
{
  public:
	BMailChain(uint32 id);
	BMailChain(BMessage*);
	virtual ~BMailChain();
	
	virtual status_t Archive(BMessage*,bool) const;
	static BArchivable* Instantiate(BMessage*);
	
	status_t Save(bigtime_t timeout = B_INFINITE_TIMEOUT);
	status_t Delete() const;
	status_t Reload();
	status_t InitCheck() const;
	
	uint32 ID() const;
	
	b_mail_chain_direction ChainDirection() const;
	void SetChainDirection(b_mail_chain_direction);
	
	const char *Name() const;
	status_t SetName(const char*);
	
	BMessage *MetaData() const;
	
	// "Filter" below refers to the settings message for a MailFilter
	int32 CountFilters() const;
	status_t GetFilter(int32 index, BMessage* out_settings, entry_ref *addon = NULL) const;
	status_t SetFilter(int32 index, const BMessage&, const entry_ref&);
	
	status_t AddFilter(const BMessage&, const entry_ref&); // at end
	status_t AddFilter(int32 index, const BMessage&, const entry_ref&);
	status_t RemoveFilter(int32 index);
	
	void RunChain(BMailStatusWindow *window,
		bool async = true,
		bool save_when_done = true,
		bool delete_when_done = false);
	
  private:
	status_t Path(BPath *path) const;
	status_t Load(BMessage*);
	
	int32 id;
	char name[B_FILE_NAME_LENGTH];
	BMessage *meta_data;
	
	status_t _err;

  	b_mail_chain_direction direction;

	int32 settings_ct, addons_ct;  
	BList filter_settings;
	BList filter_addons;
	
	uint32 _reserved[5];
};

class BMailSettings
{
  public:
	BMailSettings();
	~BMailSettings();
	
	status_t Save(bigtime_t timeout = B_INFINITE_TIMEOUT);
	status_t Reload();
	status_t InitCheck() const;
	
	// Global settings
	int32 WindowFollowsCorner();
	void SetWindowFollowsCorner(int32 which_corner);
	
	uint32 ShowStatusWindow();
	void SetShowStatusWindow(uint32 mode);
	
	bool DaemonAutoStarts();
	void SetDaemonAutoStarts(bool does_it);

	void SetConfigWindowFrame(BRect frame);
	BRect ConfigWindowFrame();

	void SetStatusWindowFrame(BRect frame);
	BRect StatusWindowFrame();

	int32 StatusWindowWorkspaces();
	void SetStatusWindowWorkspaces(int32 workspaces);

	int32 StatusWindowLook();
	void SetStatusWindowLook(int32 look);
	
	bigtime_t AutoCheckInterval();
	void SetAutoCheckInterval(bigtime_t);
	
	bool CheckOnlyIfPPPUp();
	void SetCheckOnlyIfPPPUp(bool yes);
	
	bool SendOnlyIfPPPUp();
	void SetSendOnlyIfPPPUp(bool yes);
	
	uint32 DefaultOutboundChainID();
	void SetDefaultOutboundChainID(uint32 to);

  private:
	BMessage data;
	uint32 _reserved[4];
};

#endif	/* ZOIDBERG_MAIL_SETTINGS_H */
