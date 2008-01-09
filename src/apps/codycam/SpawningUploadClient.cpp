#include <image.h>
#include <unistd.h>
#include <signal.h>

#include <String.h>

#include "SpawningUploadClient.h"

SpawningUploadClient::SpawningUploadClient()
	: FileUploadClient()
	, fCommand()
	, fCommandPid(-1)
	, fPty(-1)
{
}


SpawningUploadClient::~SpawningUploadClient()
{
	close(fPty);
	kill(fCommandPid, SIGTERM);
	kill(fCommandPid, SIGKILL);
	status_t ret;
	wait_for_thread(fCommandPid, &ret);
}


bool
SpawningUploadClient::Connect(const string& server, const string& login, const string& passwd)
{
	bool rc = false;
//	fCommand += " ";
//	fCommand += server;
	rc = SpawnCommand();
	return rc;
}


bool
SpawningUploadClient::SpawnCommand()
{
	//XXX: should use a system-provided TerminalCommand class
	BString shellcmd = "exec";
	const char *args[] = { "/bin/sh", "-c", NULL, NULL };
	char path[20];
	int pty;
	for (pty = 0; pty < 0x50; pty++) {
		int fd;
		sprintf(path, "/dev/pt/%c%1x", 'p' + pty / 16, pty & 0x0f);
		printf("trying %s\n", path);
		fd = open(path, O_RDWR);
		if (fd < 0)
			continue;
		fPty = fd;
		break;
	}
	if (fPty < 0)
		return false;
	sprintf(path, "/dev/tt/%c%1x", 'p' + pty / 16, pty & 0x0f);
	shellcmd << " 0<" << path;
	shellcmd << " 1>" << path;
	shellcmd += " 2>&1";
	shellcmd << " ; ";
	shellcmd << "export TTY=" << path << "; "; // BeOS hack
	shellcmd << "export LC_ALL=C; export LANG=C; ";
	shellcmd << "exec ";
	shellcmd << fCommand.c_str();
printf("spawning: '%s'\n", shellcmd.String());
	args[2] = shellcmd.String();
	fCommandPid = load_image(3, args, (const char **)environ);
	if (fCommandPid < 0)
		return false;
	resume_thread(fCommandPid);
	return true;
}


status_t
SpawningUploadClient::SetCommandLine(const char *command)
{
	fCommand = command;
	return B_OK;
}


ssize_t
SpawningUploadClient::SendCommand(const char *cmd)
{
	return write(InputPipe(), cmd, strlen(cmd));
}


ssize_t
SpawningUploadClient::ReadReply(BString *to)
{
	char buff[1024];
	ssize_t len;
	to->Truncate(0);
	len = read(OutputPipe(), buff, 1024);
	if (len < 0)
		return errno;
	to->Append(buff, len);
	return len;
}


status_t
SpawningUploadClient::ParseReply()
{
	return B_ERROR;
}


/* the rest is empty */


bool
SpawningUploadClient::ChangeDir(const string& dir)
{
	bool rc = false;
	return rc;
}


bool
SpawningUploadClient::ListDirContents(string& listing)
{
	bool rc = false;
	return rc;
}


bool
SpawningUploadClient::PrintWorkingDir(string& dir)
{
	bool rc = false;
	return rc;
}


bool
SpawningUploadClient::PutFile(const string& local, const string& remote, ftp_mode mode)
{
	bool rc = false;
	return rc;
}


bool
SpawningUploadClient::GetFile(const string& remote, const string& local, ftp_mode mode)
{
	bool rc = false;
	return rc;
}


// Note: this only works for local remote moves, cross filesystem moves
// will not work
bool
SpawningUploadClient::MoveFile(const string& oldPath, const string& newPath)
{
	bool rc = false;
	return rc;
}


bool
SpawningUploadClient::Chmod(const string& path, const string& mod)
{
	bool rc = false;
	return rc;
}


void
SpawningUploadClient::SetPassive(bool on)
{
}


