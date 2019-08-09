/*
 * Copyright 1998-1999 Be, Inc. All Rights Reserved.
 * Copyright 2003-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_UPLOAD_CLIENT_H
#define FILE_UPLOAD_CLIENT_H


#include <stdio.h>
#include <string>

using std::string;


class FileUploadClient {
public:
								FileUploadClient();
	virtual						~FileUploadClient();

	enum ftp_mode {
		binary_mode,
		ascii_mode
	};

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
};

#endif	// FILE_UPLOAD_CLIENT_H
