/* Add Fortune - adds fortunes to your mail
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include "ConfigView.h"

#include <Message.h>
#include <Entry.h>
#include <String.h>
#include <MailAddon.h>
#include <MailMessage.h>

#include <stdio.h>

#include "NodeMessage.h"

class FortuneFilter : public BMailFilter
{
	BMessage *settings;
  public:
	FortuneFilter(BMessage*);
	virtual status_t InitCheck(BString *err);
	virtual status_t ProcessMailMessage
	(
		BPositionIO** io_message, BEntry* io_entry,
		BMessage* io_headers, BPath* io_folder, const char* io_uid
	);
};

FortuneFilter::FortuneFilter(BMessage* msg)
	: BMailFilter(msg), settings(msg)
{
}

status_t FortuneFilter::InitCheck(BString* err)
{
	return B_OK;
}

status_t FortuneFilter::ProcessMailMessage
(BPositionIO** io, BEntry* io_entry, BMessage* headers, BPath* , const char*)
{
	// What we want to do here is to change the message body. To do that we use the
	// framework we already have by creating a new BEmailMessage based on the
	// BPositionIO, changing the message body and rendering it back to disk. Of course
	// this method ends up not being super-efficient, but it works. Ideas on how to 
	// improve this are welcome.
	BString	fortune_file;
	BString	tag_line;
	BString	fortune;
	
	BEmailMessage	mail_message(*io);
	BString				mail_body(mail_message.BodyText());
	
	// Obtain relevant settings
	settings->FindString("fortune_file", &fortune_file);
	settings->FindString("tag_line", &tag_line);
	
	// Add command to be executed
	fortune_file.Prepend("/bin/fortune ");
	
	char	buffer[768];
	FILE		*fd;
	
	fd = popen(fortune_file.String(), "r");
	if (fd)
	{
		mail_body += "\n--\n";
		mail_body += tag_line;
		
		while (fgets(buffer, 768, fd))
			mail_body += buffer;
		
		mail_body += "\n";
		
		pclose(fd);
		
		// Update the message body
		mail_message.SetBodyTextTo(mail_body.String());

		// Render it back to a BMallocIO object. We need this because we do not render
		// the entire message in memory so if we try to change the BPositionIO object we
		// have directly we will end up having the mail components pointing out to wrong
		// locations in the BPositionIo itself.
		BMallocIO shimmy_pipe;
		mail_message.RenderToRFC822(&shimmy_pipe);

		// Now we use the BMallocIO object and overwrite the BPositionIO one with it.		
		(*io)->Seek(0, SEEK_SET);
		(*io)->SetSize(0);
		(*io)->Write(shimmy_pipe.Buffer(),shimmy_pipe.BufferLength());
	}
	else
		printf("Could not open pipe to fortune!\n");

	return B_OK;	
}

BMailFilter* instantiate_mailfilter(BMessage* settings, BMailChainRunner*)
{
	return new FortuneFilter(settings);
}

BView* instantiate_config_panel(BMessage *settings,BMessage *)
{
	ConfigView *view = new ConfigView();
	view->SetTo(settings);

	return view;
}
