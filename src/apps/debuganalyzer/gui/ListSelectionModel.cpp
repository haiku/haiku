/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ListSelectionModel.h"


// #pragma mark - ListSelectionModel


ListSelectionModel::ListSelectionModel()
{
}


ListSelectionModel::ListSelectionModel(const ListSelectionModel& other)
{
	*this = other;
}


ListSelectionModel::~ListSelectionModel()
{
}


void
ListSelectionModel::Clear()
{
	int32 selectedCount = fSelectedItems.Count();
	if (selectedCount > 0) {
		int32 firstSelected = fSelectedItems[0];
		int32 lastSelected = fSelectedItems[selectedCount - 1];

		fSelectedItems.Clear();

		_NotifyItemsDeselected(firstSelected, lastSelected - firstSelected + 1);
	}
}


bool
ListSelectionModel::SelectItems(int32 itemIndex, int32 count,
	bool extendSelection)
{
	int32 endItemIndex = itemIndex + count;

	int32 index;
	if (extendSelection) {
		if (count <= 0)
			return true;

		index = _FindItem(itemIndex);

		// count already selected items
		int32 alreadySelectedCount = _CountSelectedItemsInRange(index,
			endItemIndex);
		if (alreadySelectedCount == count)
			return true;

		// make room for the new items
		if (!fSelectedItems.InsertUninitialized(index + alreadySelectedCount,
				count - alreadySelectedCount)) {
			return false;
		}
	} else {
		// TODO: Don't clear -- just resize to the right size!
		Clear();
		if (count <= 0)
			return true;

		index = 0;
		if (!fSelectedItems.AddUninitialized(count))
			return false;
	}

	for (int32 i = 0; i < count; i++)
		fSelectedItems[index + i] = itemIndex + i;

	_NotifyItemsSelected(itemIndex, count);

	return true;
}


void
ListSelectionModel::DeselectItems(int32 itemIndex, int32 count)
{
	int32 endItemIndex = itemIndex + count;
	int32 index = _FindItem(itemIndex);

	// count actually selected items
	int32 actuallySelectedCount = _CountSelectedItemsInRange(index,
		endItemIndex);
	if (actuallySelectedCount == 0)
		return;

	fSelectedItems.Remove(index, actuallySelectedCount);

	_NotifyItemsDeselected(itemIndex, count);
}


void
ListSelectionModel::ItemsAdded(int32 itemIndex, int32 count)
{
	if (count <= 0)
		return;

	// re-index following items
	int32 index = _FindItem(itemIndex);
	int32 selectedCount = fSelectedItems.Count();
	for (int32 i = index; i < selectedCount; i++)
		fSelectedItems[i] += count;
}


void
ListSelectionModel::ItemsRemoved(int32 itemIndex, int32 count)
{
	if (count <= 0)
		return;

	int32 index = _FindItem(itemIndex);

	// count selected items in the range
	int32 actuallySelectedCount = _CountSelectedItemsInRange(index,
		itemIndex + count);
	if (actuallySelectedCount > 0)
		fSelectedItems.Remove(index, actuallySelectedCount);

	// re-index following items
	int32 selectedCount = fSelectedItems.Count();
	for (int32 i = index; i < selectedCount; i++)
		fSelectedItems[i] -= count;
}


bool
ListSelectionModel::AddListener(Listener* listener)
{
	return fListeners.AddItem(listener);
}


void
ListSelectionModel::RemoveListener(Listener* listener)
{
	fListeners.RemoveItem(listener);
}


ListSelectionModel&
ListSelectionModel::operator=(const ListSelectionModel& other)
{
	Clear();

	fSelectedItems = other.fSelectedItems;

	int32 selectedCount = CountSelectedItems();
	if (selectedCount > 0) {
		int32 firstSelected = fSelectedItems[0];
		int32 lastSelected = fSelectedItems[selectedCount - 1];
		_NotifyItemsDeselected(firstSelected, lastSelected - firstSelected + 1);
	}

	return *this;
}


int32
ListSelectionModel::_FindItem(int32 itemIndex) const
{
	// binary search the index of the first item >= itemIndex
	int32 lower = 0;
	int32 upper = fSelectedItems.Count();

	while (lower < upper) {
		int32 mid = (lower + upper) / 2;

		if (fSelectedItems[mid] < itemIndex)
			lower = mid + 1;
		else
			upper = mid;
	}

	return lower;
}


int32
ListSelectionModel::_CountSelectedItemsInRange(int32 index,
	int32 endItemIndex) const
{
	int32 count = 0;
	int32 selectedCount = fSelectedItems.Count();
	for (int32 i = index; i < selectedCount; i++) {
		if (SelectedItemAt(i) >= endItemIndex)
			break;
		count++;
	}

	return count;
}


void
ListSelectionModel::_NotifyItemsSelected(int32 index, int32 count)
{
	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--)
		fListeners.ItemAt(i)->ItemsSelected(this, index, count);
}


void
ListSelectionModel::_NotifyItemsDeselected(int32 index, int32 count)
{
	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--)
		fListeners.ItemAt(i)->ItemsDeselected(this, index, count);
}


// #pragma mark - Listener


ListSelectionModel::Listener::~Listener()
{
}


void
ListSelectionModel::Listener::ItemsSelected(ListSelectionModel* model,
	int32 index, int32 count)
{
}


void
ListSelectionModel::Listener::ItemsDeselected(ListSelectionModel* model,
	int32 index, int32 count)
{
}
