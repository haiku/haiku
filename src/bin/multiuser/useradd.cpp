/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All Rights Reserved.
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
#include <parsedate.h>

#include <RegistrarDefs.h>
#include <user_group.h>
#include <util/KMessage.h>

#include <AutoDeleter.h>

#include "multiuser_utils.h"


extern const char *__progname;


static const char* kUsage =
"Usage: %s [ -d <home> ] [ -e <expiration> ] [ -f <inactive> ] [ -g <gid> ]\n"
"          [ -s <shell> ] [ -n <real name> ]\n"
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
	const char* home = "/boot/home";
	int expiration = 99999;
	int inactive = -1;
	gid_t gid = 100;
	const char* shell = "/bin/sh";
	const char* realName = "";

	int min = -1;
	int max = -1;
	int warn = 7;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "d:e:f:g:hn:s:", sLongOptions,
			NULL);
		if (c == -1)
			break;

	
		switch (c) {
			case 'd':
				home = optarg;
				break;

			case 'e':
				expiration = parsedate(optarg, time(NULL)) / (3600 * 24);
				break;

			case 'f':
				inactive = atoi(optarg);
				break;

			case 'g':
				gid = atoi(optarg);
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			case 'n':
				realName = optarg;
				break;

			case 's':
				shell = optarg;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	if (optind != argc - 1)
		print_usage_and_exit(true);

	const char* user = argv[optind];

	if (geteuid() != 0) {
		fprintf(stderr, "Error: You need to be root.\n");
		exit(1);
	}

	// check, if user already exists
	if (getpwnam(user) != NULL) {
		fprintf(stderr, "Error: User \"%s\" already exists.\n", user);
		exit(1);
	}

	// read password
	char password[LINE_MAX];
	if (read_password("password for user: ", password, sizeof(password),
			false) != B_OK) {
		exit(1);
	}

	if (strlen(password) >= MAX_SHADOW_PWD_PASSWORD_LEN) {
		fprintf(stderr, "Error: The password is too long.\n");
		exit(1);
	}

	// read password again
	char repeatedPassword[LINE_MAX];
	if (read_password("repeat password: ", repeatedPassword,
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

	// find an unused UID
	uid_t uid = 1000;
	while (getpwuid(uid) != NULL)
		uid++;

	// prepare request for the registrar
	KMessage message(BPrivate::B_REG_UPDATE_USER);
	if (message.AddInt32("uid", uid) != B_OK
		|| message.AddInt32("gid", gid) != B_OK
		|| message.AddString("name", user) != B_OK
		|| message.AddString("password", "x") != B_OK
		|| message.AddString("home", home) != B_OK
		|| message.AddString("shell", shell) != B_OK
		|| message.AddString("real name", realName) != B_OK
		|| message.AddString("shadow password", encryptedPassword) != B_OK
		|| message.AddInt32("last changed", time(NULL)) != B_OK
		|| message.AddInt32("min", min) != B_OK
		|| message.AddInt32("max", max) != B_OK
		|| message.AddInt32("warn", warn) != B_OK
		|| message.AddInt32("inactive", inactive) != B_OK
		|| message.AddInt32("expiration", expiration) != B_OK
		|| message.AddInt32("flags", 0) != B_OK
		|| message.AddBool("add user", true) != B_OK) {
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
