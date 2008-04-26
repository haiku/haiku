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

#include "TreeControl.h"

//static rgb_color active = { 155, 155, 255, 255 };
//static rgb_color inactive = { 220, 220, 220, 255 };


SmartTreeListView::SmartTreeListView(BRect Frame, CLVContainerView** ContainerView,
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

SmartTreeListView::~SmartTreeListView()
{
}

void SmartTreeListView::MouseUp(BPoint point)
{
	int curSelection = CurrentSelection();
	ColumnListView::MouseUp(point);
	if (curSelection >= 0 && curSelection != lastSelection)
	{
		TreeItem *item = (TreeItem *) ItemAt(curSelection);
		item->ItemSelected();
		lastSelection = curSelection;
	}
}

void SmartTreeListView::Expand(TreeItem *item)
{
	ColumnListView::Expand((CLVEasyItem *) item);
	item->ItemExpanding();
}



TreeItem::TreeItem(uint32 level, bool superitem, bool expanded, int32 resourceID, BView *headerView, ColumnListView *listView, char *text) :
  CLVListItem(level, superitem, expanded, 16.0)
{
	SetAssociatedHeader(headerView);
	SetAssociatedList(listView);

	setIcon(resourceID);
	fText = new char[strlen(text) + 1];
	strcpy(fText, text);
}

TreeItem::~TreeItem()
{
	delete [] fText;
}

void TreeItem::setIcon(int32 resourceID)
{
	BMessage archive;
	size_t size;
	char *bits = (char *) be_app->AppResources()->LoadResource(type_code('BBMP'), resourceID, &size);
	if (bits)
		if (archive.Unflatten(bits) == B_OK)
			fIcon = new BBitmap(&archive);
}

void TreeItem::DrawItemColumn(BView* owner, BRect item_column_rect, int32 column_index, bool complete)
{
	rgb_color color;
	bool selected = IsSelected();

	if (selected)
		color = BeListSelectGrey;
	else
		color = White;

	owner->SetLowColor(color);
	owner->SetDrawingMode(B_OP_COPY);

	if (selected || complete)
	{
		owner->SetHighColor(color);
		owner->FillRect(item_column_rect);
	}

	BRegion Region;
	Region.Include(item_column_rect);
	owner->ConstrainClippingRegion(&Region);
	owner->SetHighColor(Black);

	if (column_index == 1)
	{
		item_column_rect.left += 2.0;
		item_column_rect.right = item_column_rect.left + 15.0;
		item_column_rect.top += 2.0;
		if (Height() > 20.0)
			item_column_rect.top += ceil((Height() - 20.0) / 2.0);
		item_column_rect.bottom = item_column_rect.top + 15.0;
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(fIcon, BRect(0.0, 0.0, 15.0, 15.0), item_column_rect);
		owner->SetDrawingMode(B_OP_COPY);
	}
	else if(column_index == 2)
		owner->DrawString(fText,
			BPoint(item_column_rect.left + 2.0, item_column_rect.top + fTextOffset));

	owner->ConstrainClippingRegion(NULL);
}

void TreeItem::Update(BView *owner, const BFont *font)
{
	CLVListItem::Update(owner, font);
	font_height FontAttributes;
	be_plain_font->GetHeight(&FontAttributes);
	float FontHeight = ceil(FontAttributes.ascent) + ceil(FontAttributes.descent);
	fTextOffset = ceil(FontAttributes.ascent) + (Height() - FontHeight) / 2.0;
}

int TreeItem::MyCompare(const CLVListItem* a_Item1, const CLVListItem* a_Item2, int32 KeyColumn)
{
	char *text1 = ((TreeItem*)a_Item1)->fText;
	char *text2 = ((TreeItem*)a_Item2)->fText;
	return strcasecmp(text1, text2);
}

void TreeItem::ItemSelected()
{
	PurgeRows();
	PurgeColumns();
	PurgeHeader();
}

void TreeItem::ItemExpanding()
{
}

void TreeItem::ListItemSelected()
{
}

void TreeItem::ListItemDeselected()
{
}

void TreeItem::ListItemUpdated(int index, CLVListItem *item)
{
}

void TreeItem::PurgeHeader()
{
	if (assocHeaderView)
	{
		int children = assocHeaderView->CountChildren();
		assocHeaderView->LockLooper();

		for (int i = children - 1; i >= 0; i--)
		{
			BView *child = assocHeaderView->ChildAt(i);
			assocHeaderView->RemoveChild(child);
			delete child;
		}

		assocHeaderView->UnlockLooper();
	}
}

void TreeItem::PurgeColumns()
{
	if (assocListView)
	{
		int columns = assocListView->CountColumns();
		if (columns > 0)
			assocListView->RemoveColumns(assocListView->ColumnAt(0), columns);
	}
}

void TreeItem::PurgeRows()
{
	if (assocListView)
	{
		int rows = assocListView->CountItems();
		if (rows > 0)
			assocListView->RemoveItems(0, rows);
	}
}

bool TreeItem::HeaderMessageReceived(BMessage *msg)
{
	return false;
}
