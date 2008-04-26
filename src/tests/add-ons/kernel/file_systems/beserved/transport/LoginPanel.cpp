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
#include "Screen.h"
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

LoginView::LoginView(BRect rect, char *server, char *share) :
  BView(rect, "LoginView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	strcpy(resource, share);
	strcat(resource, " on ");
	strcat(resource, server);

	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(gray);

	BRect bmpRect(0.0, 0.0, 31.0, 31.0);
	icon = new BBitmap(bmpRect, B_CMAP8);
	BMimeType mime("application/x-vnd.Teldar-FileSharing");
	mime.GetIcon(icon, B_LARGE_ICON);

	BRect r(10, 72, 240, 92);
	user = new BTextControl(r, "User", "User:", "", NULL);
	user->SetDivider(55);
	AddChild(user);

	r.top = 97;
	r.bottom = r.top + 20;
	password = new BTextControl(r, "Password", "Password:", "", NULL);
	password->SetDivider(55);
	password->TextView()->HideTyping(true);
	AddChild(password);

	r.Set(LOGIN_PANEL_WIDTH - 160, LOGIN_PANEL_HEIGHT - 33, LOGIN_PANEL_WIDTH - 90, LOGIN_PANEL_HEIGHT - 13);
	BButton *okBtn = new BButton(r, "OkayBtn", "Login", new BMessage(MSG_LOGIN_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	okBtn->MakeDefault(true);
	AddChild(okBtn);

	r.Set(LOGIN_PANEL_WIDTH - 80, LOGIN_PANEL_HEIGHT - 33, LOGIN_PANEL_WIDTH - 10, LOGIN_PANEL_HEIGHT - 13);
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
	DrawString("The print server has rejected your user name and");
	MovePenTo(55, 40);
	DrawString("password.  Please re-enter your user credentials.");

	MovePenTo(13, 61);
	DrawString("Printer:");
	MovePenTo(70, 61);
	DrawString(resource);

	SetDrawingMode(B_OP_ALPHA); 
	SetHighColor(0, 0, 0, 180);       
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
	DrawBitmap(icon, iconRect);
}


// ----- LoginPanel ----------------------------------------------------------------------

LoginPanel::LoginPanel(BRect frame, char *server, char *share, bool show) :
  BWindow(frame, "Login", B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	user[0] = password[0] = 0;
	cancelled = false;

	BRect r = Bounds();
	loginView = new LoginView(r, server, share);
	AddChild(loginView);

	if (show)
		Show();
}

LoginPanel::~LoginPanel()
{
}

// Center()
//
void LoginPanel::Center()
{
	BScreen screen(this);
	BRect screenFrame = screen.Frame();
	BRect winFrame;
	winFrame.left = (screenFrame.Width() - 300) / 2;
	winFrame.top = (screenFrame.Height() - 158) / 2;
	winFrame.right = winFrame.left + 300;
	winFrame.bottom = winFrame.top + 158;
	MoveTo(winFrame.left, winFrame.top);
	ResizeTo(300, 158);

	Show();
}

// MessageReceived()
//
void LoginPanel::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case MSG_LOGIN_OK:
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
