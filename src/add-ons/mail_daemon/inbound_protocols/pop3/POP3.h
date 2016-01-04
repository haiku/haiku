/*
 * Copyright 2007-2016, Haiku Inc. All Rights Reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef POP3_H
#define POP3_H


#include <map>
#include <vector>

#include <DataIO.h>
#include <List.h>
#include <String.h>
#include <StringList.h>
#include <View.h>

#include <MailProtocol.h>
#include <MailSettings.h>


class BSocket;


class POP3Protocol : public BInboundMailProtocol {
public:
								POP3Protocol(
									const BMailAccountSettings& settings);
								~POP3Protocol();

			status_t			Connect();
			status_t			Disconnect();

			status_t			SyncMessages();

			status_t			Retrieve(int32 message, BPositionIO* to);
			status_t			GetHeader(int32 message, BPositionIO* to);
			void				Delete(int32 index);

protected:
	virtual	status_t			HandleFetchBody(const entry_ref& ref,
									const BMessenger& replyTo);
	virtual	status_t			HandleDeleteMessage(const entry_ref& ref);

			// pop3 methods
			status_t			Open(const char* server, int port,
									int protocol);
			status_t			Login(const char* uid, const char* password,
									int method);

			size_t				MessageSize(int32 index);
			status_t			Stat();
			int32				Messages(void);
			size_t				MailDropSize(void);
			void				CheckForDeletedMessages();

			status_t			RetrieveInternal(const char* command,
									int32 message, BPositionIO* to,
									bool showProgress);

			ssize_t				ReceiveLine(BString& line);
			status_t			SendCommand(const char* cmd);
			void				MD5Digest(unsigned char* in, char* out);

private:
			status_t			_RetrieveUniqueIDs();
			void				_ReadManifest();
			void				_WriteManifest();

private:
			BString				fLog;
			int32				fNumMessages;
			size_t				fMailDropSize;
			std::vector<size_t>	fSizes;
			off_t				fTotalSize;
			BMessage			fSettings;

			BStringList			fManifest;
			BStringList			fUniqueIDs;

			BString				fDestinationDir;
			int32				fFetchBodyLimit;

			BSocket*			fServerConnection;
			bool				fUseSSL;
};


extern "C" status_t pop3_smtp_auth(const BMailAccountSettings& settings);


#endif	/* POP3_H */
