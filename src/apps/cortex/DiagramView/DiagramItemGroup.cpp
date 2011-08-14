/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// DiagramItemGroup.cpp

/*! \class DiagramItemGroup.
	\brief Basic class for managing and accessing DiagramItem objects.

	Objects of this class can manage one or more of the DiagramItem
	type M_BOX, M_WIRE and M_ENDPOINT. Many methods let you specify
	which type of item you want to deal with.
*/

#include "DiagramItemGroup.h"
#include "DiagramItem.h"

#include <Region.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)


DiagramItemGroup::DiagramItemGroup(uint32 acceptedTypes, bool multiSelection)
	:fBoxes(0),
	fWires(0),
	fEndPoints(0),
	fSelection(0),
	fTypes(acceptedTypes),
	fItemAlignment(1.0, 1.0),
	fMultiSelection(multiSelection),
	fLastItemUnder(0)
{
	D_METHOD(("DiagramItemGroup::DiagramItemGroup()\n"));
	fSelection = new BList(1);
}


DiagramItemGroup::~DiagramItemGroup()
{
	D_METHOD(("DiagramItemGroup::~DiagramItemGroup()\n"));

	int32 count = 0;
	if (fWires && (fTypes & DiagramItem::M_WIRE)) {
		count = fWires->CountItems();
		for (int32 i = 0; i < count; ++i)
			delete static_cast<DiagramItem*>(fWires->ItemAt(i));
		delete fWires;
	}

	if (fBoxes && (fTypes & DiagramItem::M_BOX)) {
		count = fBoxes->CountItems();
		for (int32 i = 0; i < count; ++i)
			delete static_cast<DiagramItem*>(fBoxes->ItemAt(i));
		delete fBoxes;
	}

	if (fEndPoints && (fTypes & DiagramItem::M_ENDPOINT)) {
		count = fEndPoints->CountItems();
		for (int32 i = 0; i < count; ++i)
			delete static_cast<DiagramItem*>(fEndPoints->ItemAt(i));
		delete fEndPoints;
	}

	if (fSelection)
		delete fSelection;
}


//	#pragma mark - item accessors


/*! Returns the number of items in the group (optionally only those
	of the given type \param whichType)
*/
uint32
DiagramItemGroup::CountItems(uint32 whichType) const
{
	D_METHOD(("DiagramItemGroup::CountItems()\n"));
	uint32 count = 0;
	if (whichType & fTypes) {
		if (whichType & DiagramItem::M_BOX) {
			if (fBoxes)
				count += fBoxes->CountItems();
		}

		if (whichType & DiagramItem::M_WIRE) {
			if (fWires)
				count += fWires->CountItems();
		}

		if (whichType & DiagramItem::M_ENDPOINT) {
			if (fEndPoints)
				count += fEndPoints->CountItems();
		}
	}

	return count;
}


/*! Returns a pointer to the item in the lists which is
	at the given index; if none is found, this function
	returns 0
*/
DiagramItem*
DiagramItemGroup::ItemAt(uint32 index, uint32 whichType) const
{
	D_METHOD(("DiagramItemGroup::ItemAt()\n"));
	if (fTypes & whichType) {
		if (whichType & DiagramItem::M_BOX) {
			if (fBoxes && (index < CountItems(DiagramItem::M_BOX)))
				return static_cast<DiagramItem *>(fBoxes->ItemAt(index));
			else
				index -= CountItems(DiagramItem::M_BOX);
		}

		if (whichType & DiagramItem::M_WIRE) {
			if (fWires && (index < CountItems(DiagramItem::M_WIRE)))
				return static_cast<DiagramItem *>(fWires->ItemAt(index));
			else
				index -= CountItems(DiagramItem::M_WIRE);
		}

		if (whichType & DiagramItem::M_ENDPOINT) {
			if (fEndPoints && (index < CountItems(DiagramItem::M_ENDPOINT)))
				return static_cast<DiagramItem *>(fEndPoints->ItemAt(index));
		}
	}

	return 0;
}


