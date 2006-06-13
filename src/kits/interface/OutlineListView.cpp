/*
 * Copyright 2001-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Axel Dörfler, axeld@pinc-software.de
 */

//! BOutlineListView represents a "nestable" list view.

#include <OutlineListView.h>

#include <stdio.h>
#include <stdlib.h>


/*!
	\brief A simple recursive quick sort implementations for BListItem arrays.

	We need to implement it ourselves, since the BOutlineListView sort methods
	use a different comparison function than the standard functions.
*/
static void
quick_sort_item_array(BListItem** items, int32 first, int32 last,
	int (*compareFunc)(const BListItem* a, const BListItem* b))
{
	if (last == first)
		return;

	BListItem* pivot = items[first + (rand() % (last - first))];
		// choose an arbitrary pivot element

	int32 left = first;
	int32 right = last;

	do {
		while (compareFunc(items[left], pivot) < 0) {
			left++;
		}
		while (compareFunc(items[right], pivot) > 0) {
			right--;
		}

		if (left <= right) {
			// swap entries
			BListItem* temp = items[left];
			items[left] = items[right];
			items[right] = temp;

			left++;
			right--;
		}
	} while (left <= right);

	// At this point, the elements in the left half are all smaller
	// than the elements in the right half

	if (first < right) {
		// sort left half
		quick_sort_item_array(items, first, right, compareFunc);
	}
	if (left < last) {
		// sort right half
		quick_sort_item_array(items, left, last, compareFunc);
	}
}


//	#pragma mark -


BOutlineListView::BOutlineListView(BRect frame, const char* name,
		list_view_type type, uint32 resizeMode, uint32 flags)
	: BListView(frame, name, type, resizeMode, flags)
{
}


BOutlineListView::BOutlineListView(BMessage* archive)
	: BListView(archive)
{
}


BOutlineListView::~BOutlineListView()
{
	fFullList.MakeEmpty();
}


BArchivable *
BOutlineListView::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BOutlineListView"))
		return new BOutlineListView(archive);

	return NULL;
}


status_t
BOutlineListView::Archive(BMessage* archive, bool deep) const
{
	return BListView::Archive(archive, deep);
}


void
BOutlineListView::MouseDown(BPoint point)
{
	MakeFocus();

	int32 index = IndexOf(point);

	if (index != -1) {
		BListItem *item = ItemAt(index);

		if (item->fHasSubitems
			&& LatchRect(ItemFrame(index), item->fLevel).Contains(point)) {
			if (item->IsExpanded())
				Collapse(item);
			else
				Expand(item);
		} else
			BListView::MouseDown(point);
	}
}


void
BOutlineListView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes == 1) {
		switch (bytes[0]) {
			case B_RIGHT_ARROW:
			{
				BListItem *item = ItemAt(CurrentSelection());
				if (item && item->fHasSubitems)
					Expand(item);
				return;
			}

			case B_LEFT_ARROW:
			{
				BListItem *item = ItemAt(CurrentSelection());
				if (item && item->fHasSubitems)
					Collapse(item);
				return;
			}
		}
	}

	BListView::KeyDown(bytes, numBytes);
}


void
BOutlineListView::FrameMoved(BPoint newPosition)
{
	BListView::FrameMoved(newPosition);
}


void
BOutlineListView::FrameResized(float newWidth, float newHeight)
{
	BListView::FrameResized(newWidth, newHeight);
}


void
BOutlineListView::MouseUp(BPoint where)
{
	BListView::MouseUp(where);
}


bool
BOutlineListView::AddUnder(BListItem* item, BListItem* superitem)
{
	if (superitem == NULL)
		return AddItem(item);

	fFullList.AddItem(item, FullListIndexOf(superitem) + 1);

	item->fLevel = superitem->OutlineLevel() + 1;
	superitem->fHasSubitems = true;

	if (superitem->IsItemVisible() && superitem->IsExpanded()) {
		item->SetItemVisible(true);

		int32 index = BListView::IndexOf(superitem);

		BListView::AddItem(item, index + 1);
		Invalidate(LatchRect(ItemFrame(index), superitem->OutlineLevel()));
	} else
		item->SetItemVisible(false);

	return true;
}


