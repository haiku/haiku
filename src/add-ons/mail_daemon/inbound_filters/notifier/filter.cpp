/* New Mail Notification - notifies incoming e-mail
 *
 * Copyright 2001, Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
*/


#include <Message.h>
#include <String.h>
#include <Alert.h>
#include <Beep.h>
#include <Path.h>
#include <Application.h>

#include <MailAddon.h>
#include <MDRLanguage.h>

#include "ConfigView.h"


class NotifyFilter : public MailFilter
{
public:
								NotifyFilter(MailProtocol& protocol,
									AddonSettings* settings);

			void				HeaderFetched(const entry_ref& ref,
									BFile* file);
			void				MailboxSynced(status_t status);
private:
			int32				fStrategy;
			int32				fNNewMessages;
};


NotifyFilter::NotifyFilter(MailProtocol& protocol, AddonSettings* settings)
	:
	MailFilter(protocol, settings),
	fNNewMessages(0)
{
	fStrategy = settings->Settings().FindInt32("notification_method");
}


void
NotifyFilter::HeaderFetched(const entry_ref& ref, BFile* file)
{
	char statusString[256];
	if (file->ReadAttr("MAIL:status", B_STRING_TYPE, 0, statusString, 256) < 0)
		return;
	if (BString(statusString).Compare("Read") != 0)
		fNNewMessages++;
}


void
NotifyFilter::MailboxSynced(status_t status)
{
	if (fNNewMessages == 0)
		return;

	if (fStrategy & do_beep)
		system_beep("New E-mail");

	if (fStrategy & alert) {
		BString text;
		MDR_DIALECT_CHOICE (
		text << "You have " << fNNewMessages << " new message" << ((fNNewMessages != 1) ? "s" : "")
		<< " for " << fMailProtocol.AccountSettings().Name() << ".",

		text << fMailProtocol.AccountSettings().Name() << "より\n" << fNNewMessages << " 通のメッセージが届きました");

		BAlert *alert = new BAlert(MDR_DIALECT_CHOICE ("New messages","新着メッセージ"), text.String(), "OK", NULL, NULL, B_WIDTH_AS_USUAL);
		alert->SetFeel(B_NORMAL_WINDOW_FEEL);
		alert->Go(NULL);
	}

	if (fStrategy & blink_leds)
		be_app->PostMessage('mblk');

	if (fStrategy & one_central_beep)
		be_app->PostMessage('mcbp');

	if (fStrategy & big_doozy_alert) {
		BMessage msg('numg');
		msg.AddInt32("num_messages", fNNewMessages);
		msg.AddString("name", fMailProtocol.AccountSettings().Name());

		be_app->PostMessage(&msg);
	}

	if (fStrategy & log_window) {
		BString message;
		message << fNNewMessages << " new message" << ((fNNewMessages != 1) ? "s" : "");
		fMailProtocol.ShowMessage(message.String());
	}

	fNNewMessages = 0;
}


MailFilter*
instantiate_mailfilter(MailProtocol& protocol, AddonSettings* settings)
{
	return new NotifyFilter(protocol, settings);
}

