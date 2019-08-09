/*
 * Copyright 1998-1999 Be, Inc. All Rights Reserved.
 * Copyright 2003-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "SpawningUploadClient.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <image.h>
#include <String.h>


SpawningUploadClient::SpawningUploadClient()
	:
	FileUploadClient(),
	fCommand(),
	fCommandPid(-1),
	fPty(-1)
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
SpawningUploadClient::Connect(const string& server, const string& login,
	const string& passwd)
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
	char ptypath[20];
	char ttypath[20];
	//XXX: should use a system-provided TerminalCommand class
	BString shellcmd = "exec";
	const char *args[] = { "/bin/sh", "-c", NULL, NULL };
	fPty = getpty(ptypath, ttypath);
	if (fPty < 0)
		return B_ERROR;

	shellcmd << " 0<" << ttypath;
	shellcmd << " 1>" << ttypath;
	shellcmd += " 2>&1";
	shellcmd << " ; ";
	shellcmd << "export TTY=" << ttypath << "; "; // BeOS hack
	shellcmd << "export LC_ALL=C; export LANG=C; ";
	shellcmd << "exec ";
	shellcmd << fCommand.c_str();
	printf("spawning: '%s'\n", shellcmd.String());
	args[2] = shellcmd.String();
	fCommandPid = load_image(3, args, (const char**)environ);
	if (fCommandPid < 0)
		return false;
	resume_thread(fCommandPid);

	return true;
}


status_t
SpawningUploadClient::SetCommandLine(const char* command)
{
	fCommand = command;
	return B_OK;
}


ssize_t
SpawningUploadClient::SendCommand(const char* cmd)
{
	return write(InputPipe(), cmd, strlen(cmd));
}


ssize_t
SpawningUploadClient::ReadReply(BString* to)
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


int
SpawningUploadClient::getpty(char* pty, char* tty)
{
	static const char major[] = "pqrs";
	static const char minor[] = "0123456789abcdef";
	uint32 i, j;
	int32 fd = -1;

	for (i = 0; i < sizeof(major); i++)
	{
		for (j = 0; j < sizeof(minor); j++)
		{
			sprintf(pty, "/dev/pt/%c%c", major[i], minor[j]);
			sprintf(tty, "/dev/tt/%c%c", major[i], minor[j]);
			fd = open(pty, O_RDWR|O_NOCTTY);
			if (fd >= 0)
				return fd;
		}
	}

	return fd;
}


/* the rest is empty */


bool
SpawningUploadClient::ChangeDir(const string& dir)
{
	return false;
}


bool
SpawningUploadClient::ListDirContents(string& listing)
{
	return false;
}


bool
SpawningUploadClient::PrintWorkingDir(string& dir)
{
	return false;
}


bool
SpawningUploadClient::PutFile(const string& local, const string& remote,
	ftp_mode mode)
{
	return false;
}


bool
SpawningUploadClient::GetFile(const string& remote, const string& local,
	ftp_mode mode)
{
	return false;
}


// Note: this only works for local remote moves, cross filesystem moves
// will not work
bool
SpawningUploadClient::MoveFile(const string& oldPath, const string& newPath)
{
	return false;
}


bool
SpawningUploadClient::Chmod(const string& path, const string& mod)
{
	return false;
}


void
SpawningUploadClient::SetPassive(bool on)
{
}
