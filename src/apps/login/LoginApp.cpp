/*
 * Copyright 2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Alert.h>
#include <Catalog.h>
#include <Screen.h>
#include <String.h>
#include <View.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include "LoginApp.h"
#include "LoginWindow.h"
#include "DesktopWindow.h"

#ifdef __HAIKU__
#include <RosterPrivate.h>
#include <shadow.h>
#include "multiuser_utils.h"
#endif

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Login App"

const char *kLoginAppSig = "application/x-vnd.Haiku-Login";


LoginApp::LoginApp()
	: BApplication(kLoginAppSig),
	  fEditShelfMode(false),
	  fModalMode(true)
{
}


LoginApp::~LoginApp()
{
}


void
LoginApp::ReadyToRun()
{
	BScreen screen;

	if (fEditShelfMode) {
		(new BAlert(B_TRANSLATE("Info"), B_TRANSLATE("You can customize the "
			"desktop shown behind the Login application by dropping replicants"
			" onto it.\n"
			"\n"
			"When you are finished just quit the application (Alt-Q)."),
			B_TRANSLATE("OK")))->Go(NULL);
	} else {
		BRect frame(0, 0, 450, 150);
		frame.OffsetBySelf(screen.Frame().Width()/2 - frame.Width()/2,
			screen.Frame().Height()/2 - frame.Height()/2);
		fLoginWindow = new LoginWindow(frame);
		fLoginWindow->Show();
	}

	fDesktopWindow = new DesktopWindow(screen.Frame(), fEditShelfMode);
	fDesktopWindow->Show();
	// TODO: add a shelf with Activity Monitor replicant :)
}


void
LoginApp::MessageReceived(BMessage *message)
{
	bool reboot = false;

	switch (message->what) {
		case kAttemptLogin:
			message->PrintToStream();
			TryLogin(message);
			// TODO
			break;
#ifdef __HAIKU__
		case kHaltAction:
			reboot = false;
			// FALLTHROUGH
		case kRebootAction:
		{
			BRoster roster;
			BRoster::Private rosterPrivate(roster);
			status_t error = rosterPrivate.ShutDown(reboot, false, false);
			if (error < B_OK) {
				BString msg(B_TRANSLATE("Error: %1"));
				msg.ReplaceFirst("%1", strerror(error));
				(new BAlert(("Error"), msg.String(), B_TRANSLATE("OK")))->Go();
			}
			break;
		}
		case kSuspendAction:
			(new BAlert(B_TRANSLATE("Error"), B_TRANSLATE("Unimplemented"),
				B_TRANSLATE("OK")))->Go();
			break;
#endif
	default:
		BApplication::MessageReceived(message);
	}
}


void
LoginApp::ArgvReceived(int32 argc, char **argv)
{
	for (int i = 1; i < argc; i++) {
		BString arg(argv[i]);
		//printf("[%d]: %s\n", i, argv[i]);
		if (arg == "--edit")
			fEditShelfMode = true;
		else if (arg == "--nonmodal")
			fModalMode = false;
		else /*if (arg == "--help")*/ {
			printf(B_TRANSLATE("Login application for Haiku\nUsage:\n"));
			printf("%s [--nonmodal] [--edit]\n", argv[0]);
			printf(B_TRANSLATE("--nonmodal	Do not make the window modal\n"));
			printf(B_TRANSLATE("--edit	Launch in shelf editting mode to "
				"allow customizing the desktop.\n"));
			// just return to the shell
			exit((arg == "--help") ? 0 : 1);
			return;
		}
	}
}


void
LoginApp::TryLogin(BMessage *message)
{
	status_t err;
	const char *login;
	const char *password;
	BMessage reply(kLoginBad);
	if (message->FindString("login", &login) == B_OK) {
		if (message->FindString("password", &password) < B_OK)
			password = NULL;
		err = ValidateLogin(login, password);
		printf(B_TRANSLATE_COMMENT("ValidateLogin: %s\n",
			"A message returned from the ValidateLogin function. "
			"It can be \"B_OK\"."), strerror(err));
		if (err == B_OK) {
			reply.what = kLoginOk;
			message->SendReply(&reply);

			if (password == NULL)
				return;

			// start a session
			//kSetProgress
			StartUserSession(login);
		} else {
			reply.AddInt32("error", err);
			message->SendReply(&reply);
			return;
		}

	} else {
		reply.AddInt32("error", EINVAL);
		message->SendReply(&reply);
		return;
	}
}


status_t
LoginApp::ValidateLogin(const char *login, const char *password)
{
	struct passwd *pwd;

	pwd = getpwnam(login);
	if (!pwd)
		return ENOENT;
	if (strcmp(pwd->pw_name, login))
		return ENOENT;

	if (password == NULL) {
		// we only want to check is login exists.
		return B_OK;
	}

#ifdef __HAIKU__
	if (verify_password(pwd, getspnam(login), password))
		return B_OK;
#else
	// for testing
	if (strcmp(crypt(password, pwd->pw_passwd), pwd->pw_passwd) == 0)
		return B_OK;
#endif

	return B_PERMISSION_DENIED;
}


status_t
LoginApp::StartUserSession(const char *login)
{
	return B_ERROR;
}


int
LoginApp::getpty(char *pty, char *tty)
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
			{
				return fd;
			}
		}
	}

	return fd;
}