bool
BOutlineListView::AddItem(BListItem* item)
{
	return AddItem(item, FullListCountItems());
}


bool
BOutlineListView::AddItem(BListItem* item, int32 fullListIndex)
{
	if (fullListIndex < 0)
		fullListIndex = 0;
	else if (fullListIndex > FullListCountItems())
		fullListIndex = FullListCountItems();

	if (!fFullList.AddItem(item, fullListIndex))
		return false;

	// Check if this item is visible, and if it is, add it to the
	// other list, too

	if (item->fLevel > 0) {
		BListItem *super = _SuperitemForIndex(fullListIndex, item->fLevel);
		if (super == NULL)
			return true;

		bool hadSubitems = super->fHasSubitems;
		super->fHasSubitems = true;

		if (!super->IsItemVisible() || !super->IsExpanded()) {
			item->SetItemVisible(false);
			return true;
		}

		if (!hadSubitems)
			Invalidate(LatchRect(ItemFrame(IndexOf(super)), super->OutlineLevel()));
	}

	int32 listIndex = _FindPreviousVisibleIndex(fullListIndex);

	if (!BListView::AddItem(item, IndexOf(FullListItemAt(listIndex)) + 1)) {
		// adding didn't work out, we need to remove it from the main list again
		fFullList.RemoveItem(fullListIndex);
		return false;
	}

	return true;
}


bool
BOutlineListView::AddList(BList* newItems)
{
	printf("BOutlineListView::AddList Not implemented\n");
	return false;
}


bool
BOutlineListView::AddList(BList* newItems, int32 fullListIndex)
{
	printf("BOutlineListView::AddList Not implemented\n");
	return false;
}


bool
BOutlineListView::RemoveItem(BListItem* item)
{
	return _RemoveItem(item, FullListIndexOf(item)) != NULL;
}


BListItem*
BOutlineListView::RemoveItem(int32 fullIndex)
{
	return _RemoveItem(FullListItemAt(fullIndex), fullIndex);
}


bool
BOutlineListView::RemoveItems(int32 fullListIndex, int32 count)
{
	printf("BOutlineListView::RemoveItems Not implemented\n");

	return false;
}


BListItem *
BOutlineListView::FullListItemAt(int32 fullListIndex) const
{
	return (BListItem*)fFullList.ItemAt(fullListIndex);
}


int32
BOutlineListView::FullListIndexOf(BPoint point) const
{
	return BListView::IndexOf(point);
}


int32
BOutlineListView::FullListIndexOf(BListItem* item) const
{
	return fFullList.IndexOf(item);
}


BListItem *
BOutlineListView::FullListFirstItem() const
{
	return (BListItem*)fFullList.FirstItem();
}


BListItem *
BOutlineListView::FullListLastItem() const
{
	return (BListItem*)fFullList.LastItem();
}


bool
BOutlineListView::FullListHasItem(BListItem *item) const
{
	return fFullList.HasItem(item);
}


int32
BOutlineListView::FullListCountItems() const
{
	return fFullList.CountItems();
}


int32
BOutlineListView::FullListCurrentSelection(int32 index) const
{
	int32 i = BListView::CurrentSelection(index);

	BListItem *item = BListView::ItemAt(i);
	if (item)
		return fFullList.IndexOf(item);

	return -1;
}


void
BOutlineListView::MakeEmpty()
{
	fFullList.MakeEmpty();
	BListView::MakeEmpty();
}


bool
BOutlineListView::FullListIsEmpty() const
{
	return fFullList.IsEmpty();
}


