/*
 * Copyright 2011-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MAIL_FILTER_H
#define _MAIL_FILTER_H


#include "MailProtocol.h"
#include "MailSettings.h"


class BMailProtocol;
class BView;


class BMailFilter {
public:
								BMailFilter(BMailProtocol& protocol,
									BMailAddOnSettings* settings);
	virtual						~BMailFilter();

	virtual BString				DescriptiveName() const = 0;

	// Message hooks if filter is installed to an inbound protocol
	virtual	void				HeaderFetched(const entry_ref& ref,
									BFile* file);
	virtual	void				BodyFetched(const entry_ref& ref, BFile* file);
	virtual void				MailboxSynchronized(status_t status);

	// Message hooks if filter is installed to an outbound protocol
	virtual	void				MessageReadyToSend(const entry_ref& ref,
									BFile* file);
	virtual	void				MessageSent(const entry_ref& ref,
									BFile* file);

protected:
			BMailProtocol&		fMailProtocol;
			BMailAddOnSettings*	fSettings;
};


// Your filter needs to export these hooks in order to be picked up
extern "C" BView* instantiate_filter_config_panel(BMailAddOnSettings& settings);
extern "C" BMailFilter* instantiate_filter(BMailProtocol& protocol,
	BMailAddOnSettings* settings);
extern "C" BString filter_name();


#endif	// _MAIL_FILTER_H
