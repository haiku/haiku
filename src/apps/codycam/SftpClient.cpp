#include <String.h>
#include <unistd.h>

#include "SftpClient.h"

SftpClient::SftpClient()
	: SpawningUploadClient()
{
}


SftpClient::~SftpClient()
{
	if (OutputPipe() >= 0)
		SendCommand("quit\n");
}


bool
SftpClient::ChangeDir(const string& dir)
{
	bool rc = false;
	int len;
	BString cmd("cd");
	BString reply;
	cmd << " " << dir.c_str() << "\n";
	SendCommand(cmd.String());
	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, "read: %s\n", len);
		return false;
	}
	fprintf(stderr, "reply: '%s'\n", reply.String());
	if (reply.FindFirst("sftp>") < 0)
		return false;
	rc = true;
	return rc;
}


bool
SftpClient::ListDirContents(string& listing)
{
	bool rc = false;
	int len;
	SendCommand("ls\n");
	BString reply;
	do {
		if ((len = ReadReply(&reply)) < 0) {
			fprintf(stderr, "read: %s\n", len);
			return false;
		}
		fprintf(stderr, "reply: '%s'\n", reply.String());
		printf("%s", reply.String());
		if (reply.FindFirst("sftp>") == 0)
			return true;
	} while (true);
	return rc;
}


bool
SftpClient::PrintWorkingDir(string& dir)
{
	bool rc = false;
	int len;
	SendCommand("pwd\n");
	BString reply;
	return rc;
}


bool
SftpClient::Connect(const string& server, const string& login, const string& passwd)
{
	bool rc = false;
	BString cmd("sftp ");
	cmd << login.c_str();
	cmd << "@" << server.c_str();
	SetCommandLine(cmd.String());
	rc = SpawningUploadClient::Connect(server, login, passwd);
	BString reply;
	ssize_t len;
	
	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, "read: %d\n", len);
		return false;
	}
	fprintf(stderr, "reply: '%s'\n", reply.String());
	if (reply.FindFirst("Connecting to ") != 0)
		return false;
	
	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, "read: %s\n", len);
		return false;
	}
	fprintf(stderr, "reply: '%s'\n", reply.String());
	if (reply.FindFirst("password:") < 0)
		return false;
	
	write(OutputPipe(), passwd.c_str(), strlen(passwd.c_str()));
	write(OutputPipe(), "\n", 1);
	
	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, "read: %s\n", len);
		return false;
	}
	fprintf(stderr, "reply: '%s'\n", reply.String());
	if (reply != "\n")
		return false;
	
	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, "read: %s\n", len);
		return false;
	}
	fprintf(stderr, "reply: '%s'\n", reply.String());
	if (reply.FindFirst("sftp>") < 0)
		return false;
	return rc;
}


bool
SftpClient::PutFile(const string& local, const string& remote, ftp_mode mode)
{
	bool rc = false;
	int len;
	//XXX: handle mode ?
	BString cmd("put");
	cmd << " " << local.c_str() << " " << remote.c_str() << "\n";
	SendCommand(cmd.String());
	BString reply;

	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, "read: %s\n", len);
		return false;
	}
	fprintf(stderr, "reply: '%s'\n", reply.String());
	if (reply.FindFirst("Uploading") < 0)
		return false;

	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, "read: %s\n", len);
		return false;
	}
	fprintf(stderr, "reply: '%s'\n", reply.String());
	if (reply.FindFirst("sftp>") < 0)
		return false;

	rc = true;
	return rc;
}


bool
SftpClient::GetFile(const string& remote, const string& local, ftp_mode mode)
{
	bool rc = false;
	//XXX
	return rc;
}


// Note: this only works for local remote moves, cross filesystem moves
// will not work
bool
SftpClient::MoveFile(const string& oldPath, const string& newPath)
{
	bool rc = false;
	int len;
	BString cmd("rename");
	cmd << " " << oldPath.c_str() << " " << newPath.c_str() << "\n";
	SendCommand(cmd.String());
	BString reply;
	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, "read: %s\n", len);
		return false;
	}
	fprintf(stderr, "reply: '%s'\n", reply.String());
	if (reply.FindFirst("sftp>") < 0)
		return false;
	rc = true;
	return rc;
}


bool
SftpClient::Chmod(const string& path, const string& mod)
{
	bool rc = false;
	int len;
	//XXX: handle mode ?
	BString cmd("chmod");
	cmd << " " << mod.c_str() << " " << path.c_str() << "\n";
	SendCommand(cmd.String());
	BString reply;

	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, "read: %s\n", len);
		return false;
	}
	fprintf(stderr, "reply: '%s'\n", reply.String());
	if (reply.FindFirst("Changing") < 0)
		return false;

	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, "read: %s\n", len);
		return false;
	}
	fprintf(stderr, "reply: '%s'\n", reply.String());
	if (reply.FindFirst("sftp>") < 0)
		return false;

	rc = true;
	return rc;
}


void
SftpClient::SetPassive(bool on)
{
}


