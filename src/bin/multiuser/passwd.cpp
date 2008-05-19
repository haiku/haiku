/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <pwd.h>
#include <shadow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <OS.h>

#include <RegistrarDefs.h>
#include <user_group.h>
#include <util/KMessage.h>

#include <AutoDeleter.h>

#include "multiuser_utils.h"


extern const char *__progname;


static const char* kUsage =
"Usage: %s [ <user name> ]\n"
;

static void
print_usage_and_exit(bool error)
{
	fprintf(error ? stderr : stdout, kUsage, __progname);
	exit(error ? 1 : 0);
}


int
main(int argc, const char* const* argv)
{
	if (argc > 2)
		print_usage_and_exit(true);

	const char* user = NULL;
	if (argc == 2)
		user = argv[1];

	if (geteuid() != 0) {
		fprintf(stderr, "Error: You need to be root.\n");
		exit(1);
	}

	// this is a set-uid tool -- get the real UID
	uid_t uid = getuid();

	// get the passwd entry
	struct passwd* passwd;
	if (user != NULL) {
		passwd = getpwnam(user);
		if (passwd == NULL) {
			fprintf(stderr, "Error: No user with name \"%s\".\n", user);
			exit(1);
		}

		if (uid != 0 && passwd->pw_uid != uid) {
			fprintf(stderr, "Error: Only root can change the passwd for other "
				"users.\n");
			exit(1);
		}
	} else {
		passwd = getpwuid(uid);
		if (passwd == NULL) {
			fprintf(stderr, "Error: Ugh! Couldn't get passwd entry for uid "
				"%d.\n", uid);
			exit(1);
		}

		user = passwd->pw_name;
	}

	// if not root, the user needs to authenticate
	if (uid != 0) {
		if (authenticate_user("old password: ", passwd, getspnam(user), 1,
				false) != B_OK) {
			exit(1);
		}
	}

	// read new password
	char password[LINE_MAX];
	if (read_password("new password: ", password, sizeof(password), false)
			!= B_OK) {
		exit(1);
	}

	if (strlen(password) >= MAX_SHADOW_PWD_PASSWORD_LEN) {
		fprintf(stderr, "Error: The password is too long.\n");
		exit(1);
	}

	// read password again
	char repeatedPassword[LINE_MAX];
	if (read_password("repeat new password: ", repeatedPassword,
			sizeof(repeatedPassword), false) != B_OK) {
		exit(1);
	}

	// passwords need to match
	if (strcmp(password, repeatedPassword) != 0) {
		fprintf(stderr, "Error: passwords don't match\n");
		exit(1);
	}

	memset(repeatedPassword, 0, sizeof(repeatedPassword));

	// crypt it
	char* encryptedPassword;
	if (strlen(password) > 0) {
		encryptedPassword = crypt(password, user);
		memset(password, 0, sizeof(password));
	} else
		encryptedPassword = password;

	// prepare request for the registrar
	KMessage message(BPrivate::B_REG_UPDATE_USER);
	if (message.AddInt32("uid", passwd->pw_uid) != B_OK
		|| message.AddInt32("last changed", time(NULL)) != B_OK
		|| message.AddString("password", "x") != B_OK
		|| message.AddString("shadow password", encryptedPassword) != B_OK) {
		fprintf(stderr, "Error: Out of memory!\n");
		exit(1);
	}

	// send the request
	KMessage reply;
	status_t error = send_authentication_request_to_registrar(message, reply);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to create user: %s\n", strerror(error));
		exit(1);
	}

	return 0;
}
