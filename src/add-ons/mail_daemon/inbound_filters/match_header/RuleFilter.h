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

class RuleFilter : public BMailFilter {
	public:
							RuleFilter(BMessage *settings);
		virtual				~RuleFilter();
					
		virtual status_t	InitCheck(BString* out_message = NULL);
		
		virtual status_t ProcessMailMessage(BPositionIO** io_message,
											   BEntry* io_entry,
											   BMessage* io_headers,
											   BPath* io_folder,
											   const char* io_uid);
	private:
		StringMatcher		matcher;
		const char*			attribute;
		const char*			arg;
		int32				chain_id;
		z_mail_action_flags	do_what;
};

#endif	/* ZOIDBERG_RULE_FILTER_H */
