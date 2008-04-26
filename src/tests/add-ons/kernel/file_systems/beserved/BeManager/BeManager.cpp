#include <Application.h>
#include <Resources.h>
#include <Window.h>
#include <Region.h>
#include <Alert.h>
#include <Button.h>
#include "Mime.h"
#include "FindDirectory.h"

#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>

#include "ColumnListView.h"
#include "CLVColumn.h"
#include "CLVListItem.h"
#include "CLVEasyItem.h"
#include "NewStrings.h"

#include "ConsoleHeaderView.h"
#include "UserProperties.h"
#include "TreeControl.h"
#include "ListControl.h"
#include "Users.h"
#include "Groups.h"
#include "Servers.h"
#include "Icons.h"

const int32 MSG_ITEM_SELECT		= 'ISel';
const int32 MSG_ITEM_INVOKE		= 'IInv';
const int32 MSG_LIST_DESELECT	= 'CLV0';


// ----- GenericWindow --------------------------------------------

class GenericWindow : public BWindow
{
	public:
		GenericWindow() :
		  BWindow(BRect(40, 40, 630, 430), "Domain Management Console", B_DOCUMENT_WINDOW, 0)
		{
			BRect r = Bounds();
			r.bottom = 82;
			headerView = new ConsoleHeaderView(r);
			AddChild(headerView);

			r = Bounds();
			r.top = 85;
			r.right = 160;
		//	r.bottom -= B_H_SCROLL_BAR_HEIGHT;

			CLVContainerView *treeContView, *listContView;

			treeView = new SmartTreeListView(r, &treeContView, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,
				B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, B_SINGLE_SELECTION_LIST,
				true, true, true, false, B_FANCY_BORDER);
			treeView->AddColumn(new CLVColumn(NULL, 20.0, CLV_EXPANDER | CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE, 20.0));
			treeView->AddColumn(new CLVColumn(NULL, 16.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE |
				CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT, 16.0));
			treeView->AddColumn(new CLVColumn("Snap-ins", 125.0, CLV_SORT_KEYABLE, 50.0));
			treeView->SetSortFunction(TreeItem::MyCompare);

			r = Bounds();
			r.top = 85;
			r.left = 165 + B_V_SCROLL_BAR_WIDTH;
			r.right -= B_V_SCROLL_BAR_WIDTH;

			listView = new SmartColumnListView(r, &listContView, NULL, B_FOLLOW_ALL_SIDES,
				B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, B_SINGLE_SELECTION_LIST,
				false, true, true, false, B_FANCY_BORDER);

			listView->SetSelectionMessage(new BMessage(MSG_ITEM_SELECT));
			listView->SetInvocationMessage(new BMessage(MSG_ITEM_INVOKE));

			AddTreeItems(treeView);
			AddChild(treeContView);
			AddChild(listContView);

			SetSizeLimits(300, 2000, 150, 2000);
			Show();
		}

		~GenericWindow()
		{
			be_app->PostMessage(B_QUIT_REQUESTED);
		}

		// MessageReceived()
		//
		void MessageReceived(BMessage *msg)
		{
			TreeItem *treeItem = NULL;
			ListItem *listItem = NULL;
			int32 curTreeItem = treeView->CurrentSelection();
			int32 curListItem = listView->CurrentSelection();

			if (curTreeItem >= 0)
				treeItem = (TreeItem *) treeView->ItemAt(curTreeItem);

			if (curListItem >= 0)
				listItem = (ListItem *) listView->ItemAt(curListItem);

			switch (msg->what)
			{
				case MSG_ITEM_INVOKE:
					if (listItem)
						if (listItem->ItemInvoked())
							if (treeItem)
								treeItem->ListItemUpdated(curListItem, listItem);
					break;

				case MSG_ITEM_SELECT:
					if (treeItem)
						treeItem->ListItemSelected();
					if (listItem)
						listItem->ItemSelected();
					break;

				case MSG_LIST_DESELECT:
					if (treeItem)
						treeItem->ListItemDeselected();
					if (listItem)
						listItem->ItemDeselected();
					break;

				// If we get here, this may be a message we don't handle, or it may be one
				// that comes from the header view and needs to be processed by the associated
				// tree node.
				default:
					if (treeItem)
						if (treeItem->HeaderMessageReceived(msg))
							break;

					BWindow::MessageReceived(msg);
					break;
			}
		}

