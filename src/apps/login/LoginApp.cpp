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

#include <LaunchRoster.h>
#include <RosterPrivate.h>
#include <shadow.h>

#include "multiuser_utils.h"

#include "LoginApp.h"
#include "LoginWindow.h"
#include "DesktopWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Login App"

const char *kLoginAppSig = "application/x-vnd.Haiku-Login";


LoginApp::LoginApp()
	:
	BApplication(kLoginAppSig),
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
		BAlert* alert = new BAlert(B_TRANSLATE("Info"), B_TRANSLATE("You can "
			"customize the desktop shown behind the Login application by "
			"dropping replicants onto it.\n\n"
			"When you are finished just quit the application (Cmd-Q)."),
			B_TRANSLATE("OK"));
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
	} else {
		float sizeDelta = (float)be_plain_font->Size()/12.0f;
		BRect frame(0, 0, 450 * sizeDelta, 150 * sizeDelta);
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
			TryLogin(message);
			// TODO
			break;
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
				BAlert* alert = new BAlert(("Error"), msg.String(),
					B_TRANSLATE("OK"));
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go();
			}
			break;
		}
		case kSuspendAction:
		{
			BAlert* alert = new BAlert(B_TRANSLATE("Error"),
				B_TRANSLATE("Unimplemented"), B_TRANSLATE("OK"));
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			alert->Go();
			break;
		}

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
			puts(B_TRANSLATE("Login application for Haiku\nUsage:"));
			printf("%s [--nonmodal] [--edit]\n", argv[0]);
			puts(B_TRANSLATE("--nonmodal	Do not make the window modal"));
			puts(B_TRANSLATE("--edit	Launch in shelf editting mode to "
				"allow customizing the desktop."));
			// just return to the shell
			exit((arg == "--help") ? 0 : 1);
			return;
		}
	}
}


void
LoginApp::TryLogin(BMessage *message)
{
	BMessage reply(kLoginBad);
	status_t status = B_BAD_VALUE;

	const char* login;
	if (message->FindString("login", &login) == B_OK) {
		const char* password = message->GetString("password");

		status = ValidateLogin(login, password);
		if (status == B_OK) {
			status = BLaunchRoster().StartSession(login);
			if (status == B_OK)
				Quit();
		}

		fprintf(stderr, "ValidateLogin: %s\n", strerror(status));
	}

	if (status == B_OK) {
		reply.what = kLoginOk;
		message->SendReply(&reply);
	} else {
		reply.AddInt32("error", status);
		message->SendReply(&reply);
	}
}


status_t
LoginApp::ValidateLogin(const char *login, const char *password)
{
	struct passwd *pwd;

	pwd = getpwnam(login);
	if (pwd == NULL)
		return ENOENT;
	if (strcmp(pwd->pw_name, login) != 0)
		return ENOENT;

	if (verify_password(pwd, getspnam(login), password))
		return B_OK;

	return B_PERMISSION_DENIED;
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
