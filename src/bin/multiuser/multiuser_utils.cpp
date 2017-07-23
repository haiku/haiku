/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#include "multiuser_utils.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <AutoDeleter.h>

#include <user_group.h>


status_t
read_password(const char* prompt, char* password, size_t bufferSize,
	bool useStdio)
{
	FILE* in = stdin;
	FILE* out = stdout;

	// open tty
	FILE* tty = NULL;
	if (!useStdio) {
// TODO: Open tty with O_NOCTTY!
		tty = fopen("/dev/tty", "w+");
		if (tty == NULL) {
			fprintf(stderr, "Error: Failed to open tty: %s\n", strerror(errno));
			return errno;
		}

		in = tty;
		out = tty;
	}
	CObjectDeleter<FILE, int> ttyCloser(tty, fclose);

	// disable echo
	int inFD = fileno(in);
	struct termios termAttrs;
	if (tcgetattr(inFD, &termAttrs) != 0) {
		fprintf(in, "Error: Failed to get tty attributes: %s\n",
			strerror(errno));
		return errno;
	}

	tcflag_t localFlags = termAttrs.c_lflag;
	termAttrs.c_lflag &= ~ECHO;

	if (tcsetattr(inFD, TCSANOW, &termAttrs) != 0) {
		fprintf(in, "Error: Failed to set tty attributes: %s\n",
			strerror(errno));
		return errno;
	}

	status_t error = B_OK;

	// prompt and read pwd
	fprintf(out, prompt);
	fflush(out);

	if (fgets(password, bufferSize, in) == NULL) {
		fprintf(out, "\nError: Failed to read from tty: %s\n", strerror(errno));
		error = errno != 0 ? errno : B_ERROR;
	} else
		fputc('\n', out);

	// chop off trailing newline
	if (error == B_OK) {
		size_t len = strlen(password);
		if (len > 0 && password[len - 1] == '\n')
			password[len - 1] = '\0';
	}

	// restore the terminal attributes
	termAttrs.c_lflag = localFlags;
	tcsetattr(inFD, TCSANOW, &termAttrs);

	return error;
}


bool
verify_password(passwd* passwd, spwd* spwd, const char* plainPassword)
{
	if (passwd == NULL)
		return false;

	// check whether we need to check the shadow password
	const char* requiredPassword = passwd->pw_passwd;
	if (strcmp(requiredPassword, "x") == 0) {
		if (spwd == NULL) {
			// Mmh, we're suppose to check the shadow password, but we don't
			// have it. Bail out.
			return false;
		}

		requiredPassword = spwd->sp_pwdp;
	}

	// If no password is required, we're done.
	if (requiredPassword == NULL || requiredPassword[0] == '\0') {
		if (plainPassword == NULL || plainPassword[0] == '\0')
			return true;

		return false;
	}

	// crypt and check it
	char* encryptedPassword = crypt(plainPassword, requiredPassword);

	return (strcmp(encryptedPassword, requiredPassword) == 0);
}


/*!	Checks whether the user needs to authenticate with a password, and, if
	necessary, asks for it, and checks it.
	\a passwd must always be given, \a spwd only if there exists an entry
	for the user.
*/
status_t
authenticate_user(const char* prompt, passwd* passwd, spwd* spwd, int maxTries,
	bool useStdio)
{
	// check whether a password is need at all
	if (verify_password(passwd, spwd, ""))
		return B_OK;

	while (true) {
		// prompt the user for the password
		char plainPassword[MAX_SHADOW_PWD_PASSWORD_LEN];
		status_t error = read_password(prompt, plainPassword,
			sizeof(plainPassword), useStdio);
		if (error != B_OK)
			return error;

		// check it
		bool ok = verify_password(passwd, spwd, plainPassword);
		memset(plainPassword, 0, sizeof(plainPassword));
		if (ok)
			return B_OK;

		fprintf(stderr, "Incorrect password.\n");
		if (--maxTries <= 0)
			return B_PERMISSION_DENIED;
	}
}


status_t
authenticate_user(const char* prompt, const char* user, passwd** _passwd,
	spwd** _spwd, int maxTries, bool useStdio)
{
	struct passwd* passwd = getpwnam(user);
	struct spwd* spwd = getspnam(user);

	status_t error = authenticate_user(prompt, passwd, spwd, maxTries,
		useStdio);
	if (error == B_OK) {
		if (_passwd)
			*_passwd = passwd;
		if (_spwd)
			*_spwd = spwd;
	}

	return error;
}


status_t
setup_environment(struct passwd* passwd, bool preserveEnvironment, bool chngdir)
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

	if (passwd->pw_gid && setgid(passwd->pw_gid) != 0)
		return errno;

	if (passwd->pw_uid && setuid(passwd->pw_uid) != 0)
		return errno;

	if (chngdir) {
		const char* home = getenv("HOME");
		if (home == NULL)
			return B_ENTRY_NOT_FOUND;

		if (chdir(home) != 0)
			return errno;
	}

	return B_OK;
}
