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


class FortuneFilter : public MailFilter
{
public:
								FortuneFilter(MailProtocol& protocol,
									AddonSettings* settings);
			void				MessageReadyToSend(const entry_ref& ref,
									BFile* file);
};


FortuneFilter::FortuneFilter(MailProtocol& protocol, AddonSettings* settings)
	:
	MailFilter(protocol, settings)
{

}


void
FortuneFilter::MessageReadyToSend(const entry_ref& ref, BFile* file)
{
	// What we want to do here is to change the message body. To do that we use the
	// framework we already have by creating a new BEmailMessage based on the
	// BPositionIO, changing the message body and rendering it back to disk. Of course
	// this method ends up not being super-efficient, but it works. Ideas on how to 
	// improve this are welcome.
	BString	fortuneFile;
	BString	tagLine;

	const BMessage* settings = &fAddonSettings->Settings();
	// Obtain relevant settings
	settings->FindString("fortune_file", &fortuneFile);
	settings->FindString("tag_line", &tagLine);
	
	// Add command to be executed
	fortuneFile.Prepend("/bin/fortune ");
	
	char buffer[768];
	FILE *fd;
	
	fd = popen(fortuneFile.String(), "r");
	if (!fd) {
		printf("Could not open pipe to fortune!\n");
		return;
	}

	BEmailMessage mailMessage(&ref);
	BString fortuneText;
	fortuneText += "\n--\n";
	fortuneText += tagLine;

	while (fgets(buffer, 768, fd)) {
		fortuneText += buffer;
		fortuneText += "\n";
	}
	pclose(fd);

	// Update the message body
	BTextMailComponent* body = mailMessage.Body();
	body->AppendText(fortuneText);

	file->SetSize(0);
	mailMessage.RenderToRFC822(file);
}


MailFilter*
instantiate_mailfilter(MailProtocol& protocol, AddonSettings* settings)
{
	return new FortuneFilter(protocol, settings);
}


BView*
instantiate_filter_config_panel(AddonSettings& settings)
{
	ConfigView *view = new ConfigView();
	view->SetTo(&settings.Settings());

	return view;
}