		void Quit()
		{
			TreeItem *item;

			do
			{
				item = (TreeItem *) treeView->RemoveItem(int32(0));
				if (item)
					delete item;
			} while(item);

			BWindow::Quit();
		}

	private:
		void AddTreeItems(ColumnListView* treeView)
		{
			treeView->AddItem(new TreeItem(0, true, true, ICON_NETADMIN_SMALL, headerView, listView, "Network Administration"));
			treeView->AddItem(new TreeItem(1, true, true, ICON_DOMAIN_SMALL, headerView, listView, "Domains"));
			treeView->AddItem(new TreeItem(2, true, true, ICON_DOMAIN_SMALL, headerView, listView, "default"));
			treeView->AddItem(new GroupsItem(3, false, false, ICON_GROUP_SMALL, headerView, listView, "Groups"));
			treeView->AddItem(new ServersItem(3, false, false, ICON_HOST_SMALL, headerView, listView, "Servers"));
			treeView->AddItem(new UsersItem(3, false, false, ICON_USER_SMALL, headerView, listView, "Users"));
			treeView->AddItem(new TreeItem(1, false, false, ICON_SERVICES_SMALL, headerView, listView, "Services"));
		}

		ConsoleHeaderView *headerView;
		SmartTreeListView *treeView;
		SmartColumnListView *listView;
};


// ----- GenericApp ---------------------------------------------------

class GenericApp : public BApplication
{
	public:
		GenericApp() : BApplication("application/x-vnd.Teldar-DomainManager")
		{
			checkMimeTypes();
			new GenericWindow;
		}

		~GenericApp()
		{
		}

	private:
		void checkMimeTypes()
		{
			BMimeType mime;
			mime.SetTo("application/x-vnd.BeServed-DomainManager");
			if (!mime.IsInstalled())
			{
				mime.Install();
				mime.SetShortDescription("DomainManager");
				mime.SetLongDescription("BeServed Domain Management Console 1.2.6");
				setMimeIcon(&mime, ICON_BMC_LARGE, B_LARGE_ICON);
				setMimeIcon(&mime, ICON_BMC_SMALL, B_MINI_ICON);
			}

			mime.SetTo("application/x-vnd.Teldar-User");
			if (!mime.IsInstalled())
			{
				mime.Install();
				mime.SetShortDescription("User");
				mime.SetLongDescription("BeSure User Account");
				setMimeIcon(&mime, ICON_USER_LARGE, B_LARGE_ICON);
				setMimeIcon(&mime, ICON_USER_SMALL, B_MINI_ICON);
			}

			mime.SetTo("application/x-vnd.Teldar-Group");
			if (!mime.IsInstalled())
			{
				mime.Install();
				mime.SetShortDescription("User Group");
				mime.SetLongDescription("BeSure Group Account");
				setMimeIcon(&mime, ICON_GROUP_LARGE, B_LARGE_ICON);
				setMimeIcon(&mime, ICON_GROUP_SMALL, B_MINI_ICON);
			}
		}

		bool setMimeIcon(BMimeType *mime, int32 resourceID, icon_size which)
		{
			BMessage archive;
			size_t size;
			char *bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), resourceID, &size);
			if (bits)
				if (archive.Unflatten(bits) == B_OK)
				{
					BBitmap *icon = new BBitmap(&archive);
					mime->SetIcon(icon, which);
					return true;
				}

			return false;
		}
};

#include "besure_auth.h"

int main()
{
	new GenericApp;
	be_app->Run();
	delete be_app;
}