void
BOutlineListView::FullListDoForEach(bool(*func)(BListItem* item))
{
	fFullList.DoForEach(reinterpret_cast<bool (*)(void*)>(func));
}


void
BOutlineListView::FullListDoForEach(bool (*func)(BListItem* item, void* arg),
	void* arg)
{
	fList.DoForEach(reinterpret_cast<bool (*)(void*, void*)>(func), arg);
}


BListItem *
BOutlineListView::Superitem(const BListItem* item)
{
	int32 index = FullListIndexOf((BListItem*)item);
	if (index == -1)
		return NULL;

	return _SuperitemForIndex(index, item->OutlineLevel());
}


void
BOutlineListView::Expand(BListItem* item)
{
	if (item->IsExpanded() || !FullListHasItem(item))
		return;

	item->fExpanded = true;

	uint32 level = item->fLevel;
	int32 fullIndex = FullListIndexOf(item);
	int32 index = IndexOf(item) + 1;
	int32 count = FullListCountItems() - fullIndex - 1;
	BListItem** items = (BListItem**)fFullList.Items() + fullIndex + 1;

	BFont font;
	GetFont(&font);

	while (count-- > 0) {
		item = items[0];
		if (item->fLevel <= level)
			break;

		if (!item->IsItemVisible()) {
			// fix selection hints
			if (index <= fFirstSelected)
				fFirstSelected++;
			if (index <= fLastSelected)
				fLastSelected++;

			fList.AddItem(item, index++);
			item->Update(this, &font);
			item->SetItemVisible(true);
		}

		if (item->HasSubitems() && !item->IsExpanded()) {
			// Skip hidden children
			uint32 subLevel = item->fLevel;
			items++;

			while (--count > 0 && items[0]->fLevel > subLevel)
				items++;
		} else
			items++;
	}

	_FixupScrollBar();
	Invalidate();
}


void
BOutlineListView::Collapse(BListItem* item)
{
	if (!item->IsExpanded() || !FullListHasItem(item))
		return;

	item->fExpanded = false;

	uint32 level = item->fLevel;
	int32 fullIndex = FullListIndexOf(item);
	int32 index = IndexOf(item);
	int32 max = FullListCountItems() - fullIndex - 1;
	int32 count = 0;
	BListItem** items = (BListItem**)fFullList.Items() + fullIndex + 1;

	while (max-- > 0) {
		item = items[0];
		if (item->fLevel <= level)
			break;

		if (item->IsItemVisible()) {
			fList.RemoveItem(item);
			item->SetItemVisible(false);
			if (item->IsSelected())
				item->Deselect();

			count++;
		}

		items++;
	}

	// fix selection hints
	// TODO: revise for multi selection lists
	if (index < fFirstSelected) {
		if (index + count < fFirstSelected) {
			fFirstSelected -= count;
			fLastSelected -= count;
		} else {
			// select top item
			//fFirstSelected = fLastSelected = index;
			//item->Select();
			Select(index);
		}
	}

	_FixupScrollBar();
	Invalidate();
}


bool
BOutlineListView::IsExpanded(int32 fullListIndex)
{
	BListItem *item = FullListItemAt(fullListIndex);
	if (!item)
		return false;

	return item->IsExpanded();
}


BHandler *
BOutlineListView::ResolveSpecifier(BMessage* msg, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	return BListView::ResolveSpecifier(msg, index, specifier, what, property);
}


status_t
BOutlineListView::GetSupportedSuites(BMessage* data)
{
	return BListView::GetSupportedSuites(data);
}


status_t
BOutlineListView::Perform(perform_code d, void* arg)
{
	return BListView::Perform(d, arg);
}


void
BOutlineListView::ResizeToPreferred()
{
	BListView::ResizeToPreferred();
}


void
BOutlineListView::GetPreferredSize(float* _width, float* _height)
{
	BListView::GetPreferredSize(_width, _height);
}


void
BOutlineListView::MakeFocus(bool state)
{
	BListView::MakeFocus(state);
}


