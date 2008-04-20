#include <Screen.h>

#include <stdio.h>
#include <unistd.h>
#include <pwd.h>

#include "LoginApp.h"
#include "LoginWindow.h"

#ifdef __HAIKU__
#include <shadow.h>
#include "multiuser_utils.h"
#endif


const char *kLoginAppSig = "application/x-vnd.Haiku-Login";


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
	BRect frame(0, 0, 300, 100);
	frame.OffsetBySelf(s.Frame().Width()/2 - frame.Width()/2, 
						s.Frame().Height()/2 - frame.Height()/2);
	LoginWindow *w = new LoginWindow(frame);
	w->Show();
}


void
LoginApp::MessageReceived(BMessage *message)
{
	switch (message->what) {
	case kAttemptLogin:
		break;
	default:
		BApplication::MessageReceived(message);
	}
}

	void TryLogin(BMessage *message);
	status_t ValidateLogin(const char *login, const char *password/*, bool force = false*/);

void
LoginApp::TryLogin(BMessage *message)
{
	status_t err;
	const char *login;
	const char *password;
	BMessage reply(B_REPLY);
	if (message->FindString("login", &login) == B_OK) {
		if (message->FindString("password", &password) < B_OK)
			password = NULL;
		err = ValidateLogin(login, password);
		if (err == B_OK) {
			reply.AddInt32("error", B_OK);
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
	if (strcmp(crypt(password, pwd->pw_passwd), pwd->pw_passwd))
		return B_NOT_ALLOWED;
#endif

	return B_NOT_ALLOWED;
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



