/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef AUTO_CONFIG_H
#define AUTO_CONFIG_H


#include <List.h>
#include <Node.h>
#include <String.h>

#define INFO_DIR "Mail/ProviderInfo"

#define ATTR_NAME_POPSERVER "POP Server"
#define ATTR_NAME_IMAPSERVER "IMAP Server"
#define ATTR_NAME_SMTPSERVER "SMTP Server"
#define ATTR_NAME_AUTHPOP "POP Authentication"
#define ATTR_NAME_AUTHSMTP "SMTP Authentication"
#define ATTR_NAME_POPSSL "POP SSL"
#define ATTR_NAME_IMAPSSL "IMAP SSL"
#define ATTR_NAME_SMTPSSL "SMTP SSL"
#define ATTR_NAME_USERNAME "Username Pattern"


/*
ATTR_NAME_AUTHPOP:
	0	plain text
	1	APOP

ATTR_NAME_AUTHSMTP:
	0	none
	1	ESMTP
	2	POP3 before SMTP

ATTR_NAME_USERNAME:
	0	username is the email address (default)
	1	username is the local-part of the email address local-part@domain.net
	2	no username is proposed
*/



struct provider_info
{
	BString provider;

	BString pop_server;
	BString imap_server;
	BString smtp_server;

	int32 authentification_pop;
	int32 authentification_smtp;

	int32 ssl_pop;
	int32 ssl_imap;
	int32 ssl_smtp;

	int32 username_pattern;
};


class AutoConfig
{
	public:
		status_t		GetInfoFromMailAddress(const char* email,
												provider_info *info);
					
		// for debug
		void			PrintProviderInfo(provider_info* pInfo);
				  
	private:
		status_t		GetMXRecord(const char* provider, provider_info *info);
		status_t		GuessServerName(const char* provider,
											provider_info *info);

		BString			ExtractProvider(const char* email);
		status_t		LoadProviderInfo(const BString &provider, provider_info* info);
		bool			ReadProviderInfo(BNode *node, provider_info* info);
		
};



#endif
