#include "Application.h"
#include "Resources.h"
#include "Window.h"
#include "View.h"
#include "Region.h"
#include "Alert.h"
#include "Button.h"
#include "FindDirectory.h"
#include "Mime.h"

#include "string.h"
#include "dirent.h"

#include "ColumnListView.h"
#include "CLVColumn.h"
#include "CLVListItem.h"
#include "CLVEasyItem.h"
#include "NewStrings.h"

#include "TreeControl.h"
#include "ListControl.h"
#include "Users.h"
#include "UserProperties.h"
#include "Icons.h"
#include "besure_auth.h"

#define WARN_REMOVE_USER	"Removing this user account may prevent access to network resources and may permanently relinquish any rights held.  Are you sure you want to remove this user?"

const int32 MSG_USER_NEW			= 'UNew';
const int32 MSG_USER_EDIT			= 'UEdt';
const int32 MSG_USER_GONE			= 'UGon';


UserItem::UserItem(BView *headerView, const char *text0, const char *text1, const char *text2) :
  ListItem(headerView, text0, text1, text2)
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
			SetColumnContent(3, text2);
		}
}

UserItem::~UserItem()
{
}

bool UserItem::ItemInvoked()
{
	BRect r;
	BRect frame = GetAssociatedHeader()->Window()->Frame();
	status_t exitStatus;

	r.Set(frame.left + 150, frame.top + 25, frame.left + 520, frame.top + 375);		// 370 x 350
	UserPropertiesPanel *info = new UserPropertiesPanel(r, GetColumnContentText(1), NULL);
	wait_for_thread(info->Thread(), &exitStatus);
	return (!info->isCancelled());
}

void UserItem::ItemSelected()
{
}

void UserItem::ItemDeselected()
{
}

void UserItem::ItemDeleted()
{
	removeUser((char *) GetColumnContentText(1));
}

bool UserItem::IsDeleteable()
{
	BAlert *alert = new BAlert("Remove User?", WARN_REMOVE_USER, "Yes", "No");
	alert->SetShortcut(1, B_ESCAPE);
	alert->SetShortcut(0, 'y');
	alert->SetShortcut(1, 'n');
	return (alert->Go() == 0);
}


UsersItem::UsersItem(uint32 level, bool superitem, bool expanded, int32 resourceID, BView *headerView, ColumnListView *listView, char *text) :
  TreeItem(level, superitem, expanded, resourceID, headerView, listView, text)
{
}

void UsersItem::ItemSelected()
{
	DIR *dir;
	struct dirent *dirInfo;
	ColumnListView *listView;
	BView *headerView;

	PurgeRows();
	PurgeColumns();
	PurgeHeader();
	BuildHeader();

	listView = GetAssociatedList();
	headerView = GetAssociatedHeader();

	listView->AddColumn(new CLVColumn(NULL, 20.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE |
		CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT));
	listView->AddColumn(new CLVColumn("User", 85.0, CLV_SORT_KEYABLE, 50.0));
	listView->AddColumn(new CLVColumn("Full Name", 130.0, CLV_SORT_KEYABLE));
	listView->AddColumn(new CLVColumn("Description", 150.0, CLV_SORT_KEYABLE));

	dir = OpenUsers();
	if (dir)
	{
		while ((dirInfo = ReadUser(dir)) != NULL)
		{
			char fullName[64], desc[64];
			getUserFullName(dirInfo->d_name, fullName, sizeof(fullName));
			getUserDesc(dirInfo->d_name, desc, sizeof(desc));
			listView->AddItem(new UserItem(headerView, dirInfo->d_name, fullName, desc));
		}

		CloseUsers(dir);
	}
}

void UsersItem::ListItemSelected()
{
	btnEdit->SetEnabled(true);
	btnRemove->SetEnabled(true);
}

void UsersItem::ListItemDeselected()
{
	btnEdit->SetEnabled(false);
	btnRemove->SetEnabled(false);
}

void UsersItem::ListItemUpdated(int index, ListItem *item)
{
	ColumnListView *list = GetAssociatedList();
	char *user = (char *) item->GetColumnContentText(1);
	char fullName[64], desc[64];

	getUserFullName(user, fullName, sizeof(fullName));
	getUserDesc(user, desc, sizeof(desc));

	list->LockLooper();
	item->SetColumnContent(2, fullName);
	item->SetColumnContent(3, desc);
	list->InvalidateItem(index);
	list->UnlockLooper();
}

void UsersItem::BuildHeader()
{
	BRect r;
	BView *headerView = GetAssociatedHeader();
	if (headerView)
	{
		r.Set(10, 55, 90, 75);
		headerView->AddChild(new BButton(r, "AddUserBtn", "Add User", new BMessage(MSG_USER_NEW)));

		r.Set(100, 55, 180, 75);
		btnEdit = new BButton(r, "EditUserBtn", "Edit User", new BMessage(MSG_USER_EDIT));
		btnEdit->SetEnabled(false);
		headerView->AddChild(btnEdit);

		r.Set(190, 55, 280, 75);
		btnRemove = new BButton(r, "RemoveUserBtn", "Remove User", new BMessage(MSG_USER_GONE));
		btnRemove->SetEnabled(false);
		headerView->AddChild(btnRemove);
	}
}

bool UsersItem::HeaderMessageReceived(BMessage *msg)
{
	UserPropertiesPanel *info;
	status_t exitStatus;
	BRect r;
	BRect frame = GetAssociatedHeader()->Window()->Frame();
	r.Set(frame.left + 150, frame.top + 25, frame.left + 520, frame.top + 375);

	SmartColumnListView *listView = (SmartColumnListView *) GetAssociatedList();
	UserItem *item = NULL;
	int index = listView->CurrentSelection();
	if (index >= 0)
		item = (UserItem *) listView->ItemAt(index);

	switch (msg->what)
	{
		case MSG_USER_NEW:
			info = new UserPropertiesPanel(r, NULL, NULL);
			wait_for_thread(info->Thread(), &exitStatus);
			if (!info->isCancelled())
			{
				listView->LockLooper();
				listView->AddItem(new UserItem(GetAssociatedHeader(), info->getUser(), info->getFullName(), info->getDesc()));
				listView->UnlockLooper();
			}
			break;

		case MSG_USER_EDIT:
			if (item)
				if (item->ItemInvoked())
					ListItemUpdated(index, item);
			break;

		case MSG_USER_GONE:
			if (item)
				listView->DeleteItem(index, item);
			break;

		default:
			return false;
	}

	return false;
}
