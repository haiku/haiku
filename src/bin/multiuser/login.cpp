/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <SupportDefs.h>

#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>

#include "multiuser_utils.h"


extern const char* __progname;
const char* kProgramName = __progname;

const uint32 kRetries = 3;
const uint32 kTimeout = 60;


static status_t
set_tty_echo(bool enabled)
{
	struct termios termios;

	if (ioctl(STDIN_FILENO, TCGETA, &termios) != 0)
		return errno;

	// do we have to change the current setting at all?
	if (enabled == ((termios.c_lflag & ECHO) != 0))
		return B_OK;

	if (enabled)
		termios.c_lflag |= ECHO;
	else
		termios.c_lflag &= ~ECHO;

	if (ioctl(STDIN_FILENO, TCSETA, &termios) != 0)
		return errno;

	return B_OK;
}


static status_t
read_string(char* string, size_t bufferSize)
{
	// TODO: setup timeout handler

	// read everything until the next carriage return
	char c;
	while ((c = fgetc(stdin)) != EOF && c != '\r' && c != '\n') {
		if (bufferSize > 1) {
			string[0] = c;
			string++;
			bufferSize--; 
		}
	}

	if (ferror(stdin) != 0)
		return ferror(stdin);

	string[0] = '\0';
	return B_OK;
}


static status_t
login(const char* user, struct passwd** _passwd)
{
	char userBuffer[64];

	if (user == NULL) {
		char host[64];
		if (gethostname(host, sizeof(host)) != 0)
			host[0] = '\0';

		if (host[0])
			printf("%s ", host);
		printf("login: ");
		fflush(stdout);

		set_tty_echo(true);

		status_t status = read_string(userBuffer, sizeof(userBuffer));
		if (status < B_OK)
			return status;

		putchar('\n');
		user = userBuffer;
	}

	// if no user is given, we exit immediately
	if (!user[0])
		exit(1);

	printf("password: ");
	fflush(stdout);

	set_tty_echo(false);

	char password[64];
	status_t status = read_string(password, sizeof(password));

	set_tty_echo(true);
	putchar('\n');

	if (status < B_OK)
		return status;

	struct passwd* passwd = getpwnam(user);
	struct spwd* spwd = getspnam(user);

	bool ok = verify_password(passwd, spwd, password);
	memset(password, 0, sizeof(password));

	if (!ok)
		return B_PERMISSION_DENIED;

	*_passwd = passwd;
	return B_OK;
}


static status_t
setup_environment(struct passwd* passwd, bool preserveEnvironment)
{
	const char* term = getenv("TERM");
	if (!preserveEnvironment) {
		static char *empty[1];
		environ = empty;
	}

	// always preserve $TERM
	if (term != NULL)
		setenv("TERM", term, false);
	if (passwd->pw_shell)
		setenv("SHELL", passwd->pw_shell, true);
	if (passwd->pw_dir)
		setenv("HOME", passwd->pw_dir, true);

	setenv("USER", passwd->pw_name, true);

	pid_t pid = getpid();
	if (ioctl(STDIN_FILENO, TIOCSPGRP, &pid) != 0)
		return errno;

	const char* home = getenv("HOME");
	if (home == NULL)
		return B_ENTRY_NOT_FOUND;

	if (chdir(home) != 0)
		return errno;

	return B_OK;
}


static const char*
get_from(const char* host)
{
	if (host == NULL)
		return "";

	static char buffer[64];
	snprintf(buffer, sizeof(buffer), " from %s", host);
	return buffer;
}


static void
usage()
{
	fprintf(stderr, "usage: %s [-fp] [-h hostname] [username]\n", kProgramName);
	exit(1);
}


int
main(int argc, char *argv[])
{
	bool noAuthentification = false;
	bool preserveEnvironment = false;
	const char* fromHost = NULL;

	char c;
	while ((c = getopt(argc, argv, "fh:p")) != -1) {
		switch (c) {
			case 'f':
				noAuthentification = true;
				break;
			case 'h':
				if (geteuid() != 0) {
					fprintf(stderr, "%s: %s\n", kProgramName,
						strerror(B_NOT_ALLOWED));
					exit(1);
				}

				fromHost = optarg;
				break;
			case 'p':
				preserveEnvironment = true;
				break;

			default:
				usage();
				break;
		}
	}

	argc -= optind;
	argv += optind;

	const char* user = NULL;
	if (argc > 0)
		user = argv[0];

	// login

	alarm(kTimeout);
	openlog(kProgramName, 0, LOG_AUTH);

	uint32 retries = kRetries;
	status_t status = B_ERROR;
	struct passwd* passwd = NULL;

	while (retries > 0) {
		status = login(user, &passwd);
		if (status == B_OK)
			break;

		fprintf(stderr, "Login failed.\n");
		sleep(1);

		user = NULL;
			// ask for the user name as well after the first failure
	}

	alarm(0);

	if (status < B_OK) {
		// login failure
		syslog(LOG_NOTICE, "login%s failed for \"%s\"", get_from(fromHost),
			passwd->pw_name);
		exit(1);
	}

	// setup environment for the user

	status = setup_environment(passwd, preserveEnvironment);
	if (status < B_OK) {
		// refused login
		fprintf(stderr, "%s: Refused login. Setting up environment failed: %s\n",
			kProgramName, strerror(status));
		syslog(LOG_NOTICE, "login%s refused for \"%s\"", get_from(fromHost),
			passwd->pw_name);
		exit(1);
	}

	syslog(LOG_INFO, "login%s as \"%s\"", get_from(fromHost), passwd->pw_name);

	// start login shell

	const char* args[] = {getenv("SHELL"), "-login", NULL};
	execv(args[0], (char **)args);

	// try default shell
	args[0] = "/bin/sh";
	execv(args[0], (char **)args);

	fprintf(stderr, "%s: starting the shell failed: %s", kProgramName,
		strerror(errno));

	return 1;
}