void
BOutlineListView::AllAttached()
{
	BListView::AllAttached();
}


void
BOutlineListView::AllDetached()
{
	BListView::AllDetached();
}


void
BOutlineListView::DetachedFromWindow()
{
	BListView::DetachedFromWindow();
}


void
BOutlineListView::FullListSortItems(int (*compareFunc)(const BListItem* a,
	const BListItem* b))
{
	SortItemsUnder(NULL, false, compareFunc);
}


void
BOutlineListView::SortItemsUnder(BListItem* underItem, bool oneLevelOnly,
	int (*compareFunc)(const BListItem* a, const BListItem* b))
{
	// This method is quite complicated: basically, it creates a real tree
	// from the items of the full list, sorts them as needed, and then
	// populates the entries back into the full and display lists

	int32 firstIndex = FullListIndexOf(underItem) + 1;
	int32 lastIndex = firstIndex;
	BList* tree = _BuildTree(underItem, lastIndex);

	_SortTree(tree, oneLevelOnly, compareFunc);

	// Populate to the full list
	_PopulateTree(tree, fFullList, firstIndex, false);

	if (underItem == NULL || (underItem->IsItemVisible() && underItem->IsExpanded())) {
		// Populate to BListView's list
		firstIndex = fList.IndexOf(underItem) + 1;
		lastIndex = firstIndex;
		_PopulateTree(tree, fList, lastIndex, true);

		if (fFirstSelected != -1) {
			// update selection hints
			fFirstSelected = _CalcFirstSelected(0);
			fLastSelected = _CalcLastSelected(CountItems());
		}

		// only invalidate what may have changed
		BRect top = ItemFrame(firstIndex);
		BRect bottom = ItemFrame(lastIndex);
		BRect update(top.left, top.top, bottom.right, bottom.bottom);
		Invalidate(update);
	}

	_DestructTree(tree);
}


void
BOutlineListView::_PopulateTree(BList* tree, BList& target,
	int32& firstIndex, bool onlyVisible)
{
	BListItem** items = (BListItem**)target.Items();
	int32 count = tree->CountItems();

	for (int32 index = 0; index < count; index++) {
		BListItem* item = (BListItem*)tree->ItemAtFast(index);

		items[firstIndex++] = item;

		if (item->HasSubitems() && (!onlyVisible || item->IsExpanded()))
			_PopulateTree(item->fTemporaryList, target, firstIndex, onlyVisible);
	}
}


void
BOutlineListView::_SortTree(BList* tree, bool oneLevelOnly,
	int (*compareFunc)(const BListItem* a, const BListItem* b))
{
	// home-brewn quick sort for our compareFunc
	quick_sort_item_array((BListItem**)tree->Items(), 0, tree->CountItems() - 1,
		compareFunc);

	if (oneLevelOnly)
		return;

	for (int32 index = tree->CountItems(); index-- > 0;) {
		BListItem* item = (BListItem*)tree->ItemAt(index);

		if (item->HasSubitems())
			_SortTree(item->fTemporaryList, false, compareFunc);
	}
}


void
BOutlineListView::_DestructTree(BList* tree)
{
	for (int32 index = tree->CountItems(); index-- > 0;) {
		BListItem* item = (BListItem*)tree->ItemAt(index);

		if (item->HasSubitems())
			_DestructTree(item->fTemporaryList);
	}

	delete tree;
}


BList*
BOutlineListView::_BuildTree(BListItem* underItem, int32& fullIndex)
{
	int32 fullCount = FullListCountItems();
	uint32 level = underItem != NULL ? underItem->OutlineLevel() + 1 : 0;
	BList* list = new BList;
	if (underItem != NULL)
		underItem->fTemporaryList = list;

	while (fullIndex < fullCount) {
		BListItem *item = FullListItemAt(fullIndex);

		// If we jump out of the subtree, break out
		if (item->fLevel < level)
			break;

		if (item->fLevel == level) {
			// If the level matches, put them into the list
			list->AddItem(item);
		}

		fullIndex++;

		if (item->HasSubitems()) {
			// we're going deeper
			_BuildTree(item, fullIndex);
		}
	}

	return list;
}


