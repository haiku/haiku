//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		OutlineListView.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BOutlineListView represents a "nestable" list view.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <OutlineListView.h>
#include <stdio.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BOutlineListView::BOutlineListView(BRect frame, const char * name,
								   list_view_type type, uint32 resizeMask,
								   uint32 flags)
	:	BListView(frame, name, type, resizeMask, flags)
{
}
//------------------------------------------------------------------------------	
BOutlineListView::BOutlineListView(BMessage *archive)
	:	BListView(archive)
{
}
//------------------------------------------------------------------------------
BOutlineListView::~BOutlineListView()
{
	fullList.MakeEmpty();
}
//------------------------------------------------------------------------------
BArchivable *BOutlineListView::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BOutlineListView"))
		return new BOutlineListView(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BOutlineListView::Archive(BMessage *data, bool deep) const
{
	return BListView::Archive(data, deep);
}
//------------------------------------------------------------------------------
void BOutlineListView::MouseDown(BPoint point)
{
	MakeFocus();

	int32 index = IndexOf(point);

	if (index != -1)
	{
		BListItem *item = ItemAt(index);

		if (item->fHasSubitems &&
			LatchRect (ItemFrame(index ), item->fLevel).Contains(point))
		{
			if (item->IsExpanded())
				Collapse(item);
			else
				Expand(item);
		}
		else BListView::MouseDown(point);
	}
}
//------------------------------------------------------------------------------
void BOutlineListView::KeyDown(const char *bytes, int32 numBytes)
{
	if (numBytes == 1)
	{
		switch (bytes[0])
		{
			case '+':
			{
				BListItem *item = ItemAt(CurrentSelection());
				
				if ( item && item->fHasSubitems)
					Expand(item);

				break;
			}
			case '-':
			{
				BListItem *item = ItemAt(CurrentSelection());
				
				if (item && item->fHasSubitems)
					Collapse(item);

				break;
			}
			default:
				BListView::KeyDown(bytes, numBytes);
		}
	}
	else
		BListView::KeyDown(bytes, numBytes);
}
//------------------------------------------------------------------------------
void BOutlineListView::FrameMoved(BPoint new_position)
{
	BListView::FrameMoved(new_position);
}
//------------------------------------------------------------------------------
void BOutlineListView::FrameResized(float new_width, float new_height)
{
	BListView::FrameResized(new_width, new_height);
}
//------------------------------------------------------------------------------
void BOutlineListView::MouseUp(BPoint where)
{
	BListView::MouseUp(where);
}
//------------------------------------------------------------------------------
bool BOutlineListView::AddUnder(BListItem *item, BListItem *superitem)
{
	fullList.AddItem(item, FullListIndexOf(superitem) + 1);

	item->fLevel = superitem->OutlineLevel() + 1;
	superitem->fHasSubitems = true;

	if (superitem->IsItemVisible() && superitem->IsExpanded())
	{
		item->SetItemVisible(true);

		int32 index = BListView::IndexOf(superitem);

		BListView::AddItem(item, index + 1);
		Invalidate(LatchRect(ItemFrame(index), superitem->OutlineLevel()));
	}
	else
		item->SetItemVisible(false);

	return true;
}
//------------------------------------------------------------------------------
bool BOutlineListView::AddItem(BListItem *item)
{
	bool result = fullList.AddItem(item);

	if (!result)
		return false;

	return BListView::AddItem(item);
}
//------------------------------------------------------------------------------
bool BOutlineListView::AddItem(BListItem *item, int32 fullListIndex)
{
	if (fullListIndex < 0)
		fullListIndex = 0;
	else if (fullListIndex > CountItems())
		fullListIndex = CountItems();

	fullList.AddItem(item, fullListIndex);

	if (item->fLevel > 0)
	{
		BListItem *super = SuperitemForIndex(fullListIndex, item->fLevel);

		if (!super->IsItemVisible() || !super->IsExpanded())
			return true;
	}

	int32 list_index = FindPreviousVisibleIndex(fullListIndex);
		
	return BListView::AddItem(item, IndexOf(FullListItemAt(list_index)) + 1);
}
//------------------------------------------------------------------------------
bool BOutlineListView::AddList(BList *newItems)
{
	printf("BOutlineListView::AddList Not implemented\n");
	return false;
}
//------------------------------------------------------------------------------
bool BOutlineListView::AddList(BList *newItems, int32 fullListIndex)
{
	printf("BOutlineListView::AddList Not implemented\n");
	return false;
}
//------------------------------------------------------------------------------
bool BOutlineListView::RemoveItem(BListItem *item)
{
	if (!FullListHasItem(item))
		return false;

	fullList.RemoveItem(item);
	if (item->fVisible)
		BListView::RemoveItem(item);

	// TODO: remove children

	return true;
}
//------------------------------------------------------------------------------
BListItem *BOutlineListView::RemoveItem(int32 fullListIndex)
{
	BListItem *item = FullListItemAt(fullListIndex);

	if (item == NULL)
		return NULL;

	fullList.RemoveItem(fullListIndex);
	if (item->fVisible)
		BListView::RemoveItem(item);

	// TODO: remove children

	return item;
}
//------------------------------------------------------------------------------
bool BOutlineListView::RemoveItems(int32 fullListIndex, int32 count)
{
	printf("BOutlineListView::RemoveItems Not implemented\n");

	return false;
}
//------------------------------------------------------------------------------
BListItem *BOutlineListView::FullListItemAt(int32 fullListIndex) const
{
	return (BListItem*)fullList.ItemAt(fullListIndex);
}
//------------------------------------------------------------------------------
int32 BOutlineListView::FullListIndexOf(BPoint point) const
{
	return BListView::IndexOf(point);
}
//------------------------------------------------------------------------------
int32 BOutlineListView::FullListIndexOf(BListItem *item) const
{
	return fullList.IndexOf(item);
}
//------------------------------------------------------------------------------
BListItem *BOutlineListView::FullListFirstItem() const
{
	return (BListItem*)fullList.FirstItem();
}
//------------------------------------------------------------------------------
BListItem *BOutlineListView::FullListLastItem() const
{
	return (BListItem*)fullList.LastItem();
}
//------------------------------------------------------------------------------
bool BOutlineListView::FullListHasItem(BListItem *item) const
{
	return fullList.HasItem(item);
}
//------------------------------------------------------------------------------
int32 BOutlineListView::FullListCountItems() const
{
	return fullList.CountItems();
}
//------------------------------------------------------------------------------
int32 BOutlineListView::FullListCurrentSelection(int32 index) const
{
	int32 i = BListView::CurrentSelection(index);
	BListItem *item = BListView::ItemAt(i);

	if (item)
		return fullList.IndexOf(item);
	else
		return -1;
}
//------------------------------------------------------------------------------
void BOutlineListView::MakeEmpty()
{
	fullList.MakeEmpty();
	BListView::MakeEmpty();
}
//------------------------------------------------------------------------------
bool BOutlineListView::FullListIsEmpty() const
{
	return fullList.IsEmpty();
}
//------------------------------------------------------------------------------
void BOutlineListView::FullListDoForEach(bool(*func)(BListItem *))
{
	printf("BOutlineListView::fullListDoForEach Not implemented\n");
	//fullList.DoForEach(func);
}
//------------------------------------------------------------------------------
void BOutlineListView::FullListDoForEach(bool (*func)(BListItem *, void *),
										 void *data)
{
	printf("BOutlineListView::fullListDoForEach Not implemented\n");
	//fullList.DoForEach(func, data);
}
//------------------------------------------------------------------------------
BListItem *BOutlineListView::Superitem(const BListItem *item)
{
	int32 index = FullListIndexOf((BListItem*)item);

	if (index == -1)
		return NULL;
	else
		return SuperitemForIndex(index, item->OutlineLevel());
}
//------------------------------------------------------------------------------
void BOutlineListView::Expand(BListItem *item)
{
	if (!FullListHasItem(item))
		return;

	if (item->fExpanded)
		return;

	item->fExpanded = true;

	uint32 level = item->fLevel, full_index = FullListIndexOf(item),
		index = IndexOf(item) + 1, count = FullListCountItems() - full_index - 1;
	BListItem **items = (BListItem**)fullList.Items() + full_index + 1;

	BFont font;
	GetFont(&font);

	while (count-- > 0 && (*items)->fLevel > level)
	{
		if (!(*items)->IsItemVisible())
		{
			//BListView::AddItem((*items), index++);
			fList.AddItem((*items), index++);
			(*items)->Update(this,&font);
			(*items)->SetItemVisible(true);
		}

		// Skip hidden children
		if ((*items)->HasSubitems() && !(*items)->IsExpanded())
		{
			uint32 level = (*items)->fLevel;
			items++;
			
			while (--count > 0 && (*items)->fLevel > level)
				items++;
		}
		else
			items++;
	}

	FixupScrollBar();
	Invalidate();
}
//------------------------------------------------------------------------------
void BOutlineListView::Collapse(BListItem *item)
{
	if (!FullListHasItem(item))
		return;

	if (!item->fExpanded)
		return;

	item->fExpanded = false;

	uint32 level = item->fLevel, full_index = FullListIndexOf(item),
		index = IndexOf(item) + 1, count = FullListCountItems() - full_index - 1;
	BListItem **items = (BListItem**)fullList.Items() + full_index + 1;

	while (count-- > 0 && (*items)->fLevel > level)
	{
		if ((*items)->IsItemVisible())
		{
			//BListView::RemoveItem ((*items));
			fList.RemoveItem((*items));
			(*items)->SetItemVisible(false);
		}

		items++;
	}

	FixupScrollBar();
	Invalidate();
}
//------------------------------------------------------------------------------
bool BOutlineListView::IsExpanded(int32 fullListIndex)
{
	BListItem *item = FullListItemAt(fullListIndex);
	
	if (!item)
		return false;

	return item->IsExpanded();
}
//------------------------------------------------------------------------------
BHandler *BOutlineListView::ResolveSpecifier(BMessage *msg, int32 index,
											 BMessage *specifier, int32 form,
											 const char *property)
{
	return BListView::ResolveSpecifier(msg, index, specifier, form, property);
}
//------------------------------------------------------------------------------
status_t BOutlineListView::GetSupportedSuites(BMessage *data)
{
	return BListView::GetSupportedSuites(data);
}
//------------------------------------------------------------------------------
status_t BOutlineListView::Perform(perform_code d, void *arg)
{
	return BListView::Perform(d, arg);
}
//------------------------------------------------------------------------------
void BOutlineListView::ResizeToPreferred()
{
	BListView::ResizeToPreferred();
}
//------------------------------------------------------------------------------
void BOutlineListView::GetPreferredSize(float *width, float *height)
{
	BListView::GetPreferredSize(width, height);
}
//------------------------------------------------------------------------------
void BOutlineListView::MakeFocus(bool state)
{
	BListView::MakeFocus(state);
}
//------------------------------------------------------------------------------
void BOutlineListView::AllAttached()
{
	BListView::AllAttached();
}
//------------------------------------------------------------------------------
void BOutlineListView::AllDetached()
{
	BListView::AllDetached();
}
//------------------------------------------------------------------------------
void BOutlineListView::DetachedFromWindow()
{
	BListView::DetachedFromWindow();
}
//------------------------------------------------------------------------------
void BOutlineListView::FullListSortItems(int (*compareFunc)(const BListItem *,
										 const BListItem *))
{
	//fullList.SortItems(compareFunc);
	printf("BOutlineListView::FullListSortItems not implemented\n");
}
//------------------------------------------------------------------------------
void BOutlineListView::SortItemsUnder(BListItem *underItem, bool oneLevelOnly,
		int (*compareFunc)(const BListItem *, const BListItem*))
{
	printf("BOutlineListView::SortItemsUnder not implemented\n");
}
//------------------------------------------------------------------------------
int32 BOutlineListView::CountItemsUnder(BListItem *underItem,
									  bool oneLevelOnly) const
{
	int32 i = IndexOf(underItem);

	if (i == -1)
		return 0;

	int32 count = 0;

	if (oneLevelOnly)
	{
		while (i < FullListCountItems())
		{
			BListItem *item = FullListItemAt(i);

			// If we jump out of the subtree, return count
			if (item->fLevel < underItem->OutlineLevel())
				return count;

			// If the level matches, increase count
			if (item->fLevel == underItem->OutlineLevel() + 1)
				count++;

			i++;
		}
	}
	else
	{
		while (i < FullListCountItems())
		{
			BListItem *item = FullListItemAt(i);

			// If we jump out of the subtree, return count
			if (item->fLevel < underItem->OutlineLevel())
				return count;

			// Increase count
			count++;

			i++;
		}
	}

	return count;
}
//------------------------------------------------------------------------------
BListItem *BOutlineListView::EachItemUnder(BListItem *underItem,
										   bool oneLevelOnly,
										   BListItem *(*eachFunc)(BListItem *,
										   void *), void *)
{
	printf("BOutlineListView::EachItemUnder Not implemented\n");

	return NULL;
}
//------------------------------------------------------------------------------
BListItem *BOutlineListView::ItemUnderAt(BListItem *underItem,
										 bool oneLevelOnly, int32 index) const
{
	int32 i = IndexOf(underItem);

	if (i == -1)
		return NULL;

	if (oneLevelOnly)
	{
		while (i < FullListCountItems())
		{
			BListItem *item = FullListItemAt(i);

			// If we jump out of the subtree, return NULL
			if (item->fLevel < underItem->OutlineLevel())
				return NULL;

			// If the level matches, check the index
			if (item->fLevel == underItem->OutlineLevel() + 1)
			{
				if (index == 0)
					return item;
				else
					index--;
			}

			i++;
		}
	}
	else
	{
		while (i < FullListCountItems())
		{
			BListItem *item = FullListItemAt(i);

			// If we jump out of the subtree, return NULL
			if (item->fLevel < underItem->OutlineLevel())
				return NULL;

			// Check the index
			if (index == 0)
				return item;
			else
				index--;

			i++;
		}
	}

	return NULL;
}
//------------------------------------------------------------------------------
bool BOutlineListView::DoMiscellaneous(MiscCode code, MiscData * data)
{
	return BListView::DoMiscellaneous(code, data);
}
//------------------------------------------------------------------------------
void BOutlineListView::MessageReceived(BMessage *msg)
{
	BListView::MessageReceived(msg);
}
//------------------------------------------------------------------------------
void BOutlineListView::_ReservedOutlineListView1() {}
void BOutlineListView::_ReservedOutlineListView2() {}
void BOutlineListView::_ReservedOutlineListView3() {}
void BOutlineListView::_ReservedOutlineListView4() {}
//------------------------------------------------------------------------------
int32 BOutlineListView::FullListIndex(int32 index) const
{
	BListItem *item = ItemAt(index);

	if (item == NULL)
		return -1;
	else
		return FullListIndexOf(item);
}
//------------------------------------------------------------------------------
int32 BOutlineListView::ListViewIndex(int32 index) const
{
	BListItem *item = ItemAt(index);

	if (item == NULL)
		return -1;
	else
		return BListView::IndexOf(item);
}
//------------------------------------------------------------------------------
void BOutlineListView::ExpandOrCollapse(BListItem *underItem, bool expand)
{
}
//------------------------------------------------------------------------------
BRect BOutlineListView::LatchRect(BRect itemRect, int32 level) const
{
	return BRect(itemRect.left, itemRect.top, itemRect.left +
		(level * 10.0f + 15.0f), itemRect.bottom);
}
//------------------------------------------------------------------------------
void BOutlineListView::DrawLatch(BRect itemRect, int32 level, bool collapsed,
								 bool highlighted, bool misTracked)
{
	float left = level * 10.0f;

	if (collapsed)
	{
		SetHighColor(192, 192, 192);

		FillTriangle(itemRect.LeftTop() + BPoint(left + 4.0f, 2.0f),
			itemRect.LeftTop() + BPoint(left + 4.0f, 10.0f),
			itemRect.LeftTop() + BPoint(left + 8.0f, 6.0f));

		SetHighColor(0, 0, 0);

		StrokeTriangle(itemRect.LeftTop() + BPoint(left + 4.0f, 2.0f),
			itemRect.LeftTop() + BPoint(left + 4.0f, 10.0f),
			itemRect.LeftTop() + BPoint(left + 8.0f, 6.0f));
	}
	else
	{
		SetHighColor(192, 192, 192);
			
		FillTriangle(itemRect.LeftTop() + BPoint(left + 2.0f, 4.0f),
			itemRect.LeftTop() + BPoint(left + 10.0f, 4.0f),
			itemRect.LeftTop() + BPoint(left + 6.0f, 8.0f));

		SetHighColor(0, 0, 0);

		StrokeTriangle(itemRect.LeftTop() + BPoint(left + 2.0f, 4.0f),
			itemRect.LeftTop() + BPoint(left + 10.0f, 4.0f),
			itemRect.LeftTop() + BPoint(left + 6.0f, 8.0f));
	}
}
//------------------------------------------------------------------------------
void BOutlineListView::DrawItem(BListItem *item, BRect itemRect, bool complete)
{
	if (item->fHasSubitems)
		DrawLatch(itemRect, item->fLevel, !item->IsExpanded(), false, false);

	itemRect.left += (item->fLevel) * 10.0f + 15.0f;

	item->DrawItem(this, itemRect, complete);
}
//------------------------------------------------------------------------------
BListItem *BOutlineListView::RemoveCommon(int32 fullListIndex)
{
	return NULL;
}
//------------------------------------------------------------------------------
BListItem *BOutlineListView::RemoveOne(int32 fullListIndex)
{
	return NULL;
}
//------------------------------------------------------------------------------
void BOutlineListView::TrackInLatchItem(void *)
{
}
//------------------------------------------------------------------------------
void BOutlineListView::TrackOutLatchItem(void *)
{
}
//------------------------------------------------------------------------------
bool BOutlineListView::OutlineSwapItems(int32 a, int32 b)
{
	return false;
}
//------------------------------------------------------------------------------
bool BOutlineListView::OutlineMoveItem(int32 from, int32 to)
{
	return false;
}
//------------------------------------------------------------------------------
bool BOutlineListView::OutlineReplaceItem(int32 index, BListItem *item)
{
	return false;
}
//------------------------------------------------------------------------------
void BOutlineListView::CommonMoveItems(int32 from, int32 count, int32 to)
{
}
//------------------------------------------------------------------------------
BListItem *BOutlineListView::SuperitemForIndex(int32 fullListIndex, int32 level)
{
	BListItem *item;
	fullListIndex--;

	while (fullListIndex >= 0)
	{
		if ((item = FullListItemAt(fullListIndex))->OutlineLevel() <
			(uint32)level)
			return item;

		fullListIndex--;
	}

	return NULL;
}
//------------------------------------------------------------------------------
int32 BOutlineListView::FindPreviousVisibleIndex(int32 fullListIndex)
{
	fullListIndex--;

	while (fullListIndex >= 0)
	{
		if (FullListItemAt(fullListIndex)->fVisible)
			return fullListIndex;

		fullListIndex--;
	}

	return -1;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
