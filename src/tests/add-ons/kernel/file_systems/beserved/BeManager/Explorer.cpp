#include <Application.h>
#include <Resources.h>
#include <Window.h>
#include <Region.h>
#include <Alert.h>
#include "Mime.h"

#include "string.h"
#include "stdio.h"
#include "dirent.h"
#include "sys/stat.h"

#include "ColumnListView.h"
#include "CLVColumn.h"
#include "CLVListItem.h"
#include "CLVEasyItem.h"
#include "NewStrings.h"

#include "TreeControl.h"
#include "ListControl.h"
#include "Explorer.h"
#include "Icons.h"


ExplorerFile::ExplorerFile(BView *headerView, const char *text0, const char *text1, const char *text2) :
  ListItem(headerView, text0, text1, text2)
{
	BBitmap *icon;
	BMessage archive;
	size_t size;

	char *bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), 101, &size);
	if (bits)
		if (archive.Unflatten(bits) == B_OK)
		{
			icon = new BBitmap(&archive);

			SetColumnContent(0, icon, 2.0);
			SetColumnContent(1, text0);
			SetColumnContent(2, text1);
			SetColumnContent(3, text2);
		}
}

ExplorerFile::~ExplorerFile()
{
}

bool ExplorerFile::ItemInvoked()
{
	return false;
}



ExplorerFolder::ExplorerFolder(uint32 level, bool superitem, bool expanded, int32 resourceID, BView *headerView, ColumnListView *treeView, ColumnListView *listView, char *text) :
  TreeItem(level, superitem, expanded, resourceID, headerView, listView, text)
{
	parentTreeView = treeView;
}

void ExplorerFolder::ItemExpanding()
{
	DIR *dir;
	struct dirent *dirInfo;
	struct stat st;
	char path[B_PATH_NAME_LENGTH], qualifiedFile[B_PATH_NAME_LENGTH];

	ColumnListView *listView = GetAssociatedList();
	BView *headerView = GetAssociatedHeader();

	strcpy(path, "/boot/home");
	dir = opendir(path);
	if (dir)
	{
		while ((dirInfo = readdir(dir)) != NULL)
			if (strcmp(dirInfo->d_name, ".") && strcmp(dirInfo->d_name, ".."))
			{
				sprintf(qualifiedFile, "%s/%s", path, dirInfo->d_name);
				if (stat(qualifiedFile, &st) == 0)
					if (S_ISDIR(st.st_mode))
					{
						BAlert *alert = new BAlert("hello", dirInfo->d_name, "OK");
						alert->Go();
						parentTreeView->AddItem(new ExplorerFolder(OutlineLevel() + 1, true, false, ICON_HOST_SMALL, headerView, parentTreeView, listView, dirInfo->d_name));
					}
			}

		closedir(dir);
	}
}

void ExplorerFolder::ItemSelected()
{
	DIR *dir;
	struct dirent *dirInfo;
	struct stat st;
	char path[B_PATH_NAME_LENGTH], qualifiedFile[B_PATH_NAME_LENGTH];

	PurgeRows();
	PurgeColumns();

	ColumnListView *listView = GetAssociatedList();
	BView *headerView = GetAssociatedHeader();

	listView->AddColumn(new CLVColumn(NULL, 20.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE |
		CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT));
	listView->AddColumn(new CLVColumn("File", 85.0, CLV_SORT_KEYABLE, 50.0));
	listView->AddColumn(new CLVColumn("Size", 130.0, CLV_SORT_KEYABLE));
	listView->AddColumn(new CLVColumn("Date", 150.0, CLV_SORT_KEYABLE));

	strcpy(path, "/boot/home");
	dir = opendir(path);
	if (dir)
	{
		while ((dirInfo = readdir(dir)) != NULL)
			if (strcmp(dirInfo->d_name, ".") && strcmp(dirInfo->d_name, ".."))
			{
				sprintf(qualifiedFile, "%s/%s", path, dirInfo->d_name);
				if (stat(qualifiedFile, &st) == 0)
					if (!S_ISDIR(st.st_mode))
						listView->AddItem(new ExplorerFile(headerView, dirInfo->d_name, "", ""));
			}

		closedir(dir);
	}
}