/*! This function returns the first box or endpoint found that
	contains the given \param point. For connections it looks at all
	wires that 'might' contain the point and calls their method
	howCloseTo() to find the one closest to the point.
	The lists should be sorted by selection time for proper results!
*/
DiagramItem*
DiagramItemGroup::ItemUnder(BPoint point)
{
	D_METHOD(("DiagramItemGroup::ItemUnder()\n"));
	if (fTypes & DiagramItem::M_BOX) {
		for (uint32 i = 0; i < CountItems(DiagramItem::M_BOX); i++) {
			DiagramItem *item = ItemAt(i, DiagramItem::M_BOX);
			if (item->Frame().Contains(point) && (item->howCloseTo(point) == 1.0)) {
				// DiagramItemGroup *group = dynamic_cast<DiagramItemGroup *>(item);
				return (fLastItemUnder = item);
			}
		}
	}

	if (fTypes & DiagramItem::M_WIRE) {
		float closest = 0.0;
		DiagramItem *closestItem = 0;
		for (uint32 i = 0; i < CountItems(DiagramItem::M_WIRE); i++) {
			DiagramItem *item = ItemAt(i, DiagramItem::M_WIRE);
			if (item->Frame().Contains(point)) {
				float howClose = item->howCloseTo(point);
				if (howClose > closest) {
					closestItem = item;
					if (howClose == 1.0)
						return (fLastItemUnder = item);
					closest = howClose;
				}
			}
		}

		if (closest > 0.5)
			return (fLastItemUnder = closestItem);
	}

	if (fTypes & DiagramItem::M_ENDPOINT) {
		for (uint32 i = 0; i < CountItems(DiagramItem::M_ENDPOINT); i++) {
			DiagramItem *item = ItemAt(i, DiagramItem::M_ENDPOINT);
			if (item->Frame().Contains(point) && (item->howCloseTo(point) == 1.0))
				return (fLastItemUnder = item);
		}
	}

	return (fLastItemUnder = 0); // no item was found!
}


//	#pragma mark - item operations


//! Adds an \param item to the group; returns true on success.
bool
DiagramItemGroup::AddItem(DiagramItem *item)
{
	D_METHOD(("DiagramItemGroup::AddItem()\n"));
	if (item && (fTypes & item->type())) {
		if (item->m_group)
			item->m_group->RemoveItem(item);

		switch (item->type()) {
			case DiagramItem::M_BOX:
				if (!fBoxes)
					fBoxes = new BList();
				item->m_group = this;
				return fBoxes->AddItem(static_cast<void *>(item));

			case DiagramItem::M_WIRE:
				if (!fWires)
					fWires = new BList();
				item->m_group = this;
				return fWires->AddItem(static_cast<void *>(item));

			case DiagramItem::M_ENDPOINT:
				if (!fEndPoints)
					fEndPoints = new BList();
				item->m_group = this;
				return fEndPoints->AddItem(static_cast<void *>(item));
		}
	}

	return false;
}


//! Removes an \param item from the group; returns true on success.
bool
DiagramItemGroup::RemoveItem(DiagramItem* item)
{
	D_METHOD(("DiagramItemGroup::RemoveItem()\n"));
	if (item && (fTypes & item->type())) {
		// reset the lastItemUnder-pointer if it pointed to this item
		if (fLastItemUnder == item)
			fLastItemUnder = 0;

		// remove it from the selection list if it was selected
		if (item->isSelected())
			fSelection->RemoveItem(static_cast<void *>(item));

		// try to remove the item from its list
		switch (item->type()) {
			case DiagramItem::M_BOX:
				if (fBoxes) {
					item->m_group = 0;
					return fBoxes->RemoveItem(static_cast<void *>(item));
				}
				break;

			case DiagramItem::M_WIRE:
				if (fWires) {
					item->m_group = 0;
					return fWires->RemoveItem(static_cast<void *>(item));
				}
				break;

			case DiagramItem::M_ENDPOINT:
				if (fEndPoints) {
					item->m_group = 0;
					return fEndPoints->RemoveItem(static_cast<void *>(item));
				}
		}
	}

	return false;
}


/*! Performs a quicksort on a list of items with the provided
	compare function (one is already defined in the DiagramItem
	implementation); can't handle more than one item type at a
	time!
*/
void
DiagramItemGroup::SortItems(uint32 whichType,
	int (*compareFunc)(const void *, const void *))
{
	D_METHOD(("DiagramItemGroup::SortItems()\n"));
	if ((whichType != DiagramItem::M_ANY) && (fTypes & whichType)) {
		switch (whichType) {
			case DiagramItem::M_BOX:
				if (fBoxes)
					fBoxes->SortItems(compareFunc);
				break;

			case DiagramItem::M_WIRE:
				if (fWires)
					fWires->SortItems(compareFunc);
				break;

			case DiagramItem::M_ENDPOINT:
				if (fEndPoints)
					fEndPoints->SortItems(compareFunc);
				break;
		}
	}
}


