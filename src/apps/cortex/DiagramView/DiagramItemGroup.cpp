// DiagramItemGroup.cpp

#include "DiagramItemGroup.h"
#include "DiagramItem.h"

#include <Region.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

DiagramItemGroup::DiagramItemGroup(
	uint32 acceptedTypes,
	bool multiSelection)
	: m_boxes(0),
	  m_wires(0),
	  m_endPoints(0),
	  m_selection(0),
	  m_types(acceptedTypes),
	  m_itemAlignment(1.0, 1.0),
	  m_multiSelection(multiSelection),
	  m_lastItemUnder(0)
{
	D_METHOD(("DiagramItemGroup::DiagramItemGroup()\n"));
	m_selection = new BList(1);
}

DiagramItemGroup::~DiagramItemGroup()
{
	D_METHOD(("DiagramItemGroup::~DiagramItemGroup()\n"));
	if (m_selection)
	{
		m_selection->MakeEmpty();
		delete m_selection;
	}
	if (m_boxes && (m_types & DiagramItem::M_BOX))
	{
		while (countItems(DiagramItem::M_BOX) > 0)
		{
			DiagramItem *item = itemAt(0, DiagramItem::M_BOX);
			if (removeItem(item))
				delete item;
		}
		delete m_boxes;
	}
	if (m_wires && (m_types & DiagramItem::M_WIRE))
	{
		while (countItems(DiagramItem::M_WIRE) > 0)
		{
			DiagramItem *item = itemAt(0, DiagramItem::M_WIRE);
			if (removeItem(item))
				delete item;
		}
		delete m_wires;
	}
	if (m_endPoints && (m_types & DiagramItem::M_ENDPOINT))
	{
		while (countItems(DiagramItem::M_ENDPOINT) > 0)
		{
			DiagramItem *item = itemAt(0, DiagramItem::M_ENDPOINT);
			if (removeItem(item))
				delete item;
		}
		delete m_endPoints;
	}
}

// -------------------------------------------------------- //
// *** item accessors
// -------------------------------------------------------- //

uint32 DiagramItemGroup::countItems(
	uint32 whichType) const
{
	D_METHOD(("DiagramItemGroup::countItems()\n"));
	uint32 count = 0;
	if (whichType & m_types)
	{
		if (whichType & DiagramItem::M_BOX)
		{
			if (m_boxes)
			{
				count += m_boxes->CountItems();
			}
		}
		if (whichType & DiagramItem::M_WIRE)
		{
			if (m_wires)
			{
				count += m_wires->CountItems();
			}
		}
		if (whichType & DiagramItem::M_ENDPOINT)
		{
			if (m_endPoints)
			{
				count += m_endPoints->CountItems();
			}
		}
	}
	return count;
}

DiagramItem *DiagramItemGroup::itemAt(
	uint32 index,
	uint32 whichType) const
{
	D_METHOD(("DiagramItemGroup::itemAt()\n"));
	if (m_types & whichType)
	{
		if (whichType & DiagramItem::M_BOX)
		{
			if (m_boxes && (index < countItems(DiagramItem::M_BOX)))
			{
				return static_cast<DiagramItem *>(m_boxes->ItemAt(index));
			}
			else
			{
				index -= countItems(DiagramItem::M_BOX);
			}
		}
		if (whichType & DiagramItem::M_WIRE)
		{
			if (m_wires && (index < countItems(DiagramItem::M_WIRE)))
			{
				return static_cast<DiagramItem *>(m_wires->ItemAt(index));
			}
			else
			{
				index -= countItems(DiagramItem::M_WIRE);
			}
		}
		if (whichType & DiagramItem::M_ENDPOINT)
		{
			if (m_endPoints && (index < countItems(DiagramItem::M_ENDPOINT)))
			{
				return static_cast<DiagramItem *>(m_endPoints->ItemAt(index));
			}
		}
	}
	return 0;
}

