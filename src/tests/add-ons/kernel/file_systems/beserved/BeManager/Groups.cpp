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
#include "Groups.h"
#include "GroupProperties.h"
#include "Icons.h"
#include "besure_auth.h"

#define WARN_REMOVE_GROUP	"Removing this group may prevent access to network resources and may permanently relinquish any rights held by users not specifically granted access by user account or via membership in another group.  Are you sure you want to remove this group?"

const int32 MSG_GROUP_NEW			= 'GNew';
const int32 MSG_GROUP_EDIT			= 'GEdt';
const int32 MSG_GROUP_GONE			= 'GGon';


GroupItem::GroupItem(BView *headerView, ColumnListView *listView, const char *text0, const char *text1) :
  ListItem(headerView, text0, text1, "")
{
	BBitmap *icon;
	BMessage archive;
	size_t size;

	list = listView;

	char *bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), ICON_GROUP_SMALL, &size);
	if (bits)
		if (archive.Unflatten(bits) == B_OK)
		{
			icon = new BBitmap(&archive);

			SetColumnContent(0, icon, 2.0);
			SetColumnContent(1, text0);
			SetColumnContent(2, text1);
		}
}

GroupItem::~GroupItem()
{
}

bool GroupItem::ItemInvoked()
{
	BRect r;
	BRect frame = GetAssociatedHeader()->Window()->Frame();
	status_t exitStatus;

	r.Set(frame.left + 150, frame.top + 25, frame.left + 520, frame.top + 375);		// 370 x 350
	GroupPropertiesPanel *info = new GroupPropertiesPanel(r, GetColumnContentText(1), NULL);
	wait_for_thread(info->Thread(), &exitStatus);
	return (!info->isCancelled());
}

void GroupItem::ItemSelected()
{
}

void GroupItem::ItemDeselected()
{
}

void GroupItem::ItemDeleted()
{
	removeGroup((char *) GetColumnContentText(1));
}

bool GroupItem::IsDeleteable()
{
	BAlert *alert = new BAlert("Remove Group?", WARN_REMOVE_GROUP, "Yes", "No");
	alert->SetShortcut(1, B_ESCAPE);
	alert->SetShortcut(0, 'y');
	alert->SetShortcut(1, 'n');
	return (alert->Go() == 0);
}


GroupsItem::GroupsItem(uint32 level, bool superitem, bool expanded, int32 resourceID, BView *headerView, ColumnListView *listView, char *text) :
  TreeItem(level, superitem, expanded, resourceID, headerView, listView, text)
{
}

void GroupsItem::ItemSelected()
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
	listView->AddColumn(new CLVColumn("Group", 85.0, CLV_SORT_KEYABLE, 50.0));
	listView->AddColumn(new CLVColumn("Description", 150.0, CLV_SORT_KEYABLE));

	dir = OpenGroups();
	if (dir)
	{
		while ((dirInfo = ReadGroup(dir)) != NULL)
		{
			char desc[64];
			getGroupDesc(dirInfo->d_name, desc, sizeof(desc));
			listView->AddItem(new GroupItem(headerView, listView, dirInfo->d_name, desc));
		}

		CloseGroups(dir);
	}
}

void GroupsItem::ListItemSelected()
{
	btnEdit->SetEnabled(true);
	btnRemove->SetEnabled(true);
}

void GroupsItem::ListItemDeselected()
{
	btnEdit->SetEnabled(false);
	btnRemove->SetEnabled(false);
}

void GroupsItem::ListItemUpdated(int index, ListItem *item)
{
	ColumnListView *list = GetAssociatedList();
	char desc[64];

	getGroupDesc((char *) item->GetColumnContentText(1), desc, sizeof(desc));

	list->LockLooper();
	item->SetColumnContent(2, desc);
	list->InvalidateItem(index);
	list->UnlockLooper();
}

void GroupsItem::BuildHeader()
{
	BRect r;
	BView *headerView = GetAssociatedHeader();
	if (headerView)
	{
		r.Set(10, 55, 90, 75);
		headerView->AddChild(new BButton(r, "AddUserBtn", "Add Group", new BMessage(MSG_GROUP_NEW)));

		r.Set(100, 55, 180, 75);
		btnEdit = new BButton(r, "EditUserBtn", "Edit Group", new BMessage(MSG_GROUP_EDIT));
		btnEdit->SetEnabled(false);
		headerView->AddChild(btnEdit);

		r.Set(190, 55, 280, 75);
		btnRemove = new BButton(r, "RemoveUserBtn", "Remove Group", new BMessage(MSG_GROUP_GONE));
		btnRemove->SetEnabled(false);
		headerView->AddChild(btnRemove);
	}
}

bool GroupsItem::HeaderMessageReceived(BMessage *msg)
{
	GroupPropertiesPanel *info;
	status_t exitStatus;
	BRect r;
	BRect frame = GetAssociatedHeader()->Window()->Frame();
	r.Set(frame.left + 150, frame.top + 25, frame.left + 520, frame.top + 375);

	SmartColumnListView *listView = (SmartColumnListView *) GetAssociatedList();
	GroupItem *item = NULL;
	int index = listView->CurrentSelection();
	if (index >= 0)
		item = (GroupItem *) listView->ItemAt(index);

	switch (msg->what)
	{
		case MSG_GROUP_NEW:
			info = new GroupPropertiesPanel(r, NULL, NULL);
			wait_for_thread(info->Thread(), &exitStatus);
			if (!info->isCancelled())
			{
				listView->LockLooper();
				listView->AddItem(new GroupItem(GetAssociatedHeader(), listView, info->getGroup(), info->getDesc()));
				listView->UnlockLooper();
			}
			break;

		case MSG_GROUP_EDIT:
			if (item)
				if (item->ItemInvoked())
					ListItemUpdated(index, item);
			break;

		case MSG_GROUP_GONE:
			if (item)
				listView->DeleteItem(index, item);
			break;

		default:
			return false;
	}

	return false;
}