/*! Fires a Draw() command at all items of a specific type that
	intersect with the \param updateRect;
	items are drawn in reverse order; they should be sorted by
	selection time before this function gets called, so that
	the more recently selected item are drawn above others.
*/
void
DiagramItemGroup::DrawItems(BRect updateRect, uint32 whichType, BRegion* updateRegion)
{
	D_METHOD(("DiagramItemGroup::DrawItems()\n"));
	if (whichType & DiagramItem::M_WIRE) {
		for (int32 i = CountItems(DiagramItem::M_WIRE) - 1; i >= 0; i--) {
			DiagramItem *item = ItemAt(i, DiagramItem::M_WIRE);
			if (item->Frame().Intersects(updateRect))
				item->Draw(updateRect);
		}
	}

	if (whichType & DiagramItem::M_BOX) {
		for (int32 i = CountItems(DiagramItem::M_BOX) - 1; i >= 0; i--) {
			DiagramItem *item = ItemAt(i, DiagramItem::M_BOX);
			if (item && item->Frame().Intersects(updateRect)) {
				item->Draw(updateRect);
				if (updateRegion)
					updateRegion->Exclude(item->Frame());
			}
		}
	}

	if (whichType & DiagramItem::M_ENDPOINT) {
		for (int32 i = CountItems(DiagramItem::M_ENDPOINT) - 1; i >= 0; i--) {
			DiagramItem *item = ItemAt(i, DiagramItem::M_ENDPOINT);
			if (item && item->Frame().Intersects(updateRect))
				item->Draw(updateRect);
		}
	}
}


/*!	Returns in outRegion the \param region of items that lay "over" the given
	DiagramItem in \param which; returns false if no items are above or the item
	doesn't exist.
*/
bool
DiagramItemGroup::GetClippingAbove(DiagramItem *which, BRegion *region)
{
	D_METHOD(("DiagramItemGroup::GetClippingAbove()\n"));
	bool found = false;
	if (which && region) {
		switch (which->type()) {
			case DiagramItem::M_BOX:
			{
				int32 index = fBoxes->IndexOf(which);
				if (index >= 0) { // the item was found
					BRect r = which->Frame();
					for (int32 i = 0; i < index; i++) {
						DiagramItem *item = ItemAt(i, DiagramItem::M_BOX);
						if (item && item->Frame().Intersects(r)) {
							region->Include(item->Frame() & r);
							found = true;
						}
					}
				}
				break;
			}

			case DiagramItem::M_WIRE:
			{
				BRect r = which->Frame();
				for (uint32 i = 0; i < CountItems(DiagramItem::M_BOX); i++) {
					DiagramItem *item = ItemAt(i, DiagramItem::M_BOX);
					if (item && item->Frame().Intersects(r)) {
						region->Include(item->Frame() & r);
						found = true;
					}
				}
				break;
			}
		}
	}

	return found;
}


//	#pragma mark - selection accessors


/*!	Returns the type of DiagramItems in the current selection
	(currently only one type at a time is supported!)
*/
uint32
DiagramItemGroup::SelectedType() const
{
	D_METHOD(("DiagramItemGroup::SelectedType()\n"));
	if (CountSelectedItems() > 0)
		return SelectedItemAt(0)->type();

	return 0;
}


//!	Returns the number of items in the current selection
uint32
DiagramItemGroup::CountSelectedItems() const
{
	D_METHOD(("DiagramItemGroup::CountSelectedItems()\n"));
	if (fSelection)
		return fSelection->CountItems();

	return 0;
}


/*!	Returns a pointer to the item in the list which is
	at the given \param index; if none is found, this function
	returns 0
*/
DiagramItem*
DiagramItemGroup::SelectedItemAt(uint32 index) const
{
	D_METHOD(("DiagramItemGroup::SelectedItemAt()\n"));
	if (fSelection)
		return static_cast<DiagramItem *>(fSelection->ItemAt(index));

	return 0;
}


//	#pragma mark - selection related operations


