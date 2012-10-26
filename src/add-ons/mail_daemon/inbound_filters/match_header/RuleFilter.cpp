/*
 * Copyright 2004-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


//!	Performs action depending on matching a header value.


#include "RuleFilter.h"

#include <stdlib.h>
#include <stdio.h>

#include <Catalog.h>
#include <Directory.h>
#include <fs_attr.h>
#include <Node.h>
#include <String.h>

#include <MailProtocol.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RuleFilter"


RuleFilter::RuleFilter(BMailProtocol& protocol, BMailAddOnSettings* settings)
	:
	BMailFilter(protocol, settings)
{
	// attribute is adapted to our "capitalize-each-word-in-the-header" policy
	settings->FindString("attribute", &fAttribute);
	fAttribute.CapitalizeEachWord();

	settings->FindString("regex", &fExpression);
	int32 index = fExpression.FindFirst("REGEX:");
	if (index == B_ERROR || index > 0)
		EscapeRegexTokens(fExpression);
	else
		fExpression.RemoveFirst("REGEX:");

	fMatcher.SetPattern(fExpression, false);

	settings->FindString("argument", &fArg);
	settings->FindInt32("do_what", (int32*)&fAction);
	if (fAction == ACTION_REPLY_WITH)
		settings->FindInt32("argument", &fReplyAccount);
}


BString
RuleFilter::DescriptiveName() const
{
	BString name(B_TRANSLATE("Match \"%attribute\" against \"%regex\""));
	name.ReplaceAll("%attribute", fAttribute);
	name.ReplaceAll("%regex", fExpression);
	return name;
}


void
RuleFilter::HeaderFetched(const entry_ref& ref, BFile* file)
{
	// That field doesn't exist? NO match
	if (fAttribute == "")
		return;

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

	if (!fMatcher.Match(data)) {
		// We're not supposed to do anything
		return;
	}

	switch (fAction) {
		case ACTION_MOVE_TO:
		{
			BDirectory dir(fArg);
			// TODO: move is currently broken!
//			fMailProtocol.Looper()->TriggerFileMove(ref, dir);
			break;
		}
		case ACTION_DELETE_MESSAGE:
			// TODO trash!?
//			fMailProtocol.Looper()->TriggerFileDeletion(ref);
			break;

		case ACTION_SET_FLAGS_TO:
			file->WriteAttrString("MAIL:filter_flags", &fArg);
			break;

		case ACTION_REPLY_WITH:
			file->WriteAttr("MAIL:reply_with", B_INT32_TYPE, 0, &fReplyAccount,
				sizeof(int32));
			break;
		case ACTION_SET_AS_READ:
		{
			BInboundMailProtocol& protocol
				= (BInboundMailProtocol&)fMailProtocol;
			protocol.MarkMessageAsRead(ref, B_READ);
			break;
		}
		default:
			fprintf(stderr,"Unknown do_what: 0x%04x!\n", fAction);
	}

	return;
}


// #pragma mark -


BString
filter_name()
{
	return B_TRANSLATE("Rule filter");
}


BMailFilter*
instantiate_filter(BMailProtocol& protocol, BMailAddOnSettings* settings)
{
	return new RuleFilter(protocol, settings);
}
