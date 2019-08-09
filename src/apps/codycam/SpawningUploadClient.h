/*
 * Copyright 1998-1999 Be, Inc. All Rights Reserved.
 * Copyright 2003-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SPAWNING_UPLOAD_CLIENT_H
#define SPAWNING_UPLOAD_CLIENT_H

#include "FileUploadClient.h"

#include <stdio.h>
#include <string.h>

#include <OS.h>
#include <String.h>


class SpawningUploadClient : public FileUploadClient {
public:
								SpawningUploadClient();
	virtual						~SpawningUploadClient();

	virtual bool				Connect(const string& server,
									const string& login,
									const string& passwd);

	virtual bool				PutFile(const string& local,
									const string& remote,
									ftp_mode mode = binary_mode);

	virtual bool				GetFile(const string& remote,
									const string& local,
									ftp_mode mode = binary_mode);

	virtual bool				MoveFile(const string& oldPath,
									const string& newPath);
	virtual bool				ChangeDir(const string& dir);
	virtual bool				PrintWorkingDir(string& dir);
	virtual bool				ListDirContents(string& listing);
	virtual bool				Chmod(const string& path, const string& mod);

	virtual void				SetPassive(bool on);

protected:
			status_t			SetCommandLine(const char* command);
			ssize_t				SendCommand(const char* cmd);
			ssize_t				ReadReply(BString* to);
	virtual status_t			ParseReply();
			int					getpty(char* pty, char* tty);

			int					InputPipe() const { return fPty; };
			int					OutputPipe() const { return fPty; };

private:
			bool				SpawnCommand();

private:
			string				fCommand;
			pid_t				fCommandPid;
			int					fPty;
};

#endif	// SPAWNING_UPLOAD_CLIENT_H
