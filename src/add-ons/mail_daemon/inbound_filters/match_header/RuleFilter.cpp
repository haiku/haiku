/*
 * Copyright 2004-2013, Haiku, Inc. All rights reserved.
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

#include "MatchHeaderSettings.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RuleFilter"


RuleFilter::RuleFilter(BMailProtocol& protocol,
	const BMailAddOnSettings& addOnSettings)
	:
	BMailFilter(protocol, &addOnSettings)
{
	MatchHeaderSettings settings(addOnSettings);

	// attribute is adapted to our "capitalize-each-word-in-the-header" policy
	fAttribute = settings.Attribute();
	fAttribute.CapitalizeEachWord();

	fExpression = settings.Expression();
	int32 index = fExpression.FindFirst("REGEX:");
	if (index == B_ERROR || index > 0)
		EscapeRegexTokens(fExpression);
	else
		fExpression.RemoveFirst("REGEX:");

	fMatcher.SetPattern(fExpression, false);

	fAction = settings.Action();
	fMoveTarget = settings.MoveTarget();
	fSetFlags = settings.SetFlagsTo();
	fReplyAccount = settings.ReplyAccount();
}


BMailFilterAction
RuleFilter::HeaderFetched(entry_ref& ref, BFile& file, BMessage& attributes)
{
	// That field doesn't exist? NO match
	if (fAttribute == "")
		return B_NO_MAIL_ACTION;

	attr_info info;
	if (file.GetAttrInfo("Subject", &info) != B_OK
		|| info.type != B_STRING_TYPE)
		return B_NO_MAIL_ACTION;

	BString data = attributes.GetString(fAttribute.String(), NULL);
	if (data.IsEmpty() || !fMatcher.Match(data)) {
		// We're not supposed to do anything
		return B_NO_MAIL_ACTION;
	}

	switch (fAction) {
		case ACTION_MOVE_TO:
		{
			BDirectory dir(fMoveTarget);
			node_ref nodeRef;
			status_t status = dir.GetNodeRef(&nodeRef);
			if (status != B_OK)
				return status;

			ref.device = nodeRef.device;
			ref.directory = nodeRef.node;
			return B_MOVE_MAIL_ACTION;
		}

		case ACTION_DELETE_MESSAGE:
			return B_DELETE_MAIL_ACTION;

		case ACTION_SET_FLAGS_TO:
			file.WriteAttrString("MAIL:filter_flags", &fSetFlags);
			break;

		case ACTION_REPLY_WITH:
			file.WriteAttr("MAIL:reply_with", B_INT32_TYPE, 0, &fReplyAccount,
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

	return B_NO_MAIL_ACTION;
}


// #pragma mark -


BString
filter_name(const BMailAccountSettings& accountSettings,
	const BMailAddOnSettings* addOnSettings)
{
	if (addOnSettings != NULL) {
		MatchHeaderSettings settings(*addOnSettings);
		if (settings.Attribute() != NULL && settings.Expression() != NULL) {
			BString name(
				B_TRANSLATE("Match \"%attribute\" against \"%regex\""));
			name.ReplaceAll("%attribute", settings.Attribute());
			name.ReplaceAll("%regex", settings.Expression());
			return name;
		}
	}
	return B_TRANSLATE("Match header");
}


BMailFilter*
instantiate_filter(BMailProtocol& protocol, const BMailAddOnSettings& settings)
{
	return new RuleFilter(protocol, settings);
}
