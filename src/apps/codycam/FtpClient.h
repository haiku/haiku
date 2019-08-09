/*
 * Copyright 1998-1999 Be, Inc. All Rights Reserved.
 * Copyright 2003-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H


#include <stdio.h>
#include <string>

#include <File.h>
#include <NetAddress.h>
#include <NetEndpoint.h>

#include "FileUploadClient.h"

using std::string;


/*
 * Definitions for the TELNET protocol. Snarfed from the BSD source.
 */
#define IAC     255
#define DONT    254
#define DO      253
#define WONT    252
#define WILL    251
#define xEOF    236


class FtpClient : public FileUploadClient {
public:
								FtpClient();
								~FtpClient();

			bool 				Connect(const string& server,
									const string& login,
									const string& passwd);

			bool 				PutFile(const string& local,
									const string& remote,
									ftp_mode mode = binary_mode);

			bool 				GetFile(const string& remote,
									const string& local,
									ftp_mode mode = binary_mode);

			bool 				MoveFile(const string& oldPath,
									const string& newPath);
			bool 				ChangeDir(const string& dir);
			bool 				PrintWorkingDir(string& dir);
			bool 				ListDirContents(string& listing);
			bool 				Chmod(const string& path, const string& mod);

			void				SetPassive(bool on);

protected:
	enum {
		ftp_complete = 1UL,
		ftp_connected = 2,
		ftp_passive = 4
	};

			bool 				_TestState(unsigned long state);
			void 				_SetState(unsigned long state);
			void 				_ClearState(unsigned long state);

			bool 				_SendRequest(const string& cmd);
			bool 				_GetReply(string& outString, int& outCode,
									int& codeType);
			bool 				_GetReplyLine(string& line);
			bool 				_OpenDataConnection();
			bool 				_AcceptDataConnection();

			unsigned long 		fState;
			BNetEndpoint* 		fControl;
			BNetEndpoint* 		fData;

};

#endif	// FTP_CLIENT_H
