/*
 * Copyright 2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LOGINAPP_H_
#define _LOGINAPP_H_

#include <Application.h>

/* try loging in a user */
const uint32 kAttemptLogin = 'logi';
const uint32 kHaltAction = 'halt';
const uint32 kRebootAction = 'rebo';
const uint32 kSuspendAction = 'susp';
const uint32 kLoginBad = 'lgba';
const uint32 kLoginOk = 'lgok';

class LoginWindow;
class DesktopWindow;

class LoginApp : public BApplication {
public:
					LoginApp();
	virtual			~LoginApp();
	void			ReadyToRun();
	void			MessageReceived(BMessage *message);
	void			ArgvReceived(int32 argc, char **argv);

private:
	void			TryLogin(BMessage *message);
	status_t		ValidateLogin(const char *login, const char *password);
	status_t		StartUserSession(const char *login);
	int				getpty(char *pty, char *tty);
	
	DesktopWindow*	fDesktopWindow;
	LoginWindow*	fLoginWindow;
	bool			fEditShelfMode;
	bool			fModalMode;
};

#endif	// _LOGINAPP_H_
