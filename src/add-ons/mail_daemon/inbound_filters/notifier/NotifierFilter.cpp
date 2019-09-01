/*
 * Copyright 2004-2015, Haiku, Inc. All rights reserved.
 * Copyright 2001, Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */


//!	Notifies incoming e-mail


#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Catalog.h>
#include <Message.h>
#include <Path.h>
#include <String.h>
#include <StringFormat.h>

#include <MailFilter.h>

#include "NotifierConfigView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "NotifierFilter"


class NotifyFilter : public BMailFilter {
public:
								NotifyFilter(BMailProtocol& protocol,
									const BMailAddOnSettings& settings);

			BMailFilterAction	HeaderFetched(entry_ref& ref,
									BFile& file, BMessage& attributes);
			void				MailboxSynchronized(status_t status);

private:
			int32				fStrategy;
			int32				fNNewMessages;
};


NotifyFilter::NotifyFilter(BMailProtocol& protocol,
	const BMailAddOnSettings& settings)
	:
	BMailFilter(protocol, &settings),
	fNNewMessages(0)
{
	fStrategy = settings.FindInt32("notification_method");
}


BMailFilterAction
NotifyFilter::HeaderFetched(entry_ref& ref, BFile& file,
	BMessage& attributes)
{
	// TODO: do not use MAIL:status here!
	char statusString[256];
	if (file.ReadAttr("MAIL:status", B_STRING_TYPE, 0, statusString, 256) < 0)
		return B_NO_MAIL_ACTION;
	if (BString(statusString).Compare("Read") != 0)
		fNNewMessages++;
	return B_NO_MAIL_ACTION;
}


void
NotifyFilter::MailboxSynchronized(status_t status)
{
	if (fNNewMessages == 0)
		return;

	if ((fStrategy & NOTIFY_BEEP) != 0)
		system_beep("New E-mail");

	if ((fStrategy & NOTIFY_ALERT) != 0) {
		BStringFormat format(B_TRANSLATE(
			"You have {0, plural, one{one new message} other{# new messages}} "
			"for %account."));

		BString text;
		format.Format(text, fNNewMessages);
		text.ReplaceFirst("%account", fMailProtocol.AccountSettings().Name());

		BAlert *alert = new BAlert(B_TRANSLATE("New messages"), text.String(),
			B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->SetFeel(B_NORMAL_WINDOW_FEEL);
		alert->Go(NULL);
	}

	if ((fStrategy & NOTIFY_BLINK_LEDS) != 0)
		be_app->PostMessage('mblk');

	if ((fStrategy & NOTIFY_CENTRAL_BEEP) != 0)
		be_app->PostMessage('mcbp');

	if ((fStrategy & NOTIFY_CENTRAL_ALERT) != 0) {
		BMessage msg('numg');
		msg.AddInt32("num_messages", fNNewMessages);
		msg.AddString("name", fMailProtocol.AccountSettings().Name());

		be_app->PostMessage(&msg);
	}

	if ((fStrategy & NOTIFY_NOTIFICATION) != 0) {
		BStringFormat format(B_TRANSLATE("{0, plural, "
			"one{One new message} other{# new messages}}"));

		BString message;
		format.Format(message, fNNewMessages);
		fMailProtocol.ShowMessage(message.String());
	}

	fNNewMessages = 0;
}


// #pragma mark -


BString
filter_name(const BMailAccountSettings& accountSettings,
	const BMailAddOnSettings* addOnSettings)
{
	return B_TRANSLATE("New mails notification");
}


BMailFilter*
instantiate_filter(BMailProtocol& protocol, const BMailAddOnSettings& settings)
{
	return new NotifyFilter(protocol, settings);
}