// This function returns the first box or endpoint found that
// contains the given point. For connections it looks at all
// wires that 'might' contain the point and calls their method
// howCloseTo() to find the one closest to the point.
// The lists should be sorted by selection time for proper results!
DiagramItem *DiagramItemGroup::itemUnder(
	BPoint point)
{
	D_METHOD(("DiagramItemGroup::itemUnder()\n"));
	if (m_types & DiagramItem::M_BOX)
	{
		for (uint32 i = 0; i < countItems(DiagramItem::M_BOX); i++)
		{
			DiagramItem *item = itemAt(i, DiagramItem::M_BOX);
			if (item->frame().Contains(point) && (item->howCloseTo(point) == 1.0))
			{
//				DiagramItemGroup *group = dynamic_cast<DiagramItemGroup *>(item);
				return (m_lastItemUnder = item);
			}
		}
	}
	if (m_types & DiagramItem::M_WIRE)
	{
		float closest = 0.0;
		DiagramItem *closestItem = 0;
		for (uint32 i = 0; i < countItems(DiagramItem::M_WIRE); i++)
		{
			DiagramItem *item = itemAt(i, DiagramItem::M_WIRE);
			if (item->frame().Contains(point))
			{
				float howClose = item->howCloseTo(point);
				if (howClose > closest)
				{
					closestItem = item;
					if (howClose == 1.0)
						return (m_lastItemUnder = item);
					closest = howClose;
				}
			}
		}
		if (closest > 0.5)
		{
			return (m_lastItemUnder = closestItem);
		}
	}
	if (m_types & DiagramItem::M_ENDPOINT)
	{
		for (uint32 i = 0; i < countItems(DiagramItem::M_ENDPOINT); i++)
		{
			DiagramItem *item = itemAt(i, DiagramItem::M_ENDPOINT);
			if (item->frame().Contains(point) && (item->howCloseTo(point) == 1.0))
			{
				return (m_lastItemUnder = item);
			}
		}
	}
	return (m_lastItemUnder = 0); // no item was found!
}

// -------------------------------------------------------- //
// *** item operations
// -------------------------------------------------------- //

bool DiagramItemGroup::addItem(
	DiagramItem *item)
{
	D_METHOD(("DiagramItemGroup::addItem()\n"));
	if (item && (m_types & item->type()))
	{
		switch (item->type())
		{
			case DiagramItem::M_BOX:
			{
				if (!m_boxes)
				{
					m_boxes = new BList();
				}
				item->m_group = this;
				return m_boxes->AddItem(static_cast<void *>(item));
			}
			case DiagramItem::M_WIRE:
			{
				if (!m_wires)
				{
					m_wires = new BList();
				}
				item->m_group = this;
				return m_wires->AddItem(static_cast<void *>(item));
			}
			case DiagramItem::M_ENDPOINT:
			{
				if (!m_endPoints)
				{
					m_endPoints = new BList();
				}
				item->m_group = this;
				return m_endPoints->AddItem(static_cast<void *>(item));
			}
		}
	}
	return false;
}

bool DiagramItemGroup::removeItem(
	DiagramItem *item)
{
	D_METHOD(("DiagramItemGroup::removeItem()\n"));
	if (item && (m_types & item->type()))
	{
		// reset the lastItemUnder-pointer if it pointed to this item
		if (m_lastItemUnder == item)
		{
			m_lastItemUnder = 0;
		}
		// remove it from the selection list if it was selected
		if (item->isSelected())
		{
			m_selection->RemoveItem(static_cast<void *>(item));
		}
		// try to remove the item from its list
		switch (item->type())
		{
			case DiagramItem::M_BOX:
			{
				if (m_boxes)
				{
					item->m_group = 0;
					return m_boxes->RemoveItem(static_cast<void *>(item));
				}
			}
			case DiagramItem::M_WIRE:
			{
				if (m_wires)
				{
					item->m_group = 0;
					return m_wires->RemoveItem(static_cast<void *>(item));
				}
			}
			case DiagramItem::M_ENDPOINT:
			{
				if (m_endPoints)
				{
					item->m_group = 0;
					return m_endPoints->RemoveItem(static_cast<void *>(item));
				}
			}
		}
	}
	return false;
}

void DiagramItemGroup::sortItems(
	uint32 whichType,
	int (*compareFunc)(const void *, const void *))
{
	D_METHOD(("DiagramItemGroup::sortItems()\n"));
	if ((whichType != DiagramItem::M_ANY) && (m_types & whichType))
	{
		switch (whichType)
		{
			case DiagramItem::M_BOX:
			{
				if (m_boxes)
				{
					m_boxes->SortItems(compareFunc);
				}
				break;
			}
			case DiagramItem::M_WIRE:
			{
				if (m_wires)
				{
					m_wires->SortItems(compareFunc);
				}
				break;
			}
			case DiagramItem::M_ENDPOINT:
			{
				if (m_endPoints)
				{
					m_endPoints->SortItems(compareFunc);
				}
				break;
			}
		}
	}
}

