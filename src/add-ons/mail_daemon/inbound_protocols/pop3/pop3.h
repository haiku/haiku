#ifndef ZOIDBERG_POP3_H
#define ZOIDBERG_POP3_H
/* POP3Protocol - implementation of the POP3 protocol
**
** Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
*/

#include <String.h>
#include <map>

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
	status_t RetrieveInternal(const char *command,int32 message, BPositionIO *write_to, bool show_progress);
	
	int32 ReceiveLine(BString &line);
	status_t SendCommand(const char* cmd);
	void MD5Digest (unsigned char *in, char *out); // MD5 Digest
	
private:
	int				conn;
	BString			fLog;
	int32			fNumMessages;
	size_t			fMailDropSize;
	BList			sizes;
        
        #ifdef USESSL
                SSL_CTX *ctx;
                SSL *ssl;
                BIO *sbio;
                
                bool use_ssl;
        #endif
};

#endif	/* ZOIDBERG_POP3_H */
