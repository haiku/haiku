/*
 * Copyright 2011-2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <MailFilter.h>


BMailFilter::BMailFilter(BMailProtocol& protocol,
	const BMailAddOnSettings* settings)
	:
	fMailProtocol(protocol),
	fSettings(settings)
{
}


BMailFilter::~BMailFilter()
{
}


BMailFilterAction
BMailFilter::HeaderFetched(entry_ref& ref, BFile& file, BMessage& attributes)
{
	return B_NO_MAIL_ACTION;
}


void
BMailFilter::BodyFetched(const entry_ref& ref, BFile& file,
	BMessage& attributes)
{
}


void
BMailFilter::MailboxSynchronized(status_t status)
{
}


void
BMailFilter::MessageReadyToSend(const entry_ref& ref, BFile& file)
{
}


void
BMailFilter::MessageSent(const entry_ref& ref, BFile& file)
{
}
