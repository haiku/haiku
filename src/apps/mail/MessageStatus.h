/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */
#ifndef _MESSAGE_STATUS_H
#define _MESSAGE_STATUS_H


#include <SupportDefs.h>


enum messageStatus {
	MAIL_READING = 0,
	MAIL_WRITING,
	MAIL_WRITING_DRAFT,
	MAIL_REPLYING,
	MAIL_FORWARDING
};


class MessageStatus {
public:
							MessageStatus();
							~MessageStatus();

			void			SetStatus(messageStatus status);
			messageStatus	Status();

			bool			Reading();
			bool			Writing();
			bool			WritingDraft();
			bool			Replying();
			bool			Forwarding();

			bool			Outgoing();

			bool			MailIsOnDisk();

private:
			messageStatus	fStatus;
};


#endif	// _MESSAGE_STATUS_H

