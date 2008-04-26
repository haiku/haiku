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
#include "Servers.h"
#include "ServerProperties.h"
#include "Icons.h"
#include "besure_auth.h"

void recvServerName(char *name, uint32 addr);

const int32 MSG_SERVER_EDIT			= 'SEdt';

ColumnListView *globalListView = NULL;
BView *globalHeaderView = NULL;


ServerItem::ServerItem(BView *headerView, ColumnListView *listView, const char *text0) :
  ListItem(headerView, text0, "", "")
{
	BBitmap *icon;
	BMessage archive;
	size_t size;

	list = listView;

	char *bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), ICON_HOST_SMALL, &size);
	if (bits)
		if (archive.Unflatten(bits) == B_OK)
		{
			icon = new BBitmap(&archive);

			SetColumnContent(0, icon, 2.0);
			SetColumnContent(1, text0);
		}
}

ServerItem::~ServerItem()
{
}

bool ServerItem::ItemInvoked()
{
	BRect r;
	BRect frame = GetAssociatedHeader()->Window()->Frame();
//	status_t exitStatus;

	r.Set(frame.left + 150, frame.top + 25, frame.left + 520, frame.top + 182);		// 370 x 350
	ServerPropertiesPanel *info = new ServerPropertiesPanel(r, GetColumnContentText(1), NULL);
//	wait_for_thread(info->Thread(), &exitStatus);
	return true;
//	return (!info->isCancelled());
}

void ServerItem::ItemSelected()
{
}

void ServerItem::ItemDeselected()
{
}

void ServerItem::ItemDeleted()
{
}

bool ServerItem::IsDeleteable()
{
	return false;
}


ServersItem::ServersItem(uint32 level, bool superitem, bool expanded, int32 resourceID, BView *headerView, ColumnListView *listView, char *text) :
  TreeItem(level, superitem, expanded, resourceID, headerView, listView, text)
{
}

void ServersItem::ItemSelected()
{
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
	listView->AddColumn(new CLVColumn("Server", 100.0, CLV_SORT_KEYABLE, 50.0));

	globalHeaderView = headerView;
	globalListView = listView;
	GetServerList(recvServerName);
}

void ServersItem::ListItemSelected()
{
	btnEdit->SetEnabled(true);
}

void ServersItem::ListItemDeselected()
{
	btnEdit->SetEnabled(false);
}

void ServersItem::ListItemUpdated(int index, ListItem *item)
{
	ColumnListView *list = GetAssociatedList();

//	getGroupDesc((char *) item->GetColumnContentText(1), desc, sizeof(desc));

	list->LockLooper();
//	item->SetColumnContent(2, desc);
	list->InvalidateItem(index);
	list->UnlockLooper();
}

void ServersItem::BuildHeader()
{
	BRect r;
	BView *headerView = GetAssociatedHeader();
	if (headerView)
	{
		r.Set(10, 55, 90, 75);
		btnEdit = new BButton(r, "EditServerBtn", "Properties", new BMessage(MSG_SERVER_EDIT));
		btnEdit->SetEnabled(false);
		headerView->AddChild(btnEdit);
	}
}

bool ServersItem::HeaderMessageReceived(BMessage *msg)
{
	BRect r;
	BRect frame = GetAssociatedHeader()->Window()->Frame();
	r.Set(frame.left + 150, frame.top + 25, frame.left + 520, frame.top + 375);

	SmartColumnListView *listView = (SmartColumnListView *) GetAssociatedList();
	ServerItem *item = NULL;
	int index = listView->CurrentSelection();
	if (index >= 0)
		item = (ServerItem *) listView->ItemAt(index);

	switch (msg->what)
	{
		case MSG_SERVER_EDIT:
			if (item)
				if (item->ItemInvoked())
					ListItemUpdated(index, item);
			break;

		default:
			return false;
	}

	return false;
}


void recvServerName(char *name, uint32 addr)
{
	if (globalListView)
	{
		globalListView->LockLooper();
		globalListView->AddItem(new ServerItem(globalHeaderView, globalListView, name));
		globalListView->UnlockLooper();
	}
}
