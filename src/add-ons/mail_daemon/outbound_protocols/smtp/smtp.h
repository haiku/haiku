#ifndef ZOIDBERG_SMTP_H
#define ZOIDBERG_SMTP_H
/* SMTPProtocol - implementation of the SMTP protocol
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <String.h>

#include <MailAddon.h>


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
		status_t POP3Authentification();

		int _fd;
		BString fLog;
		BMessage *fSettings;
		BMailChainRunner *runner;
		int32 fAuthType;

		status_t fStatus;
};

#endif	/* ZOIDBERG_SMTP_H */
