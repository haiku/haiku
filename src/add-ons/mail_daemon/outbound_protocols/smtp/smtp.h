/*
 * Copyright 2007-2011, Haiku Inc. All Rights Reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef SMTP_H
#define SMTP_H


#include <String.h>

#include <MailAddon.h>
#include <MailProtocol.h>
#include <MailSettings.h>


#ifdef USE_SSL
#	include <openssl/ssl.h>
#	include <openssl/rand.h>
#endif


class SMTPProtocol : public OutboundProtocol {
public:
								SMTPProtocol(BMailAccountSettings* settings);
								~SMTPProtocol();

			status_t			Connect();
			void				Disconnect();

			status_t			SendMessages(const std::vector<entry_ref>&
									mails, size_t totalBytes);

			//----Perfectly good holdovers from the old days
			status_t			Open(const char *server, int port, bool esmtp);
			void				Close();
			status_t			Login(const char *uid, const char *password);
			status_t			Send(const char *to, const char *from,
									BPositionIO *message);

			int32				ReceiveResponse(BString &line);
			status_t			SendCommand(const char *cmd);

private:
			status_t			_SendMessage(const entry_ref& mail);
			status_t			_POP3Authentication();

			int					fSocket;
			BString				fLog;
			BMessage			fSettingsMessage;
			int32				fAuthType;

#ifdef USE_SSL
		SSL_CTX *ctx;
		SSL *ssl;
		BIO *sbio;

		bool use_ssl;
		bool use_STARTTLS;
#endif

			status_t			fStatus;
			BString				fServerName;	// required for DIGEST-MD5
};


#endif // SMTP_H
