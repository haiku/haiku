/*
 * Copyright 2004-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001, Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


//!	Adds fortunes to your mail


#include "ConfigView.h"

#include <Catalog.h>
#include <Message.h>
#include <Entry.h>
#include <String.h>
#include <MailFilter.h>
#include <MailMessage.h>

#include <stdio.h>

#include "NodeMessage.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FortuneFilter"


class FortuneFilter : public BMailFilter {
public:
								FortuneFilter(BMailProtocol& protocol,
									const BMailAddOnSettings& settings);

	virtual	void				MessageReadyToSend(const entry_ref& ref,
									BFile& file);
};


FortuneFilter::FortuneFilter(BMailProtocol& protocol,
	const BMailAddOnSettings& settings)
	:
	BMailFilter(protocol, &settings)
{
}


void
FortuneFilter::MessageReadyToSend(const entry_ref& ref, BFile& file)
{
	// What we want to do here is to change the message body. To do that we use the
	// framework we already have by creating a new BEmailMessage based on the
	// BPositionIO, changing the message body and rendering it back to disk. Of course
	// this method ends up not being super-efficient, but it works. Ideas on how to
	// improve this are welcome.
	BString	fortuneFile;
	BString	tagLine;

	// Obtain relevant settings
	fSettings->FindString("fortune_file", &fortuneFile);
	fSettings->FindString("tag_line", &tagLine);

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

	file.SetSize(0);
	mailMessage.RenderToRFC822(&file);
}


// #pragma mark -


BString
filter_name(const BMailAccountSettings& accountSettings,
	const BMailAddOnSettings* settings)
{
	return B_TRANSLATE("Fortune");
}


BMailFilter*
instantiate_filter(BMailProtocol& protocol,
	const BMailAddOnSettings& settings)
{
	return new FortuneFilter(protocol, settings);
}


BMailSettingsView*
instantiate_filter_settings_view(const BMailAccountSettings& accountSettings,
	const BMailAddOnSettings& settings)
{
	ConfigView* view = new ConfigView();
	view->SetTo(settings);

	return view;
}
