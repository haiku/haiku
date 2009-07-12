#include "Application.h"
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
#include "Entry.h"
#include "Path.h"

#include "dirent.h"
#include "stdio.h"

#include "UserProperties.h"
#include "besure_auth.h"

const int32 MSG_USER_OK			= 'UOky';
const int32 MSG_USER_CANCEL		= 'UCan';
const int32 MSG_USER_BROWSE		= 'UBrw';
const int32 MSG_USER_REFRESH	= 'URef';

// ----- UserPropertiesView -----------------------------------------------------

UserPropertiesView::UserPropertiesView(BRect rect, const char *name) :
  BView(rect, "HostInfoView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	BMenuItem *item;
	char strDays[10];

	days = flags = 0;
	newUser = name == NULL;
	strcpy(user, newUser ? "unknown" : name);
	getUserFullName(user, fullName, sizeof(fullName));
	getUserDesc(user, desc, sizeof(desc));
	getUserPassword(user, password, sizeof(password));
	getUserFlags(user, &flags);
	getUserDaysToExpire(user, &days);
	getUserHome(user, home, sizeof(home));
	getUserGroup(user, group, sizeof(group));

	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(gray);

	BRect bmpRect(0.0, 0.0, 31.0, 31.0);
	icon = new BBitmap(bmpRect, B_CMAP8);
	BMimeType mime("application/x-vnd.Teldar-User");
	mime.GetIcon(icon, B_LARGE_ICON);

	BRect r(10, 52, 200, 72);
	editName = new BTextControl(r, "UserName", "Name:", user, NULL);
	editName->SetDivider(75);
	editName->SetEnabled(newUser);
	AddChild(editName);

	r.Set(10, 77, 250, 97);
	editFullName = new BTextControl(r, "FullName", "Full Name:", fullName, NULL);
	editFullName->SetDivider(75);
	AddChild(editFullName);

	r.Set(10, 102, 250, 122);
	editDesc = new BTextControl(r, "Description", "Description:", desc, NULL);
	editDesc->SetDivider(75);
	AddChild(editDesc);

	r.Set(10, 127, 200, 147);
	editPassword = new BTextControl(r, "Password", "Password:", password, NULL);
	editPassword->SetDivider(75);
	editPassword->TextView()->HideTyping(true);
	AddChild(editPassword);

	r.top = 157;
	r.bottom = r.top + 15;
	chkDisabled = new BCheckBox(r, "Disabled", "Account is disabled", NULL);
	chkDisabled->SetValue(flags & USER_FLAG_DISABLED ? B_CONTROL_ON : B_CONTROL_OFF);
	chkDisabled->SetEnabled(false);
	AddChild(chkDisabled);

	r.top = 177;
	r.bottom = r.top + 15;
	chkExpiresFirst = new BCheckBox(r, "Expires", "Password expires on first use", NULL);
	chkExpiresFirst->SetValue(flags & USER_FLAG_INIT_EXPIRE ? B_CONTROL_ON : B_CONTROL_OFF);
	chkExpiresFirst->SetEnabled(false);
	AddChild(chkExpiresFirst);

	r.top = 197;
	r.bottom = r.top + 15;
	chkExpiresEvery = new BCheckBox(r, "ExpireEvery", "Password expires every", NULL);
	chkExpiresEvery->SetValue(flags & USER_FLAG_DAYS_EXPIRE ? B_CONTROL_ON : B_CONTROL_OFF);
	chkExpiresEvery->SetEnabled(false);
	AddChild(chkExpiresEvery);

	r.Set(150, 197, 190, 217);
	sprintf(strDays, "%lu", days);
	editDays = new BTextControl(r, "ExpireDays", "", strDays, NULL);
	editDays->SetDivider(0);
	editDays->SetEnabled(false);
	AddChild(editDays);

	r.left = 10;
	r.top = 217;
	r.bottom = r.top + 15;
	chkCantChange = new BCheckBox(r, "CantChange", "User cannot change password", NULL);
	chkCantChange->SetValue(flags & USER_FLAG_PW_LOCKED ? B_CONTROL_ON : B_CONTROL_OFF);
	chkCantChange->SetEnabled(false);
	AddChild(chkCantChange);

	r.top = 242;
	r.bottom = r.top + 20;
	r.right = 270;
	editPath = new BTextControl(r, "HomePath", "Home Path:", home, NULL);
	editPath->SetDivider(75);
	AddChild(editPath);

	r.Set(280, 239, 360, 259);
	AddChild(new BButton(r, "BrowseBtn", "Browse...", new BMessage(MSG_USER_BROWSE)));

	mnuGroups = new BPopUpMenu("");
	DIR *groups = OpenGroups();
	if (groups)
	{
		dirent_t *group;
		while ((group = ReadGroup(groups)) != NULL)
			mnuGroups->AddItem(new BMenuItem(group->d_name, NULL));

		CloseGroups(groups);
	}

	r.Set(10, 270, 200, 290);
	mnuDefaultGroup = new BMenuField(r, "DefaultGroup", "Default Group:", mnuGroups);
	mnuDefaultGroup->SetDivider(75);
	item = mnuGroups->FindItem(group);
	if (item)
		item->SetMarked(true);
	AddChild(mnuDefaultGroup);

	r.Set(205, 320, 275, 340);
	BButton *okBtn = new BButton(r, "OkayBtn", "OK", new BMessage(MSG_USER_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	okBtn->MakeDefault(true);
	AddChild(okBtn);

	r.Set(285, 320, 360, 340);
	AddChild(new BButton(r, "CancelBtn", "Cancel", new BMessage(MSG_USER_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));
}

UserPropertiesView::~UserPropertiesView()
{
}

void UserPropertiesView::Draw(BRect rect)
{
	BRect r = Bounds();
	BRect iconRect(13.0, 5.0, 45.0, 37.0);
	rgb_color black = { 0, 0, 0, 255 };
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color disabled = { 120, 120, 120, 255 };

	SetViewColor(gray);
	SetLowColor(gray);
	FillRect(r, B_SOLID_LOW);

	SetHighColor(black);
	SetFont(be_bold_font);
	SetFontSize(11);
	MovePenTo(55, 15);
	BString string(user);
	string += " Properties";
	DrawString(string.String());

	SetFont(be_plain_font);
	SetFontSize(10);
	MovePenTo(55, 28);
	DrawString("The properties of this user account govern how it is to be used");
	MovePenTo(55, 40);
	DrawString("and the rules the authentication server applies to its maintenance.");

	if (!editDays->IsEnabled())
		SetHighColor(disabled);
	MovePenTo(202, 210);
	DrawString("days");

	SetDrawingMode(B_OP_ALPHA);
	SetHighColor(0, 0, 0, 180);
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
	DrawBitmap(icon, iconRect);
}

void UserPropertiesView::UpdateInfo()
{
	if (newUser)
		strcpy(user, editName->Text());

	strcpy(fullName, editFullName->Text());
	strcpy(desc, editDesc->Text());
	strcpy(password, editPassword->Text());
	strcpy(home, editPath->Text());

	flags = 0;
	if (chkDisabled->Value() == B_CONTROL_ON)
		flags |= USER_FLAG_DISABLED;
	if (chkExpiresFirst->Value() == B_CONTROL_ON)
		flags |= USER_FLAG_INIT_EXPIRE;
	if (chkExpiresEvery->Value() == B_CONTROL_ON)
		flags |= USER_FLAG_DAYS_EXPIRE;
	if (chkCantChange->Value() == B_CONTROL_ON)
		flags |= USER_FLAG_PW_LOCKED;

	BMenuItem *item = mnuGroups->FindMarked();
	if (item)
		strcpy(group, item->Label());
}

// SetPath()
//
void UserPropertiesView::SetPath(const char *path)
{
	if (editPath && path)
	{
		editPath->LockLooper();
		editPath->SetText(path);
		editPath->UnlockLooper();
	}
}


// ----- UserPropertiesPanel ----------------------------------------------------------------------

UserPropertiesPanel::UserPropertiesPanel(BRect frame, const char *name, BWindow *parent) :
  BWindow(frame, name, B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	newUser = name == NULL;
	if (!newUser)
		strcpy(user, name);

	shareWin = parent;
	cancelled = true;

	BRect r = Bounds();
	infoView = new UserPropertiesView(r, name == NULL ? NULL : user);
	AddChild(infoView);

	Show();
}

// MessageReceived()
//
void UserPropertiesPanel::MessageReceived(BMessage *msg)
{
	BFilePanel *filePanel;
	entry_ref entryRef;
	BPath path;

	BEntry entry("/boot/home", false);
	entry.GetRef(&entryRef);

	switch (msg->what)
	{
		case MSG_USER_BROWSE:{
			BMessenger messenger(this);
			filePanel = new BFilePanel(B_OPEN_PANEL, &messenger, &entryRef,
				B_DIRECTORY_NODE, false);
//			filePanel->SetTarget(this);
			filePanel->Show();
			filePanel->Window()->SetTitle("User Home");
			filePanel->SetButtonLabel(B_DEFAULT_BUTTON, "Select");
			break;
		}

		case B_REFS_RECEIVED:
			msg->FindRef("refs", &entryRef);
			entry.SetTo(&entryRef, true);
			entry.GetPath(&path);
			infoView->SetPath(path.Path());
			break;

		case MSG_USER_OK:
			cancelled = false;
			infoView->UpdateInfo();
			if (newUser)
			{
				strcpy(user, infoView->getUser());
				createUser(user, infoView->getPassword());
				setUserGroup(user, infoView->getGroup());
			}
			else
			{
				char group[64];
				getUserGroup(user, group, sizeof(group));
				removeUserFromGroup(user, group);
				setUserGroup(user, infoView->getGroup());
			}

			strcpy(fullName, infoView->getFullName());
			strcpy(desc, infoView->getDesc());

			setUserFullName(user, infoView->getFullName());
			setUserDesc(user, infoView->getDesc());
			setUserPassword(user, infoView->getPassword());
			setUserHome(user, infoView->getHome());
			setUserFlags(user, infoView->getFlags());
			BWindow::Quit();
			break;

		case MSG_USER_CANCEL:
			cancelled = true;
			BWindow::Quit();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}
