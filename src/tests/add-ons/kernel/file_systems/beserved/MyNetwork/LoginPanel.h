#include "Window.h"
#include "View.h"
#include "TextControl.h"
#include "Button.h"
#include "Bitmap.h"

#include "betalk.h"

const uint32 MSG_LOGIN_OK = 'LgOK';
const uint32 MSG_LOGIN_CANCEL = 'LgCn';


// ----- LoginView -----------------------------------------------------

class LoginView : public BView
{
	public:
		LoginView(BRect rect);
		~LoginView();

		void Draw(BRect rect);

		const char *GetUser()		{ return user->Text(); }
		const char *GetPassword()	{ return password->Text(); }

	private:
		BBitmap *icon;
		BTextControl *user;
		BTextControl *password;
};


// ----- LoginPanel ----------------------------------------------------------------------

class LoginPanel : public BWindow
{
	public:
		LoginPanel(BRect frame);
		~LoginPanel();

		void MessageReceived(BMessage *msg);
		bool IsCancelled()		{ return cancelled; }

		char user[MAX_NAME_LENGTH];
		char password[MAX_NAME_LENGTH];
		char md5passwd[MAX_NAME_LENGTH];

		sem_id loginSem;

	private:
		LoginView *loginView;
		bool cancelled;
};


// ----- Utilities ----------------------------------------------------------------------

void safeStringCopy(char *dest, const char *source, int destSize);
