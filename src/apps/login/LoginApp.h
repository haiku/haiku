#include <Application.h>

/* try loging in a user */
const uint32 kAttemptLogin = 'logi';

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
