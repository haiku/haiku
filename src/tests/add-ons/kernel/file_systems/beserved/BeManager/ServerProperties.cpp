#include "Application.h"
#include "Resources.h"
#include "Window.h"
#include "View.h"
#include "Region.h"
#include "Button.h"
#include "CheckBox.h"
#include "TextControl.h"
#include "Bitmap.h"
#include "Mime.h"
#include "FilePanel.h"
#include "Menu.h"
#include "PopUpMenu.h"
#include "MenuItem.h"
#include "MenuField.h"
#include "String.h"

#include "dirent.h"

#include "ColumnListView.h"
#include "CLVColumn.h"
#include "CLVListItem.h"
#include "CLVEasyItem.h"
#include "NewStrings.h"

#include "ListControl.h"
#include "PickUser.h"
#include "ServerProperties.h"
#include "Icons.h"
#include "besure_auth.h"

const int32 MSG_SERVER_OK		= 'SOky';
const int32 MSG_SERVER_CANCEL	= 'SCan';
const int32 MSG_SERVER_NEWKEY	= 'SKey';
const int32 MSG_SERVER_JOIN		= 'SJon';


// ----- ServerPropertiesView -----------------------------------------------------

ServerPropertiesView::ServerPropertiesView(BRect rect, const char *name) :
  BView(rect, "GroupInfoView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	strcpy(server, name);
	recordLogins = isServerRecordingLogins(server);

	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(gray);

	BRect bmpRect(0.0, 0.0, 31.0, 31.0);
	icon = new BBitmap(bmpRect, B_CMAP8);
	BMimeType mime("application/x-vnd.BeServed-fileserver");
	mime.GetIcon(icon, B_LARGE_ICON);

	BRect r(10, 64, 250, 84);
	chkRecordLogins = new BCheckBox(r, "RecordLogins", "Record all login attempts on this server", new BMessage(MSG_SERVER_JOIN));
	chkRecordLogins->SetValue(recordLogins ? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild(chkRecordLogins);

	r.Set(10, 87, 170, 107);
	btnViewLogins = new BButton(r, "ViewLogBtn", "View Login Attempts", new BMessage(MSG_SERVER_NEWKEY), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	btnViewLogins->SetEnabled(recordLogins);
	AddChild(btnViewLogins);

	r.Set(205, 127, 275, 147);
	BButton *okBtn = new BButton(r, "OkayBtn", "OK", new BMessage(MSG_SERVER_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	okBtn->MakeDefault(true);
	AddChild(okBtn);

	r.Set(285, 127, 360, 147);
	AddChild(new BButton(r, "CancelBtn", "Cancel", new BMessage(MSG_SERVER_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));
}

ServerPropertiesView::~ServerPropertiesView()
{
}

void ServerPropertiesView::Draw(BRect rect)
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
	BString string(server);
	string += " Properties";
	DrawString(string.String());

	SetFont(be_plain_font);
	SetFontSize(10);
	MovePenTo(55, 28);
	DrawString("This server has certain properties which govern its domain");
	MovePenTo(55, 40);
	DrawString("membership and allow authentication servers to mitigate access");
	MovePenTo(55, 52);
	DrawString("to resources which it exports.");

	SetDrawingMode(B_OP_ALPHA); 
	SetHighColor(0, 0, 0, 180);       
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
	DrawBitmap(icon, iconRect);
}

void ServerPropertiesView::EnableControls()
{
	recordLogins = chkRecordLogins->Value() == B_CONTROL_ON;
	btnViewLogins->SetEnabled(recordLogins);
}


// ----- ServerPropertiesPanel ----------------------------------------------------------------------

ServerPropertiesPanel::ServerPropertiesPanel(BRect frame, const char *name, BWindow *parent) :
  BWindow(frame, name, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	strcpy(server, name);
	shareWin = parent;
	cancelled = true;

	BRect r = Bounds();
	infoView = new ServerPropertiesView(r, server);
	AddChild(infoView);

	Show();
}

// MessageReceived()
//
void ServerPropertiesPanel::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case MSG_SERVER_OK:
			cancelled = false;
			setServerRecordingLogins(server, infoView->GetRecordingLogins());
			BWindow::Quit();
			break;

		case MSG_SERVER_CANCEL:
			cancelled = true;
			BWindow::Quit();
			break;

		case MSG_SERVER_JOIN:
			infoView->EnableControls();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}
