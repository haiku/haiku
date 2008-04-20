#include <Alert.h>
#include <Screen.h>
#include <String.h>
#include <View.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include "LoginApp.h"
#include "LoginWindow.h"

#ifdef __HAIKU__
#include <RosterPrivate.h>
#include <shadow.h>
#include "multiuser_utils.h"
#endif


const char *kLoginAppSig = "application/x-vnd.Haiku-Login";

const window_feel kPrivateDesktopWindowFeel = window_feel(1024);
const window_look kPrivateDesktopWindowLook = window_look(4);
	// this is a mirror of an app server private values


LoginApp::LoginApp()
	: BApplication(kLoginAppSig)
{
}


LoginApp::~LoginApp()
{
}


void
LoginApp::ReadyToRun()
{
	BScreen s;
	BRect frame(0, 0, 400, 150);
	frame.OffsetBySelf(s.Frame().Width()/2 - frame.Width()/2, 
						s.Frame().Height()/2 - frame.Height()/2);
	fLoginWindow = new LoginWindow(frame);
	fLoginWindow->Show();
	
	BScreen screen;
	fDesktopWindow = new BWindow(screen.Frame(), "Desktop", 
		kPrivateDesktopWindowLook, 
		kPrivateDesktopWindowFeel, 
		B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE
		 | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE
		 | B_ASYNCHRONOUS_CONTROLS,
		B_ALL_WORKSPACES);
	BView *desktop = new BView(fDesktopWindow->Bounds(), "desktop", 
		B_FOLLOW_NONE, 0);
	desktop->SetViewColor(screen.DesktopColor());
	fDesktopWindow->AddChild(desktop);
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
				BString msg("Error: ");
				msg << strerror(error);
				(new BAlert("Error", msg.String(), "Ok"))->Go();
			}
			break;
		}
		case kSuspendAction:
			(new BAlert("Error", "Unimplemented", "Ok"))->Go();
			break;
#endif
	default:
		BApplication::MessageReceived(message);
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
		printf("ValidateLogin: %s\n", strerror(err));
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
LoginApp::ValidateLogin(const char *login, const char *password/*, bool force = false*/)
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