int32
BOutlineListView::CountItemsUnder(BListItem* underItem,
	bool oneLevelOnly) const
{
	int32 i = IndexOf(underItem);
	if (i == -1)
		return 0;

	int32 count = 0;

	while (i < FullListCountItems()) {
		BListItem *item = FullListItemAt(i);

		// If we jump out of the subtree, return count
		if (item->fLevel < underItem->OutlineLevel())
			return count;

		// If the level matches, increase count
		if (!oneLevelOnly || item->fLevel == underItem->OutlineLevel() + 1)
			count++;

		i++;
	}

	return count;
}


BListItem *
BOutlineListView::EachItemUnder(BListItem *underItem, bool oneLevelOnly,
	BListItem *(*eachFunc)(BListItem* item, void* arg), void* arg)
{
	int32 i = IndexOf(underItem);
	if (i == -1)
		return NULL;

	while (i < FullListCountItems()) {
		BListItem *item = FullListItemAt(i);

		// If we jump out of the subtree, return NULL
		if (item->fLevel < underItem->OutlineLevel())
			return NULL;

		// If the level matches, check the index
		if (!oneLevelOnly || item->fLevel == underItem->OutlineLevel() + 1) {
			item = eachFunc(item, arg);
			if (item != NULL)
				return item;
		}

		i++;
	}

	return NULL;
}


BListItem *
BOutlineListView::ItemUnderAt(BListItem* underItem,
	bool oneLevelOnly, int32 index) const
{
	int32 i = IndexOf(underItem);
	if (i == -1)
		return NULL;

	while (i < FullListCountItems()) {
		BListItem *item = FullListItemAt(i);

		// If we jump out of the subtree, return NULL
		if (item->fLevel < underItem->OutlineLevel())
			return NULL;

		// If the level matches, check the index
		if (!oneLevelOnly || item->fLevel == underItem->OutlineLevel() + 1) {
			if (index == 0)
				return item;

			index--;
		}

		i++;
	}

	return NULL;
}


bool
BOutlineListView::DoMiscellaneous(MiscCode code, MiscData* data)
{
	return BListView::DoMiscellaneous(code, data);
}


void
BOutlineListView::MessageReceived(BMessage* msg)
{
	BListView::MessageReceived(msg);
}


void BOutlineListView::_ReservedOutlineListView1() {}
void BOutlineListView::_ReservedOutlineListView2() {}
void BOutlineListView::_ReservedOutlineListView3() {}
void BOutlineListView::_ReservedOutlineListView4() {}


int32
BOutlineListView::FullListIndex(int32 index) const
{
	BListItem *item = ItemAt(index);

	if (item == NULL)
		return -1;

	return FullListIndexOf(item);
}


int32
BOutlineListView::ListViewIndex(int32 index) const
{
	BListItem *item = ItemAt(index);

	if (item == NULL)
		return -1;

	return BListView::IndexOf(item);
}


void
BOutlineListView::ExpandOrCollapse(BListItem *underItem, bool expand)
{
}


BRect
BOutlineListView::LatchRect(BRect itemRect, int32 level) const
{
	return BRect(itemRect.left, itemRect.top, itemRect.left
		+ (level * 10.0f + 15.0f), itemRect.bottom);
}


