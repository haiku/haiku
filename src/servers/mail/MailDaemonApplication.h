/*
 * Copyright 2007-2015, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIL_DAEMON_APPLICATION_H
#define MAIL_DAEMON_APPLICATION_H


#include <map>

#include <ObjectList.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Node.h>
#include <Query.h>
#include <Server.h>
#include <String.h>

#include <MailProtocol.h>

#include "LEDAnimation.h"
#include "DefaultNotifier.h"


class BNotification;
struct send_mails_info;


struct account_protocols {
	account_protocols();

	image_id				inboundImage;
	BInboundMailProtocol*	inboundProtocol;
	image_id				outboundImage;
	BOutboundMailProtocol*	outboundProtocol;
};


typedef std::map<int32, account_protocols> AccountMap;


class MailDaemonApplication : public BServer {
public:
								MailDaemonApplication();
	virtual						~MailDaemonApplication();

	virtual void				ReadyToRun();
	virtual	void				RefsReceived(BMessage* message);
	virtual	void				MessageReceived(BMessage* message);

	virtual void				Pulse();
	virtual bool				QuitRequested();

			void				InstallDeskbarIcon();
			void				RemoveDeskbarIcon();

			void				GetNewMessages(BMessage* message);
			void				SendPendingMessages(BMessage* message);

			void				MakeMimeTypes(bool remakeMIMETypes = false);

private:
			void				_InitAccounts();
			void				_InitAccount(BMailAccountSettings& settings);
			void				_ReloadAccounts(BMessage* message);
			void				_RemoveAccount(
									const account_protocols& account);

			BInboundMailProtocol* _CreateInboundProtocol(
									BMailAccountSettings& settings,
									image_id& image);
			BOutboundMailProtocol* _CreateOutboundProtocol(
									BMailAccountSettings& settings,
									image_id& image);

			BInboundMailProtocol* _InboundProtocol(int32 account);
			BOutboundMailProtocol* _OutboundProtocol(int32 account);

			void				_InitNewMessagesCount();
			void				_UpdateNewMessagesNotification();
			void				_UpdateAutoCheckRunner();

			void				_AddMessage(send_mails_info& info,
									const BEntry& entry, const BNode& node);

	static	bool				_IsPending(BNode& node);
	static	bool				_IsEntryInTrash(BEntry& entry);

private:
			BMessageRunner*		fAutoCheckRunner;
			BMailSettings		fSettingsFile;

			int32				fNewMessages;
			bool				fCentralBeep;
				// TRUE to do a beep when the status window closes. This happens
				// when all mail has been received, so you get one beep for
				// everything rather than individual beeps for each mail
				// account.
				// Set to TRUE by the 'mcbp' message that the mail Notification
				// filter sends us, cleared when the beep is done.
			BObjectList<BMessage> fFetchDoneRespondents;
			BObjectList<BQuery>	fQueries;

			LEDAnimation*		fLEDAnimation;

			BString				fAlertString;

			AccountMap			fAccounts;

			ErrorLogWindow*		fErrorLogWindow;
			BNotification*		fNotification;
};


#endif // MAIL_DAEMON_APPLICATION_H
