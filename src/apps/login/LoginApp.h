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

class LoginApp : public BApplication {
public:
	LoginApp();
	virtual ~LoginApp();
	void ReadyToRun();
	void MessageReceived(BMessage *message);

private:
	void TryLogin(BMessage *message);
	status_t ValidateLogin(const char *login, const char *password/*, bool force = false*/);
	status_t StartUserSession(const char *login);
	int getpty(char *pty, char *tty);

};

#endif	// _LOGINAPP_H_
