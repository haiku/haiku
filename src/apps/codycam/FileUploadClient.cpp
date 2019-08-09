/*
 * Copyright 1998-1999 Be, Inc. All Rights Reserved.
 * Copyright 2003-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FileUploadClient.h"


FileUploadClient::FileUploadClient()
{
}


FileUploadClient::~FileUploadClient()
{
}


bool
FileUploadClient::ChangeDir(const string& dir)
{
	return false;
}


bool
FileUploadClient::ListDirContents(string& listing)
{
	return false;
}


bool
FileUploadClient::PrintWorkingDir(string& dir)
{
	return false;
}


bool
FileUploadClient::Connect(const string& server, const string& login,
	const string& passwd)
{
	return false;
}


bool
FileUploadClient::PutFile(const string& local, const string& remote,
	ftp_mode mode)
{
	return false;
}


bool
FileUploadClient::GetFile(const string& remote, const string& local,
	ftp_mode mode)
{
	return false;
}


// Note: this only works for local remote moves, cross filesystem moves
// will not work
bool
FileUploadClient::MoveFile(const string& oldPath, const string& newPath)
{
	return false;
}


bool
FileUploadClient::Chmod(const string& path, const string& mod)
{
	return false;
}


void
FileUploadClient::SetPassive(bool on)
{
}
