/*
 * Copyright 2004-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001, Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef NOTIFIER_CONFIG_VIEW_H
#define NOTIFIER_CONFIG_VIEW_H


#include <MailSettingsView.h>


enum {
	NOTIFY_BEEP				= 1,
	NOTIFY_ALERT			= 2,
	NOTIFY_BLINK_LEDS		= 4,
	NOTIFY_CENTRAL_ALERT	= 8,
	NOTIFY_CENTRAL_BEEP		= 16,
	NOTIFY_NOTIFICATION		= 32
};


class NotifierConfigView : public BMailSettingsView {
public:
								NotifierConfigView();

			void				SetTo(const BMessage *archive);

	virtual status_t			SaveInto(BMailAddOnSettings& settings) const;

	virtual void				AttachedToWindow();
	virtual void				MessageReceived(BMessage *msg);

private:
			void				_UpdateNotifyText();
};


#endif	// NOTIFIER_CONFIG_VIEW_H