// items are drawn in reverse order; they should be sorted by
// selection time before this function gets called, so that
// the more recently selected item are drawn above others
void DiagramItemGroup::drawItems(
	BRect updateRect,
	uint32 whichType,
	BRegion *updateRegion)
{
	D_METHOD(("DiagramItemGroup::drawItems()\n"));
	if (whichType & DiagramItem::M_WIRE)
	{
		for (int32 i = countItems(DiagramItem::M_WIRE) - 1; i >= 0; i--)
		{
			DiagramItem *item = itemAt(i, DiagramItem::M_WIRE);
			if (item->frame().Intersects(updateRect))
			{	
				item->draw(updateRect);
			}
		}
	}
	if (whichType & DiagramItem::M_BOX)
	{
		for (int32 i = countItems(DiagramItem::M_BOX) - 1; i >= 0; i--)
		{
			DiagramItem *item = itemAt(i, DiagramItem::M_BOX);
			if (item && item->frame().Intersects(updateRect))
			{	
				item->draw(updateRect);
				if (updateRegion)
					updateRegion->Exclude(item->frame());
			}
		}
	}
	if (whichType & DiagramItem::M_ENDPOINT)
	{
		for (int32 i = countItems(DiagramItem::M_ENDPOINT) - 1; i >= 0; i--)
		{
			DiagramItem *item = itemAt(i, DiagramItem::M_ENDPOINT);
			if (item && item->frame().Intersects(updateRect))
			{	
				item->draw(updateRect);
			}
		}
	}
}

bool DiagramItemGroup::getClippingAbove(
	DiagramItem *which,
	BRegion *region)
{
	D_METHOD(("DiagramItemGroup::getClippingAbove()\n"));
	bool found = false;
	if (which && region)
	{
		switch (which->type())
		{
			case DiagramItem::M_BOX:
			{
				int32 index = m_boxes->IndexOf(which);
				if (index >= 0) // the item was found
				{
					BRect r = which->frame();
					for (int32 i = 0; i < index; i++)
					{
						DiagramItem *item = itemAt(i, DiagramItem::M_BOX);
						if (item && item->frame().Intersects(r))
						{
							region->Include(item->frame() & r);
							found = true;
						}
					}
				}
				break;
			}
			case DiagramItem::M_WIRE:
			{
				BRect r = which->frame();
				for (uint32 i = 0; i < countItems(DiagramItem::M_BOX); i++)
				{
					DiagramItem *item = itemAt(i, DiagramItem::M_BOX);
					if (item && item->frame().Intersects(r))
					{
						region->Include(item->frame() & r);
						found = true;
					}
				}
				break;
			}
		}
	}
	return found;
}

// -------------------------------------------------------- //
// *** selection accessors
// -------------------------------------------------------- //

uint32 DiagramItemGroup::selectedType() const
{
	D_METHOD(("DiagramItemGroup::selectedType()\n"));
	if (countSelectedItems() > 0)
	{
		return selectedItemAt(0)->type();
	}
	return 0;
}

uint32 DiagramItemGroup::countSelectedItems() const
{
	D_METHOD(("DiagramItemGroup::countSelectedItems()\n"));
	if (m_selection)
	{
		return m_selection->CountItems();
	}
	return 0;
}

DiagramItem *DiagramItemGroup::selectedItemAt(
	uint32 index) const
{
	D_METHOD(("DiagramItemGroup::selectedItemAt()\n"));
	if (m_selection)
	{
		return static_cast<DiagramItem *>(m_selection->ItemAt(index));
	}
	return 0;
}

// -------------------------------------------------------- //
// *** selection related operations
// -------------------------------------------------------- //

