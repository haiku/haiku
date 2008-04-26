#include <Application.h>
#include <Resources.h>
#include <Window.h>
#include <Region.h>
#include <Alert.h>
#include "Mime.h"

#include "string.h"

#include "ColumnListView.h"
#include "CLVColumn.h"
#include "CLVListItem.h"
#include "CLVEasyItem.h"
#include "NewStrings.h"

#include "ListControl.h"


//static rgb_color active = { 155, 155, 255, 255 };
//static rgb_color inactive = { 220, 220, 220, 255 };


ListItem::ListItem(BView *headerView, const char *text0, const char *text1, const char *text2) :
  CLVEasyItem(0, false, false, 20.0)
{
	SetAssociatedHeader(headerView);
	SetColumnContent(1, text0);
	SetColumnContent(2, text1);
	SetColumnContent(3, text2);
}

ListItem::~ListItem()
{
}

bool ListItem::ItemInvoked()
{
	return false;
}

void ListItem::ItemSelected()
{
}

void ListItem::ItemDeselected()
{
}

void ListItem::ItemDeleted()
{
}

bool ListItem::IsDeleteable()
{
	return false;
}


// ----- SmartColumnListView ----------------------------------------------------------------

SmartColumnListView::SmartColumnListView(BRect Frame, CLVContainerView** ContainerView,
	const char *Name, uint32 ResizingMode, uint32 flags, list_view_type Type,
	bool hierarchical, bool horizontal, bool vertical, bool scroll_view_corner,
	border_style border, const BFont* LabelFont) :
	  ColumnListView(Frame, ContainerView, Name, ResizingMode, flags, Type, hierarchical,
		horizontal, vertical, scroll_view_corner, border, LabelFont)
{
	container = *ContainerView;
//	SetItemSelectColor(true, active);
//	SetItemSelectColor(false, inactive);
}

SmartColumnListView::~SmartColumnListView()
{
}

void SmartColumnListView::MouseDown(BPoint point)
{
	ColumnListView::MouseDown(point);
	if (CurrentSelection() < 0)
		container->Window()->PostMessage(MSG_LIST_DESELECT);
}

void SmartColumnListView::KeyUp(const char *bytes, int32 numBytes)
{
	if (*bytes == B_DELETE)
	{
		int index = CurrentSelection();
		if (index >= 0)
		{
			ListItem *item = (ListItem *) ItemAt(index);
			DeleteItem(index, item);
		}
	}
}

bool SmartColumnListView::DeleteItem(int index, ListItem *item)
{
	if (!item->IsDeleteable())
		return true;

	LockLooper();
	RemoveItem(index);
	UnlockLooper();

	item->ItemDeleted();

	if (CountItems() > 0)
	{
		Select(max(--index, 0));
		return true;
	}

	return false;
}
