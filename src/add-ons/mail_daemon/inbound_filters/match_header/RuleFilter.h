#ifndef ZOIDBERG_RULE_FILTER_H
#define ZOIDBERG_RULE_FILTER_H
/* RuleFilter - performs action depending on matching a header value
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Message.h>
#include <List.h>
#include <MailAddon.h>

#include "StringMatcher.h"


typedef enum {
	Z_MOVE_TO,
	Z_FLAG,
	Z_TRASH,
	Z_SET_REPLY,
	Z_SET_READ
} z_mail_action_flags;


class RuleFilter : public MailFilter {
public:
								RuleFilter(MailProtocol& protocol,
									AddonSettings* settings);
			void				HeaderFetched(const entry_ref& ref,
									BFile* file);

private:
			StringMatcher		fMatcher;
			BString				fAttribute;
			BString				fArg;
			int32				fReplyAccount;
			z_mail_action_flags	fDoWhat;
};

#endif	/* ZOIDBERG_RULE_FILTER_H */
