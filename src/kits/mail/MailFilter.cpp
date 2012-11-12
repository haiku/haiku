/*
 * Copyright 2011-2012, Haiku, Inc. All rights reserved.
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


void
BMailFilter::HeaderFetched(const entry_ref& ref, BFile* file)
{
}


void
BMailFilter::BodyFetched(const entry_ref& ref, BFile* file)
{
}


void
BMailFilter::MailboxSynchronized(status_t status)
{
}


void
BMailFilter::MessageReadyToSend(const entry_ref& ref, BFile* file)
{
}


void
BMailFilter::MessageSent(const entry_ref& ref, BFile* file)
{
}
