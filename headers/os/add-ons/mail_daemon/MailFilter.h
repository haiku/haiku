/*
 * Copyright 2011-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MAIL_FILTER_H
#define _MAIL_FILTER_H


#include "MailProtocol.h"
#include "MailSettings.h"


class BMailProtocol;
class BMailSettingsView;


class BMailFilter {
public:
								BMailFilter(BMailProtocol& protocol,
									const BMailAddOnSettings* settings);
	virtual						~BMailFilter();

	// Message hooks if filter is installed to an inbound protocol
	virtual	BMailFilterAction	HeaderFetched(entry_ref& ref, BFile& file,
									BMessage& attributes);
	virtual	void				BodyFetched(const entry_ref& ref, BFile& file,
									BMessage& attributes);
	virtual void				MailboxSynchronized(status_t status);

	// Message hooks if filter is installed to an outbound protocol
	virtual	void				MessageReadyToSend(const entry_ref& ref,
									BFile& file);
	virtual	void				MessageSent(const entry_ref& ref, BFile& file);

protected:
			BMailProtocol&		fMailProtocol;
			const BMailAddOnSettings* fSettings;
};


// Your filter needs to export these hooks in order to be picked up
extern "C" BMailSettingsView* instantiate_filter_settings_view(
	const BMailAccountSettings& accountSettings,
	const BMailAddOnSettings& settings);
extern "C" BString filter_name(const BMailAccountSettings& accountSettings,
	const BMailAddOnSettings* settings);
extern "C" BMailFilter* instantiate_filter(BMailProtocol& protocol,
	const BMailAddOnSettings& settings);


#endif	// _MAIL_FILTER_H
