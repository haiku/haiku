/*
 * Copyright 2008-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <getopt.h>
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
	"Usage: %s [ <options> ] [ <user name> ]\n"
	"Change the password of the specified user.\n"
	"\n"
	"Options:\n"
	"  -d\n"
	"    Delete the password for the specified user.\n"
	"  -h, --help\n"
	"    Print usage info.\n"
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
	bool deletePassword = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "dh", sLongOptions, NULL);
		if (c == -1)
			break;


		switch (c) {
			case 'd':
				deletePassword = true;
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	if (optind + 1 < argc)
		print_usage_and_exit(true);

	const char* user = optind < argc ? argv[optind] : NULL;

	if (geteuid() != 0) {
		fprintf(stderr, "Error: You need to be root.\n");
		exit(1);
	}

	// this is a set-uid tool -- get the real UID
	uid_t uid = getuid();

	if (deletePassword) {
		if (uid != 0) {
			fprintf(stderr, "Error: Only root can delete users' passwords.\n");
			exit(1);
		}

		if (user == NULL) {
			fprintf(stderr, "Error: A user must be specified.\n");
			exit(1);
		}
	}

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

	char password[LINE_MAX];
	char* encryptedPassword;

	if (deletePassword) {
		password[0] = '\0';
		encryptedPassword = password;
	} else {
		// read new password
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

		explicit_bzero(repeatedPassword, sizeof(repeatedPassword));

		// crypt it
		encryptedPassword = crypt(password, NULL);
		explicit_bzero(password, sizeof(password));
	}

	// prepare request for the registrar
	KMessage message(BPrivate::B_REG_UPDATE_USER);
	if (message.AddInt32("uid", passwd->pw_uid) != B_OK
		|| message.AddInt32("last changed", time(NULL)) != B_OK
		|| message.AddString("password", "x") != B_OK
		|| message.AddString("shadow password", encryptedPassword) != B_OK) {
		fprintf(stderr, "Error: Failed to construct message!\n");
		exit(1);
	}

	// send the request
	KMessage reply;
	status_t error = send_authentication_request_to_registrar(message, reply);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to set the password: %s\n",
			strerror(error));
		exit(1);
	}

	return 0;
}
