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

#include "MatchHeaderSettings.h"
#include "StringMatcher.h"


class RuleFilter : public BMailFilter {
public:
								RuleFilter(BMailProtocol& protocol,
									const BMailAddOnSettings& settings);

	virtual	void				HeaderFetched(const entry_ref& ref,
									BFile* file);

private:
			BString				fAttribute;
			BString				fExpression;
			StringMatcher		fMatcher;
			BString				fMoveTarget;
			BString				fSetFlags;
			int32				fReplyAccount;
			rule_action			fAction;
};


#endif	// RULE_FILTER_H
