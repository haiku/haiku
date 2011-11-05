/*
 * Copyright 2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <Catalog.h>
#include <ScrollView.h>
#include <String.h>
#include <Window.h>
#ifdef __HAIKU__
#include <Layout.h>
#endif

#include <pwd.h>

#include "LoginApp.h"
#include "LoginView.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Login View"

#define LW 120
#define CSEP 15
#define BH 20
#define BW 60

class PwdItem : public BStringItem {
public:
					PwdItem(struct passwd *pwd, uint32 level = 0,
						bool expanded = true)
						 : BStringItem("", level, expanded)
						{
							if (pwd) {
								BString name(pwd->pw_gecos);
								// TODO: truncate at first ;
								fLogin = pwd->pw_name;
								SetText(name.String());
							}
						};
	virtual			~PwdItem() {};
	const char*		Login() const { return fLogin.String(); };
private:
	BString			fLogin;
};


LoginView::LoginView(BRect frame)
	: BView(frame, "LoginView", B_FOLLOW_ALL, B_PULSE_NEEDED)
{
	// TODO: when I don't need to test in BeOS anymore,
	// rewrite to use layout engine.
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ViewColor());
	BRect r;
	r.Set(CSEP, CSEP, LW, Bounds().Height() - 3 * CSEP - BH);
	fUserList = new BListView(r, "users");
	BScrollView *sv = new BScrollView("userssv", fUserList,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true);
	AddChild(sv);
	fUserList->SetSelectionMessage(new BMessage(kUserSelected));
	fUserList->SetInvocationMessage(new BMessage(kUserInvoked));

	r.Set(LW + 30, Bounds().top + CSEP,
		Bounds().right - CSEP, Bounds().top + CSEP + CSEP);
	fLoginControl = new BTextControl(r, "login", B_TRANSLATE("Login:"), "",
		new BMessage(kLoginEdited));
	AddChild(fLoginControl);

	r.OffsetBySelf(0, CSEP + CSEP);
	fPasswordControl = new BTextControl(r, "password",
		B_TRANSLATE("Password:"), "", new BMessage(kPasswordEdited));
	fPasswordControl->TextView()->HideTyping(true);
	AddChild(fPasswordControl);

	r.OffsetBySelf(0, CSEP + CSEP);
	fHidePasswordCheckBox = new BCheckBox(r, "hidepw",
		B_TRANSLATE("Hide password"), new BMessage(kHidePassword));
	fHidePasswordCheckBox->SetValue(1);
	AddChild(fHidePasswordCheckBox);

	// buttons
	float buttonWidth = BW; //(Bounds().Width() - 4 * CSEP) / 3;
	BRect buttonRect(0, Bounds().bottom - BH,
		buttonWidth, Bounds().bottom);
	buttonRect.OffsetBySelf(CSEP, -CSEP);

	fHaltButton = new BButton(buttonRect, "halt", B_TRANSLATE("Halt"),
		new BMessage(kHaltAction));
	AddChild(fHaltButton);

	buttonRect.OffsetBySelf(CSEP + buttonWidth, 0);
	fRebootButton = new BButton(buttonRect, "reboot", B_TRANSLATE("Reboot"),
		new BMessage(kRebootAction));
	AddChild(fRebootButton);

	BRect infoRect(buttonRect);
	infoRect.OffsetBySelf(buttonWidth + CSEP, 0);

	buttonRect.OffsetToSelf(Bounds().Width() - CSEP - buttonWidth,
		Bounds().Height() - CSEP - BH);
	fLoginButton = new BButton(buttonRect, "ok", B_TRANSLATE("OK"),
		new BMessage(kAttemptLogin));
	AddChild(fLoginButton);

	infoRect.right = buttonRect.left - CSEP + 5;
	BString info;
	//info << hostname;
	fInfoView = new BStringView(infoRect, "info", info.String());
	AddChild(fInfoView);
}


LoginView::~LoginView()
{
}

void
LoginView::AttachedToWindow()
{
	fUserList->SetTarget(this);
	fLoginControl->SetTarget(this);
	fPasswordControl->SetTarget(this);
	fHidePasswordCheckBox->SetTarget(this);
	fHaltButton->SetTarget(be_app_messenger);
	fRebootButton->SetTarget(be_app_messenger);
	fLoginButton->SetTarget(this);
	Window()->SetDefaultButton(fLoginButton);
	//fLoginControl->MakeFocus();
	fUserList->MakeFocus();
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
		case kUserSelected:
		{
			int32 selection = fUserList->CurrentSelection();
			if (selection > -1) {
				PwdItem *item = dynamic_cast<PwdItem *>(
					fUserList->ItemAt(selection));
				if (item)
					fLoginControl->SetText(item->Login());
			}
			break;
		}
		case kUserInvoked:
			fPasswordControl->MakeFocus();
			break;
		case kLoginEdited:
			break;
		case kHidePassword:
			fPasswordControl->TextView()->HideTyping(
				fHidePasswordCheckBox->Value());
			break;
		case kAttemptLogin:
		{
			// if no pass specified and we were selecting the user,
			// give a chance to enter the password
			// else we might want to enter an empty password.
			if (strlen(fPasswordControl->Text()) < 1
				&& (fUserList->IsFocus() || fLoginControl->IsFocus())) {
				fPasswordControl->MakeFocus();
				break;
			}
			BMessage *m = new BMessage(kAttemptLogin);
			m->AddString("login", fLoginControl->Text());
			m->AddString("password", fPasswordControl->Text());
			be_app->PostMessage(m, NULL, this);
			break;
		}
		case kLoginBad:
			fPasswordControl->SetText("");
			EnableControls(false);
			fInfoView->SetText(B_TRANSLATE("Invalid login!"));
			if (Window()) {
				BPoint savedPos = Window()->Frame().LeftTop();
				for (int i = 0; i < 10; i++) {
					BPoint p(savedPos);
					p.x += (i%2) ? 10 : -10;
					Window()->MoveTo(p);
					Window()->UpdateIfNeeded();
					snooze(30000);
				}
				Window()->MoveTo(savedPos);
			}
			EnableControls(true);
			break;
		case kLoginOk:
			// XXX: quit ?
			if (Window()) {
				Window()->Hide();
			}
			break;
		default:
			message->PrintToStream();
			BView::MessageReceived(message);
	}
}


void
LoginView::Pulse()
{
	BString info;
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	// TODO: use strftime and locale settings
	info << asctime(t);
	info.RemoveSet("\r\n");
	fInfoView->SetText(info.String());
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
		PwdItem *item = new PwdItem(pwd);
		fUserList->AddItem(item);
	}
	if (pwd)
		BMessenger(this).SendMessage(kAddNextUser);
	else
		endpwent();
}


void
LoginView::EnableControls(bool enable)
{
	int32 i;
	for (i = 0; i < fUserList->CountItems(); i++) {
		fUserList->ItemAt(i)->SetEnabled(enable);
	}
	fLoginControl->SetEnabled(enable);
	fPasswordControl->SetEnabled(enable);
	fHidePasswordCheckBox->SetEnabled(enable);
	fHaltButton->SetEnabled(enable);
	fRebootButton->SetEnabled(enable);
	fLoginButton->SetEnabled(enable);
}

