/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */


#include "MessageStatus.h"


MessageStatus::MessageStatus()
	:
	fStatus(MAIL_WRITING)
{

}


MessageStatus::~MessageStatus()
{

}


void
MessageStatus::SetStatus(messageStatus status)
{
	fStatus = status;
}


messageStatus
MessageStatus::Status()
{
	return fStatus;
}


bool
MessageStatus::Reading()
{
	return fStatus == MAIL_READING;
}


bool
MessageStatus::Writing()
{
	return fStatus == MAIL_WRITING;
}


bool
MessageStatus::WritingDraft()
{
	return fStatus == MAIL_WRITING_DRAFT;
}


bool
MessageStatus::Replying()
{
	return fStatus == MAIL_REPLYING;
}


bool
MessageStatus::Forwarding()
{
	return fStatus == MAIL_FORWARDING;
}


bool
MessageStatus::Outgoing()
{
	return (fStatus == MAIL_WRITING
		|| fStatus == MAIL_WRITING_DRAFT
		|| fStatus == MAIL_REPLYING
		|| fStatus == MAIL_FORWARDING);
}


bool
MessageStatus::MailIsOnDisk()
{
	return (fStatus == MAIL_READING
		|| fStatus == MAIL_WRITING_DRAFT
		|| fStatus == MAIL_REPLYING
		|| fStatus == MAIL_FORWARDING);
}