// selects an item, optionally replacing the complete former
// selection. If the type of the item to be selected differs
// from the type of items currently selected, this methods
// automatically replaces the former selection
bool DiagramItemGroup::selectItem(
	DiagramItem *which,
	bool deselectOthers)
{
	D_METHOD(("DiagramItemGroup::selectItem()\n"));
	bool selectionChanged = false;
	if (which && !which->isSelected() && which->isSelectable())
	{
		// check if the item's type is the same as of the other
		// selected items
		if (m_multiSelection)
		{
			if (which->type() != selectedType())
			{
				deselectOthers = true;
			}
		}

		// check if the former selection has to be deselected
		if (deselectOthers || !m_multiSelection)
		{
			while (countSelectedItems() > 0)
			{
				deselectItem(selectedItemAt(0));
			}
		}

		// select the item
		if (deselectOthers || countSelectedItems() == 0)
		{
			which->select();
		}
		else
		{
			which->selectAdding();
		}
		m_selection->AddItem(which);
		selectionChanged = true;
	}

	// resort the lists if necessary
	if (selectionChanged)
	{
		sortItems(which->type(), compareSelectionTime);
		sortSelectedItems(compareSelectionTime);
		return true;
	}
	return false;
}

bool DiagramItemGroup::deselectItem(
	DiagramItem *which)
{
	D_METHOD(("DiagramItemGroup::deselectItem()\n"));
	if (which && which->isSelected())
	{
		m_selection->RemoveItem(which);
		which->deselect();
		sortItems(which->type(), compareSelectionTime);
		sortSelectedItems(compareSelectionTime);
		return true;
	}
	return false;
}

bool DiagramItemGroup::selectAll(
	uint32 itemType)
{
	D_METHOD(("DiagramItemGroup::selectAll()\n"));
	bool selectionChanged = false;
	if (m_types & itemType)
	{
		for (int32 i = 0; i < countItems(itemType); i++)
		{
			if (selectItem(itemAt(i, itemType), false))
				selectionChanged = true;
		}
	}
	return selectionChanged;
}

bool DiagramItemGroup::deselectAll(
	uint32 itemType)
{
	D_METHOD(("DiagramItemGroup::deselectAll()\n"));
	bool selectionChanged = false;
	if (m_types & itemType)
	{
		for (int32 i = 0; i < countItems(itemType); i++)
		{
			if (deselectItem(itemAt(i, itemType)))
				selectionChanged = true;
		}
	}
	return selectionChanged;
}

void DiagramItemGroup::sortSelectedItems(
	int (*compareFunc)(const void *, const void *))
{
	D_METHOD(("DiagramItemGroup::sortSelectedItems()\n"));
	m_selection->SortItems(compareFunc);
}

void DiagramItemGroup::dragSelectionBy(
	float x,
	float y,
	BRegion *updateRegion)
{
	D_METHOD(("DiagramItemGroup::dragSelectionBy()\n"));
	if (selectedType() == DiagramItem::M_BOX)
	{
		align(&x, &y);
		if ((x != 0) || (y != 0))
		{
			for (int32 i = countSelectedItems() - 1; i >= 0; i--)
			{
				DiagramItem *item = dynamic_cast<DiagramItem *>(selectedItemAt(i));
				if (item->isDraggable())
				{
					item->moveBy(x, y, updateRegion);
				}
			}
		}
	}
}

void DiagramItemGroup::removeSelection()
{
	D_METHOD(("DiagramItemGroup::removeSelection()\n"));
	for (int32 i = 0; i < countSelectedItems(); i++)
	{
		removeItem(selectedItemAt(i));
	}
}

// -------------------------------------------------------- //
// *** alignment related accessors & operations
// -------------------------------------------------------- //

void DiagramItemGroup::getItemAlignment(
	float *horizontal,
	float *vertical)
{
	D_METHOD(("DiagramItemGroup::getItemAlignment()\n"));
	if (horizontal)
		*horizontal = m_itemAlignment.x;
	if (vertical)
		*vertical = m_itemAlignment.y;
}

void DiagramItemGroup::align(
	float *x,
	float *y) const
{
	D_METHOD(("DiagramItemGroup::align()\n"));
	*x = ((int)*x / (int)m_itemAlignment.x) * m_itemAlignment.x;
	*y = ((int)*y / (int)m_itemAlignment.y) * m_itemAlignment.y;
}

BPoint DiagramItemGroup::align(
	BPoint point) const
{
	D_METHOD(("DiagramItemGroup::align()\n"));
	float x = point.x, y = point.y;
	align(&x, &y);
	return BPoint(x, y);
}

// END -- DiagramItemGroup.cpp --
