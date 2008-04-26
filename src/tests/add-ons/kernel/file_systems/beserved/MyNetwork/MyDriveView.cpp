#include "Mime.h"
#include "TypeConstants.h"
#include "Application.h"
#include "InterfaceDefs.h"
#include "TranslationUtils.h"
#include "StatusBar.h"
#include "Button.h"
#include "Alert.h"
#include "Errors.h"
#include "OS.h"

// POSIX includes
#include "errno.h"
#include "malloc.h"
#include "socket.h"
#include "signal.h"
#include "netdb.h"

#include "IconListItem.h"
#include "MyDriveView.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET		(int)(~0)
#endif

#define BTN_MOUNT_MSG		'bmnt'
#define BTN_INCREASE_MSG	'binc'
#define BTN_SETTINGS_MSG	'bset'


MyDriveView::MyDriveView(BRect rect)
	: BView(rect, "MyDriveView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(gray);

	// Here we're going to get the mini icon from a specific mime type
//	BRect bmpRect(0.0, 0.0, 15.0, 15.0);
//	icon = new BBitmap(bmpRect, B_CMAP8);
	SetDriveSpace(0, 0, 0);

	BRect progRect(5.0, 30.0, 225.0, 42.0);
	barFull = new BStatusBar(progRect, "barFull");
	barFull->SetBarHeight(13.0);
	AddChild(barFull);

	BRect mntRect(5.0, 65.0, 70.0, 80.0);
	BButton *btnMount = new BButton(mntRect, "btnMount", "Mount", new BMessage(BTN_MOUNT_MSG));
	AddChild(btnMount);

	BRect incRect(75.0, 65.0, 190.0, 80.0);
	BButton *btnIncrease = new BButton(incRect, "btnIncrease", "Increase Space", new BMessage(BTN_INCREASE_MSG));
	AddChild(btnIncrease);
}

MyDriveView::~MyDriveView()
{
}

void MyDriveView::Draw(BRect rect)
{
	rgb_color black = { 0, 0, 0, 255 };
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(gray);
	SetHighColor(black);

	SetFont(be_bold_font);
	SetFontSize(10);
	MoveTo(5, 10);
	DrawString("Online Storage Details");

	SetFont(be_plain_font);
	MoveTo(5, 20);
//	DrawString(msg);
}

void MyDriveView::SetDriveSpace(int percentFull, int files, int folders)
{
//	barFull->Update(percentFull);
//	sprintf(msg, "Your online storage is %d%% full, containing %d files in %d folders",
//		percentFull, files, folders);
}