void
BOutlineListView::DrawLatch(BRect itemRect, int32 level, bool collapsed,
	bool highlighted, bool misTracked)
{
	float left = level * 10.0f;

	if (collapsed) {
		SetHighColor(192, 192, 192);

		FillTriangle(itemRect.LeftTop() + BPoint(left + 4.0f, 2.0f),
			itemRect.LeftTop() + BPoint(left + 4.0f, 10.0f),
			itemRect.LeftTop() + BPoint(left + 8.0f, 6.0f));

		SetHighColor(0, 0, 0);

		StrokeTriangle(itemRect.LeftTop() + BPoint(left + 4.0f, 2.0f),
			itemRect.LeftTop() + BPoint(left + 4.0f, 10.0f),
			itemRect.LeftTop() + BPoint(left + 8.0f, 6.0f));
	} else {
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


void
BOutlineListView::DrawItem(BListItem* item, BRect itemRect, bool complete)
{
	if (item->fHasSubitems)
		DrawLatch(itemRect, item->fLevel, !item->IsExpanded(), false, false);

	itemRect.left += (item->fLevel) * 10.0f + 15.0f;
	item->DrawItem(this, itemRect, complete);
}


/*!
	\brief Removes a single item from the list and all of its children.

	Unlike the BeOS version, this one will actually delete the children, too,
	as there should be no reference left to them. This may cause problems for
	applications that actually take the misbehaviour of the Be classes into
	account.
*/
BListItem *
BOutlineListView::_RemoveItem(BListItem* item, int32 fullIndex)
{
	if (item == NULL || fullIndex < 0 || fullIndex >= FullListCountItems())
		return NULL;

	uint32 level = item->OutlineLevel();
	int32 superIndex;
	BListItem* super = _SuperitemForIndex(fullIndex, level, &superIndex);

	if (item->IsItemVisible()) {
		// remove children, too
		int32 max = FullListCountItems() - fullIndex - 1;
		BListItem** items = (BListItem**)fFullList.Items() + fullIndex + 1;

		while (max-- > 0) {
			BListItem* subItem = items[0];
			if (subItem->fLevel <= level)
				break;

			if (subItem->IsItemVisible())
				BListView::RemoveItem(subItem);

			fFullList.RemoveItem(fullIndex + 1);

			// TODO: this might be problematic, see comment above
			delete subItem;
		}

		BListView::RemoveItem(item);
	}

	fFullList.RemoveItem(fullIndex);

	if (super != NULL) {
		// we might need to change the fHasSubitems field of the parent
		BListItem* child = FullListItemAt(superIndex + 1);
		if (child == NULL || child->OutlineLevel() <= super->OutlineLevel())
			super->fHasSubitems = false;
	}

	return item;
}


BListItem *
BOutlineListView::RemoveOne(int32 fullListIndex)
{
	return NULL;
}


void
BOutlineListView::TrackInLatchItem(void *)
{
}


void
BOutlineListView::TrackOutLatchItem(void *)
{
}


bool
BOutlineListView::OutlineSwapItems(int32 a, int32 b)
{
	return false;
}


bool
BOutlineListView::OutlineMoveItem(int32 from, int32 to)
{
	return false;
}


bool
BOutlineListView::OutlineReplaceItem(int32 index, BListItem *item)
{
	return false;
}


void
BOutlineListView::CommonMoveItems(int32 from, int32 count, int32 to)
{
}


/*!
	Returns the super item before the item specified by \a fullListIndex
	and \a level.
*/
BListItem *
BOutlineListView::_SuperitemForIndex(int32 fullListIndex, int32 level,
	int32* _superIndex)
{
	BListItem *item;
	fullListIndex--;

	while (fullListIndex >= 0) {
		if ((item = FullListItemAt(fullListIndex))->OutlineLevel()
				< (uint32)level) {
			if (_superIndex != NULL)
				*_superIndex = fullListIndex;
			return item;
		}

		fullListIndex--;
	}

	return NULL;
}


int32
BOutlineListView::_FindPreviousVisibleIndex(int32 fullListIndex)
{
	fullListIndex--;

	while (fullListIndex >= 0) {
		if (FullListItemAt(fullListIndex)->fVisible)
			return fullListIndex;

		fullListIndex--;
	}

	return -1;
}

