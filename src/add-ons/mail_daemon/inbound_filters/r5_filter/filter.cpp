/* R5 Daemon Filter - a filter comparable with the one from the original mail_daemon
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Message.h>
#include <FindDirectory.h>
#include <Entry.h>
#include <Roster.h>
#include <File.h>
#include <Path.h>
#include <String.h>
#include <stdio.h>
#include <image.h>
#include <stdlib.h>
#include <MailAddon.h>

#include "NodeMessage.h"

class CompatibilityFilter : public BMailFilter
{
	bool enabled;
	BPath path;
	BEntry filter;
	status_t status;
	
  public:
	CompatibilityFilter(BMessage*);
	virtual status_t InitCheck(BString *err);
	virtual status_t ProcessMailMessage
	(
		BPositionIO** io_message, BEntry* io_entry,
		BMessage* io_headers, BPath* io_folder, const char* io_uid
	);
};

CompatibilityFilter::CompatibilityFilter(BMessage* msg)
	: BMailFilter(msg), enabled(msg->FindBool("enabled")), status(B_OK)
{
	if (find_directory(B_USER_ADDONS_DIRECTORY, &path) != B_OK) {
		status = B_NAME_NOT_FOUND;
		return;
	}
	path.Append("MailDaemon/Filter");
	BEntry filter(path.Path());
}

status_t CompatibilityFilter::InitCheck(BString* err)
{
	return status;
}

status_t CompatibilityFilter::ProcessMailMessage
	(BPositionIO** , BEntry* io_entry, BMessage* headers, BPath* , const char*)
{
	int32 ret;
	
	if (enabled && filter.InitCheck() == B_OK && filter.Exists())
	{
		const char *refs[2];
		refs[0] = path.Path();
		
		BPath epath;
		io_entry->GetPath(&epath);
		refs[1] = epath.Path();
		
		thread_id fil = load_image(2, refs, (const char**)environ);
		if (fil >= B_OK)
		{
			/*BNode node(io_entry); //---ATT-Someone explain this, please.
			node << *message;*/
			
			wait_for_thread(fil,&ret);
			
			/*message->MakeEmpty();
			node >> *message;*/
		}
		else fprintf(stderr,"%s\n", strerror(fil));
	} 
	return B_OK;
}

BMailFilter* instantiate_mailfilter(BMessage* settings, BMailChainRunner*)
{
	return new CompatibilityFilter(settings);
}