/*!	Selects an item, optionally replacing the complete former
	selection. If the type of the item to be selected differs
	from the type of items currently selected, this methods
	automatically replaces the former selection
*/
bool
DiagramItemGroup::SelectItem(DiagramItem* which, bool deselectOthers)
{
	D_METHOD(("DiagramItemGroup::SelectItem()\n"));
	bool selectionChanged = false;
	if (which && !which->isSelected() && which->isSelectable()) {
		// check if the item's type is the same as of the other
		// selected items
		if (fMultiSelection) {
			if (which->type() != SelectedType())
				deselectOthers = true;
		}

		// check if the former selection has to be deselected
		if (deselectOthers || !fMultiSelection) {
			while (CountSelectedItems() > 0)
				DeselectItem(SelectedItemAt(0));
		}

		// select the item
		if (deselectOthers || CountSelectedItems() == 0)
			which->select();
		else
			which->selectAdding();

		fSelection->AddItem(which);
		selectionChanged = true;
	}

	// resort the lists if necessary
	if (selectionChanged) {
		SortItems(which->type(), compareSelectionTime);
		SortSelectedItems(compareSelectionTime);
		return true;
	}

	return false;
}


//!	Simply deselects one item
bool
DiagramItemGroup::DeselectItem(DiagramItem* which)
{
	D_METHOD(("DiagramItemGroup::DeselectItem()\n"));
	if (which && which->isSelected()) {
		fSelection->RemoveItem(which);
		which->deselect();
		SortItems(which->type(), compareSelectionTime);
		SortSelectedItems(compareSelectionTime);
		return true;
	}

	return false;
}


//! Selects all items of the given \param itemType
bool
DiagramItemGroup::SelectAll(uint32 itemType)
{
	D_METHOD(("DiagramItemGroup::SelectAll()\n"));
	bool selectionChanged = false;
	if (fTypes & itemType) {
		for (uint32 i = 0; i < CountItems(itemType); i++) {
			if (SelectItem(ItemAt(i, itemType), false))
				selectionChanged = true;
		}
	}

	return selectionChanged;
}


//! Deselects all items of the given \param itemType
bool
DiagramItemGroup::DeselectAll(uint32 itemType)
{
	D_METHOD(("DiagramItemGroup::DeselectAll()\n"));
	bool selectionChanged = false;
	if (fTypes & itemType) {
		for (uint32 i = 0; i < CountItems(itemType); i++) {
			if (DeselectItem(ItemAt(i, itemType)))
				selectionChanged = true;
		}
	}

	return selectionChanged;
}


/*!	Performs a quicksort on the list of selected items with the
	provided compare function (one is already defined in the DiagramItem
	implementation)
*/
void
DiagramItemGroup::SortSelectedItems(int (*compareFunc)(const void *, const void *))
{
	D_METHOD(("DiagramItemGroup::SortSelectedItems()\n"));
	fSelection->SortItems(compareFunc);
}


/*!	Moves all selected items by a given amount, taking
	item alignment into account; in updateRegion the areas
	that still require updating by the caller are returned
*/
void
DiagramItemGroup::DragSelectionBy(float x, float y, BRegion* updateRegion)
{
	D_METHOD(("DiagramItemGroup::DragSelectionBy()\n"));
	if (SelectedType() == DiagramItem::M_BOX) {
		Align(&x, &y);
		if ((x != 0) || (y != 0)) {
			for (int32 i = CountSelectedItems() - 1; i >= 0; i--) {
				DiagramItem *item = dynamic_cast<DiagramItem *>(SelectedItemAt(i));
				if (item->isDraggable())
					item->MoveBy(x, y, updateRegion);
			}
		}
	}
}


//!	Removes all selected items from the group
void
DiagramItemGroup::RemoveSelection()
{
	D_METHOD(("DiagramItemGroup::RemoveSelection()\n"));
	for (uint32 i = 0; i < CountSelectedItems(); i++)
		RemoveItem(SelectedItemAt(i));
}


//	#pragma mark - alignment related accessors & operations


void
DiagramItemGroup::GetItemAlignment(float *horizontal, float *vertical)
{
	D_METHOD(("DiagramItemGroup::GetItemAlignment()\n"));
	if (horizontal)
		*horizontal = fItemAlignment.x;
	if (vertical)
		*vertical = fItemAlignment.y;
}


//! Align a given point(\param x, \param y) to the current grid
void
DiagramItemGroup::Align(float *x, float *y) const
{
	D_METHOD(("DiagramItemGroup::Align()\n"));
	*x = ((int)*x / (int)fItemAlignment.x) * fItemAlignment.x;
	*y = ((int)*y / (int)fItemAlignment.y) * fItemAlignment.y;
}


//! Align a given \param point to the current grid
BPoint
DiagramItemGroup::Align(BPoint point) const
{
	D_METHOD(("DiagramItemGroup::Align()\n"));
	float x = point.x, y = point.y;
	Align(&x, &y);
	return BPoint(x, y);
}
