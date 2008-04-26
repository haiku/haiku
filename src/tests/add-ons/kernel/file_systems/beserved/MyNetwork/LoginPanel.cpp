#include "Mime.h"
#include "TypeConstants.h"
#include "Application.h"
#include "InterfaceDefs.h"
#include "TranslationUtils.h"
#include "Button.h"
#include "Errors.h"
#include "Window.h"
#include "TextControl.h"
#include "Roster.h"
#include "Bitmap.h"
#include "OS.h"

// POSIX includes
#include "errno.h"
#include "malloc.h"

#include "LoginPanel.h"
#include "md5.h"

// To create:
//		BRect frame2(100, 150, 350, 290);
//		LoginPanel *login = new LoginPanel(frame2);


// ----- LoginView -----------------------------------------------------

LoginView::LoginView(BRect rect)
	: BView(rect, "LoginView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(gray);

	BRect bmpRect(0.0, 0.0, 31.0, 31.0);
	icon = new BBitmap(bmpRect, B_CMAP8);
	BMimeType mime("application/x-vnd.Teldar-FileSharing");
	mime.GetIcon(icon, B_LARGE_ICON);

	BRect r(10, 52, 240, 72);
	user = new BTextControl(r, "User", "User:", "", NULL);
	user->SetDivider(55);
	AddChild(user);

	r.top = 77;
	r.bottom = r.top + 20;
	password = new BTextControl(r, "Password", "Password:", "", NULL);
	password->SetDivider(55);
	password->TextView()->HideTyping(true);
	AddChild(password);

	r.Set(90, 105, 160, 125);
	BButton *okBtn = new BButton(r, "OkayBtn", "Login", new BMessage(MSG_LOGIN_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	okBtn->MakeDefault(true);
	AddChild(okBtn);

	r.Set(170, 105, 240, 125);
	AddChild(new BButton(r, "CancelBtn", "Cancel", new BMessage(MSG_LOGIN_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));
}

LoginView::~LoginView()
{
}

void LoginView::Draw(BRect rect)
{
	BRect r = Bounds();
	BRect iconRect(13.0, 5.0, 45.0, 37.0);
	rgb_color black = { 0, 0, 0, 255 };
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(gray);
	SetLowColor(gray);
	FillRect(r, B_SOLID_LOW);

	SetHighColor(black);
	SetFont(be_bold_font);
	SetFontSize(11);
	MovePenTo(55, 15);
	DrawString("Login Required");

	SetFont(be_plain_font);
	SetFontSize(10);
	MovePenTo(55, 28);
	DrawString("The resource you've specified requires");
	MovePenTo(55, 40);
	DrawString("permission to access it.");

	SetDrawingMode(B_OP_ALPHA); 
	SetHighColor(0, 0, 0, 180);       
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
	DrawBitmap(icon, iconRect);
}


// ----- LoginPanel ----------------------------------------------------------------------

LoginPanel::LoginPanel(BRect frame)
	: BWindow(frame, "Login", B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	user[0] = password[0] = 0;
	cancelled = false;

	BRect r = Bounds();
	loginView = new LoginView(r);
	AddChild(loginView);

	Show();
}

LoginPanel::~LoginPanel()
{
}

// MessageReceived()
//
void LoginPanel::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case MSG_LOGIN_OK:
			cancelled = false;
			safeStringCopy(user, loginView->GetUser(), sizeof(user));
			safeStringCopy(password, loginView->GetPassword(), sizeof(password));
			md5EncodeString(password, md5passwd);
			BWindow::Quit();
			break;

		case MSG_LOGIN_CANCEL:
			cancelled = true;
			BWindow::Quit();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

void safeStringCopy(char *dest, const char *source, int destSize)
{
	int length = strlen(source);
	if (length >= destSize)
		length = destSize - 1;

	strncpy(dest, source, length);
	dest[length] = 0;
}
