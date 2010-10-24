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
		fprintf(stderr, _GetReadText(), len);
		return false;
	}
	fprintf(stderr, _GetReplyText(), reply.String());
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
			fprintf(stderr, _GetReadText(), len);
			return false;
		}
		fprintf(stderr, _GetReplyText(), reply.String());
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
	SendCommand("pwd\n");
	BString reply;
	return rc;
}


bool
SftpClient::Connect(const string& server, const string& login,
	const string& passwd)
{
	bool rc = false;
	BString cmd("sftp ");
	BString host(server.c_str());
	BString port;
	if (host.FindFirst(':'))
		host.MoveInto(port, host.FindFirst(':'), host.Length());
	port.RemoveAll(":");
	if (port.Length())
		cmd << "-oPort=" << port << " ";
	cmd << login.c_str();
	cmd << "@" << host.String();
	printf("COMMAND: '%s'\n", cmd.String());
	SetCommandLine(cmd.String());
	rc = SpawningUploadClient::Connect(server, login, passwd);
	BString reply;
	ssize_t len;

	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, _GetLongReadText(), len);
		return false;
	}
	fprintf(stderr, _GetReplyText(), reply.String());
	if (reply.FindFirst("Connecting to ") != 0)
		return false;

	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, _GetLongReadText(), len);
		return false;
	}
	fprintf(stderr, _GetReplyText(), reply.String());
	if (reply.FindFirst(/*[pP]*/"assword:") < 0)
		return false;

	write(OutputPipe(), passwd.c_str(), strlen(passwd.c_str()));
	write(OutputPipe(), "\n", 1);

	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, _GetLongReadText(), len);
		return false;
	}
	fprintf(stderr, _GetReplyText(), reply.String());
	if (reply != "\n")
		return false;

	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, _GetLongReadText(), len);
		return false;
	}
	fprintf(stderr, _GetReplyText(), reply.String());
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
		fprintf(stderr, _GetReadText(), len);
		return false;
	}
	fprintf(stderr, _GetReplyText(), reply.String());
	if (reply.FindFirst("Uploading") < 0)
		return false;

	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, _GetReadText(), len);
		return false;
	}
	fprintf(stderr, _GetReplyText(), reply.String());
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

	// sftpd can't rename to an existing file...
	BString cmd("rm");
	cmd << " " << newPath.c_str() << "\n";
	fprintf(stderr, "CMD: '%s'\n", cmd.String());
	SendCommand(cmd.String());
	BString reply;

	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, _GetReadText(), len);
		return false;
	}
	fprintf(stderr, _GetReplyText(), reply.String());
	// we don't care if it worked or not.
	//if (reply.FindFirst("Removing") != 0 && reply.FindFirst("Couldn't") )
	//	return false;

	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, _GetReadText(), len);
		return false;
	}
	fprintf(stderr, _GetReplyText(), reply.String());
	if (reply.FindFirst("sftp>") < 0)
		return false;

	cmd = "rename";
	cmd << " " << oldPath.c_str() << " " << newPath.c_str() << "\n";
	SendCommand(cmd.String());
	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, _GetReadText(), len);
		return false;
	}
	fprintf(stderr, _GetReplyText(), reply.String());
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
		fprintf(stderr, _GetReadText(), len);
		return false;
	}
	fprintf(stderr, _GetReplyText(), reply.String());
	if (reply.FindFirst("Changing") < 0)
		return false;

	if ((len = ReadReply(&reply)) < 0) {
		fprintf(stderr, _GetReadText(), len);
		return false;
	}
	fprintf(stderr, _GetReplyText(), reply.String());
	if (reply.FindFirst("sftp>") < 0)
		return false;

	rc = true;
	return rc;
}


void
SftpClient::SetPassive(bool on)
{
}


const char*
SftpClient::_GetLongReadText() const
{
	return B_TRANSLATE("read: %ld\n");
}


const char*
SftpClient::_GetReadText() const
{
	return B_TRANSLATE("read: %d\n");
}


const char*
SftpClient::_GetReplyText() const
{
	return B_TRANSLATE("reply: '%s'\n");
}


