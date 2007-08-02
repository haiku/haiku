/* 
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2007, Haiku Inc. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef POP3_H
#define POP3_H


#include <map>

#include <List.h>
#include <String.h>

#include "SimpleMailProtocol.h"


class POP3Protocol : public SimpleMailProtocol {
public:
  	POP3Protocol(BMessage *settings, BMailChainRunner *status);
  	~POP3Protocol();
  
	status_t Open(const char *server, int port, int protocol);
	status_t Login(const char *uid, const char *password, int method);
	status_t UniqueIDs();
	status_t Retrieve(int32 message, BPositionIO *write_to);
	status_t GetHeader(int32 message, BPositionIO *write_to);
	void Delete(int32 num);
	size_t MessageSize(int32 index);
	status_t Stat();
	int32 Messages(void);
	size_t MailDropSize(void);

protected:
	status_t RetrieveInternal(const char *command, int32 message,
		BPositionIO *writeTo, bool showProgress);

	int32 ReceiveLine(BString &line);
	status_t SendCommand(const char* cmd);
	void MD5Digest(unsigned char *in, char *out);

private:
	int				fSocket;
	BString			fLog;
	int32			fNumMessages;
	size_t			fMailDropSize;
	BList			fSizes;

#ifdef USESSL
	SSL_CTX*		fSSLContext;
	SSL*			fSSL;
	BIO*			fSSLBio;

	bool			fUseSSL;
#endif
};

#endif	/* POP3_H */
