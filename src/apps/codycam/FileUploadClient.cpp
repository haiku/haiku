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
	bool rc = false;
	return rc;
}


bool
FileUploadClient::ListDirContents(string& listing)
{
	bool rc = false;
	return rc;
}


bool
FileUploadClient::PrintWorkingDir(string& dir)
{
	bool rc = false;
	return rc;
}


bool
FileUploadClient::Connect(const string& server, const string& login, const string& passwd)
{
	bool rc = false;
	return rc;
}


bool
FileUploadClient::PutFile(const string& local, const string& remote, ftp_mode mode)
{
	bool rc = false;
	return rc;
}


bool
FileUploadClient::GetFile(const string& remote, const string& local, ftp_mode mode)
{
	bool rc = false;
	return rc;
}


// Note: this only works for local remote moves, cross filesystem moves
// will not work
bool
FileUploadClient::MoveFile(const string& oldPath, const string& newPath)
{
	bool rc = false;
	return rc;
}


bool
FileUploadClient::Chmod(const string& path, const string& mod)
{
	bool rc = false;
	return rc;
}


void
FileUploadClient::SetPassive(bool on)
{
}


