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
#include "GroupProperties.h"
#include "PickUser.h"
#include "Icons.h"
#include "besure_auth.h"

const int32 MSG_PICKUSER_OK		= 'GOky';
const int32 MSG_PICKUSER_CANCEL	= 'GCan';
const int32 MSG_PICKUSER_SELECT	= 'MSel';


// ----- PickUserItem ------------------------------------------------------------

PickUserItem::PickUserItem(BView *headerView, const char *text0, const char *text1) :
  ListItem(headerView, text0, text1, "")
{
	BBitmap *icon;
	BMessage archive;
	size_t size;

	char *bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), ICON_USER_SMALL, &size);
	if (bits)
		if (archive.Unflatten(bits) == B_OK)
		{
			icon = new BBitmap(&archive);

			SetAssociatedHeader(headerView);
			SetColumnContent(0, icon, 2.0);
			SetColumnContent(1, text0);
			SetColumnContent(2, text1);
		}
}

PickUserItem::~PickUserItem()
{
}

void PickUserItem::ItemSelected()
{
}

void PickUserItem::ItemDeselected()
{
}


// ----- PickUserView -----------------------------------------------------

PickUserView::PickUserView(BRect rect, char *group) :
  BView(rect, "PickUserView", B_FOLLOW_ALL, B_WILL_DRAW)
{
	strcpy(excludedGroup, group);

	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetViewColor(gray);

	BRect r;
	r.Set(125, 310, 195, 330);
	okBtn = new BButton(r, "OkayBtn", "OK", new BMessage(MSG_PICKUSER_OK), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	okBtn->MakeDefault(true);
	AddChild(okBtn);

	r.Set(205, 310, 275, 330);
	AddChild(new BButton(r, "CancelBtn", "Cancel", new BMessage(MSG_PICKUSER_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM));

	// Now add the membership list.
	CLVContainerView *listContView;
	r.Set(13, 48, 275 - B_V_SCROLL_BAR_WIDTH, 300);
	listView = new SmartColumnListView(r, &listContView, NULL, B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, B_SINGLE_SELECTION_LIST,
		false, false, true, false, B_FANCY_BORDER);

	listView->AddColumn(new CLVColumn(NULL, 20.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE |
		CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT));
	listView->AddColumn(new CLVColumn("User", 85.0, CLV_SORT_KEYABLE, 50.0));
	listView->AddColumn(new CLVColumn("Full Name", 130.0, CLV_SORT_KEYABLE));

	listView->SetSelectionMessage(new BMessage(MSG_PICKUSER_SELECT));
	listView->SetInvocationMessage(new BMessage(MSG_LIST_DESELECT));

	AddUserList(listView);
	AddChild(listContView);
}

PickUserView::~PickUserView()
{
}

void PickUserView::Draw(BRect rect)
{
	BRect r = Bounds();
	rgb_color black = { 0, 0, 0, 255 };
	rgb_color gray = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(gray);
	SetLowColor(gray);
	FillRect(r, B_SOLID_LOW);

	SetHighColor(black);
	SetFont(be_plain_font);
	SetFontSize(10);
	MovePenTo(10, 12);
	DrawString("Select a user from the list below, then click the OK button.");
	MovePenTo(10, 24);
	DrawString("Click the Cancel button to abort the selection.");

	MovePenTo(13, 43);
	DrawString("Users:");
}

void PickUserView::AddUserList(ColumnListView* listView)
{
	DIR *users;
	struct dirent *user;

	users = OpenUsers();
	if (users)
	{
		while ((user = ReadUser(users)) != NULL)
			if (!isUserInGroup(user->d_name, excludedGroup))
			{
				char desc[64];
				getUserFullName(user->d_name, desc, sizeof(desc));
				listView->AddItem(new PickUserItem(this, user->d_name, desc));
			}

		CloseGroup(users);
	}
}

char *PickUserView::GetSelectedUser()
{
	if (listView)
	{
		int index = listView->CurrentSelection();
		if (index >= 0)
		{
			PickUserItem *item = (PickUserItem *) listView->ItemAt(index);
			return ((char *) item->GetColumnContentText(1));
		}
	}

	return NULL;
}

void PickUserView::ItemSelected()
{
	okBtn->SetEnabled(true);
}

void PickUserView::ItemDeselected()
{
	okBtn->SetEnabled(false);
}


// ----- PickUserPanel ----------------------------------------------------------------------

PickUserPanel::PickUserPanel(BRect frame, char *group) :
  BWindow(frame, "Select User", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	BRect r = Bounds();
	infoView = new PickUserView(r, group);
	AddChild(infoView);

	Show();
}

// GetUser()
//
char *PickUserPanel::GetUser()
{
	if (user[0])
		return user;

	return NULL;
}

// MessageReceived()
//
void PickUserPanel::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case MSG_PICKUSER_OK:
			strcpy(user, infoView->GetSelectedUser());
			BWindow::Quit();
			break;

		case MSG_PICKUSER_CANCEL:
			user[0] = 0;
			BWindow::Quit();
			break;

		case MSG_PICKUSER_SELECT:
			infoView->ItemSelected();
			break;

		case (int32) 'CLV0': //MSG_LIST_DESELECT:
			infoView->ItemDeselected();
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}
