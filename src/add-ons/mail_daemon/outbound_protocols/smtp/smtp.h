/*
 * Copyright 2007-2008, Haiku Inc. All Rights Reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef ZOIDBERG_SMTP_H
#define ZOIDBERG_SMTP_H


#include <String.h>

#include <MailAddon.h>

#ifdef USE_SSL
#	include <openssl/ssl.h>
#	include <openssl/rand.h>
#endif


class SMTPProtocol : public BMailFilter {
	public:
		SMTPProtocol(BMessage *message, BMailChainRunner *runner);
		~SMTPProtocol();

		virtual status_t InitCheck(BString *verbose);
		virtual status_t ProcessMailMessage(BPositionIO **io_message, BEntry *io_entry,
			BMessage *io_headers, BPath *io_folder, const char *io_uid);

		//----Perfectly good holdovers from the old days
		status_t Open(const char *server, int port, bool esmtp);
		status_t Login(const char *uid, const char *password);
		void Close();
		status_t Send(const char *to, const char *from, BPositionIO *message);

		int32 ReceiveResponse(BString &line);
		status_t SendCommand(const char *cmd);

	private:
		status_t POP3Authentication();

		int _fd;
		BString fLog;
		BMessage *fSettings;
		BMailChainRunner *runner;
		int32 fAuthType;

#ifdef USE_SSL
		SSL_CTX *ctx;
		SSL *ssl;
		BIO *sbio;

		bool use_ssl;
		bool use_STARTTLS;
#endif

		status_t fStatus;
		BString fServerName;		// required for DIGEST-MD5
};

#endif	/* ZOIDBERG_SMTP_H */
