/*
 * Copyright 2007-2015, Haiku Inc. All Rights Reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef SMTP_H
#define SMTP_H


#include <String.h>

#include <MailFilter.h>
#include <MailProtocol.h>
#include <MailSettings.h>


class BSocket;


class SMTPProtocol : public BOutboundMailProtocol {
public:
								SMTPProtocol(
									const BMailAccountSettings& settings);
	virtual						~SMTPProtocol();

protected:
			status_t			Connect();
			void				Disconnect();

	virtual	status_t			HandleSendMessages(const BMessage& message,
									off_t totalBytes);

			status_t			Open(const char *server, int port, bool esmtp);
			void				Close();
			status_t			Login(const char *uid, const char *password);
			status_t			Send(const char *to, const char *from,
									BPositionIO *message);

			int32				ReceiveResponse(BString &line);
			status_t			SendCommand(const char *cmd);

private:
			status_t			_SendMessage(const entry_ref& ref);
			status_t			_POP3Authentication();

			BSocket*			fSocket;
			BString				fLog;
			BMessage			fSettingsMessage;
			int32				fAuthType;

			bool				use_ssl;

			status_t			fStatus;
			BString				fServerName;	// required for DIGEST-MD5
};


#endif // SMTP_H
