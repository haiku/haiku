/*
 * Copyright 2004-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef RULE_FILTER_H
#define RULE_FILTER_H


#include <Message.h>
#include <List.h>
#include <MailFilter.h>

#include "StringMatcher.h"


enum rule_action {
	ACTION_MOVE_TO,
	ACTION_SET_FLAGS_TO,
	ACTION_DELETE_MESSAGE,
	ACTION_REPLY_WITH,
	ACTION_SET_AS_READ
};


class RuleFilter : public BMailFilter {
public:
								RuleFilter(BMailProtocol& protocol,
									BMailAddOnSettings* settings);

	virtual BString				DescriptiveName() const;

	virtual	void				HeaderFetched(const entry_ref& ref,
									BFile* file);

private:
			BString				fAttribute;
			BString				fExpression;
			StringMatcher		fMatcher;
			BString				fArg;
			int32				fReplyAccount;
			rule_action			fAction;
};


#endif	// RULE_FILTER_H
