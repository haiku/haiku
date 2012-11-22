/*
 * Copyright 2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MAIL_SETTINGS_VIEW_H
#define _MAIL_SETTINGS_VIEW_H


#include <MailSettings.h>
#include <View.h>


class BMailSettingsView : public BView {
public:
								BMailSettingsView(const char* name);
	virtual						~BMailSettingsView();

	virtual status_t			SaveInto(
									BMailAddOnSettings& settings) const = 0;
};


#endif	// _MAIL_SETTINGS_VIEW_H
