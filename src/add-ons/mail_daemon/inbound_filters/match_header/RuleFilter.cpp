/* Match Header - performs action depending on matching a header value
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include "RuleFilter.h"

#include <stdlib.h>
#include <stdio.h>

#include <Catalog.h>
#include <Directory.h>
#include <fs_attr.h>
#include <Node.h>
#include <String.h>

#include <MailProtocol.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "RuleFilter"


RuleFilter::RuleFilter(MailProtocol& protocol, AddonSettings* addonSettings)
	:
	MailFilter(protocol, addonSettings)
{
	const BMessage* settings = &addonSettings->Settings();
	// attribute is adapted to our "capitalize-each-word-in-the-header" policy
	settings->FindString("attribute", &fAttribute);
	fAttribute.CapitalizeEachWord();

	BString regex;
	settings->FindString("regex", &regex);
	
	int32 index = regex.FindFirst("REGEX:");
	if (index == B_ERROR || index > 0)
		EscapeRegexTokens(regex);
	else
		regex.RemoveFirst("REGEX:");
		
	fMatcher.SetPattern(regex, false);
	
	settings->FindString("argument",&fArg);
	settings->FindInt32("do_what",(long *)&fDoWhat);
	if (fDoWhat == Z_SET_REPLY)
		settings->FindInt32("argument", &fReplyAccount);
}


void
RuleFilter::HeaderFetched(const entry_ref& ref, BFile* file)
{
	if (fAttribute == "")
		return; //----That field doesn't exist? NO match

	attr_info info;
	if (file->GetAttrInfo("Subject", &info) != B_OK
		|| info.type != B_STRING_TYPE)
		return;

	char* buffer = new char[info.size];
	if (file->ReadAttr(fAttribute, B_STRING_TYPE, 0, buffer, info.size) < 0) {
		delete[] buffer;
		return;
	}
	BString data = buffer;
	delete[] buffer;

	if (!fMatcher.Match(data))
		return; //-----There wasn't an error. We're just not supposed to do anything
	
	switch (fDoWhat) {
		case Z_MOVE_TO:
		{
			BDirectory dir(fArg);
			fMailProtocol.Looper()->TriggerFileMove(ref, dir);
			break;
		}
		case Z_TRASH:
			// TODO trash!?
			fMailProtocol.Looper()->TriggerFileDeletion(ref);
			break;

		case Z_FLAG:
			file->WriteAttrString("MAIL:filter_flags", &fArg);
			break;

		case Z_SET_REPLY:
			file->WriteAttr("MAIL:reply_with", B_INT32_TYPE, 0, &fReplyAccount,
				4);
			break;
		case Z_SET_READ:
		{
			InboundProtocol& protocol = (InboundProtocol&)fMailProtocol;
			protocol.MarkMessageAsRead(ref, B_READ);
			break;
		}
		default:
			fprintf(stderr,"Unknown do_what: 0x%04x!\n", fDoWhat);
	}
	
	return;
}


BString
descriptive_name()
{
	/*const char *attribute = NULL;
	settings->FindString("attribute",&attribute);
	const char *regex = NULL;
	settings->FindString("regex",&regex);

	if (!attribute || strlen(attribute) > 15)
		return B_ERROR;
	sprintf(buffer, "Match \"%s\"", attribute);

	if (!regex)
		return B_OK;

	char reg[20];
	strncpy(reg, regex, 16);
	if (strlen(regex) > 15)
		strcpy(reg + 15, "...");

	sprintf(buffer + strlen(buffer), " against \"%s\"", reg);

	return B_OK;*/
	return B_TRANSLATE("Rule filter");
}


MailFilter*
instantiate_mailfilter(MailProtocol& protocol, AddonSettings* settings)
{
	return new RuleFilter(protocol, settings);
}
