#include <ScrollView.h>

#include <pwd.h>

#include "LoginView.h"

#define CSEP 15

LoginView::LoginView(BRect frame)
	: BView(frame, "LoginView", B_FOLLOW_ALL, 0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ViewColor());
	BRect r;
	r.Set(CSEP, CSEP, 80, Bounds().Height() - 2 * CSEP);
	fUserList = new BListView(r, "users");
	BScrollView *sv = new BScrollView("userssv", fUserList);
	AddChild(sv);
	
	r.Set(100, Bounds().top + CSEP, Bounds().right - CSEP, Bounds().top + CSEP + CSEP);
	fLoginControl = new BTextControl(r, "login", "login:", "", new BMessage(kLoginEdited));
	AddChild(fLoginControl);
	r.OffsetBySelf(0, CSEP + CSEP);
	fPasswordControl = new BTextControl(r, "password", "password:", "", new BMessage(kPasswordEdited));
	fPasswordControl->TextView()->HideTyping(true);
	AddChild(fPasswordControl);
	
}


LoginView::~LoginView()
{
}

void
LoginView::AttachedToWindow()
{
	fLoginControl->MakeFocus();
	// populate user list
	BMessenger(this).SendMessage(kAddNextUser);
}


void
LoginView::MessageReceived(BMessage *message)
{
	switch (message->what) {
	case kSetProgress:
		break;
	case kAddNextUser:
		AddNextUser();
		break;
	default:
		BView::MessageReceived(message);
	}
}


void
LoginView::AddNextUser()
{
	struct passwd *pwd;
	if (fUserList->CountItems() < 1)
		setpwent();

	pwd = getpwent();

	if (pwd && pwd->pw_shell && 
		strcmp(pwd->pw_shell, "false") && 
		strcmp(pwd->pw_shell, "true") && 
		strcmp(pwd->pw_shell, "/bin/false") && 
		strcmp(pwd->pw_shell, "/bin/true")) {
		// not disabled
		BStringItem *item = new BStringItem(pwd->pw_name);
		fUserList->AddItem(item);
	}
	if (pwd)
		BMessenger(this).SendMessage(kAddNextUser);
	else
		endpwent();
}


