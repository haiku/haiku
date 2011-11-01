/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIL_DAEMON_APP_H
#define MAIL_DAEMON_APP_H


#include <map>

#include <Application.h>
#include <ObjectList.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Node.h>
#include <Query.h>
#include <String.h>

#include <MailProtocol.h>

#include "LEDAnimation.h"
#include "Notifier.h"


class BNotification;


struct account_protocols {
	account_protocols() {
		inboundImage = -1;
		inboundThread = NULL;
		inboundProtocol = NULL;
		outboundImage = -1;
		outboundThread = NULL;
		outboundProtocol = NULL;
	}
	image_id				inboundImage;
	InboundProtocolThread*	inboundThread;
	InboundProtocol*		inboundProtocol;
	image_id				outboundImage;
	OutboundProtocolThread*	outboundThread;
	OutboundProtocol*		outboundProtocol;
};


typedef std::map<int32, account_protocols> AccountMap;


class MailDaemonApp : public BApplication {
public:
								MailDaemonApp();
	virtual						~MailDaemonApp();

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
			void				_RemoveAccount(AccountMap::const_iterator it);

			InboundProtocol*	_CreateInboundProtocol(
									BMailAccountSettings& settings,
									image_id& image);
			OutboundProtocol*	_CreateOutboundProtocol(
									BMailAccountSettings& settings,
									image_id& image);

			InboundProtocolThread*	_FindInboundProtocol(int32 account);
			OutboundProtocolThread*	_FindOutboundProtocol(int32 account);

			void				_UpdateAutoCheck(bigtime_t interval);

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
			uint32				fNotifyMode;
};


#endif // MAIL_DAEMON_APP_H
