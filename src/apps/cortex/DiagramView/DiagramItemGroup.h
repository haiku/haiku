// DiagramItemGroup.h (Cortex/DiagramView)
//
// * PURPOSE
//   Basic class for managing and accessing DiagramItem objects.
//   Objects of this class can manage one or more of the DiagramItem
//   type M_BOX, M_WIRE and M_ENDPOINT. Many methods let you specify
//   which type of item you want to deal with.
//
// * HISTORY
//   c.lenz		28sep99		Begun
//

#ifndef __DiagramItemGroup_H__
#define __DiagramItemGroup_H__

#include "DiagramItem.h"

#include <List.h>
#include <Point.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class DiagramItemGroup
{

public:					// *** ctor/dtor

						DiagramItemGroup(
							uint32 acceptedTypes,
							bool multiSelection = true);

	virtual				~DiagramItemGroup();

public:					// *** hook functions

	// is called whenever the current selection has changed
	virtual void		selectionChanged()
						{ /*does nothing */ }

public:				// *** item accessors

	// returns the number of items in the group (optionally only those
	// of the given type)
	uint32			countItems(
							uint32 whichType = DiagramItem::M_ANY) const;
	
	// returns a pointer to the item in the lists which is
	// at the given index; if none is found, this function
	// returns 0
	DiagramItem		   *itemAt(
							uint32 index,
							uint32 whichType = DiagramItem::M_ANY) const;

	// returns a pointer to the item that is currently under
	// the given point, and 0 if there is none
	DiagramItem		   *itemUnder(
							BPoint point);

public:					// *** item operations

	// adds an item to the group; returns true on success
	virtual bool		addItem(
							DiagramItem *item);

	// removes an item from the group; returns true on success
	bool				removeItem(
							DiagramItem *item);

	// performs a quicksort on a list of items with the provided
	// compare function (one is already defined in the DiagramItem
	// implementation); can't handle more than one item type at a
	// time!
	void				sortItems(
							uint32 itemType,
							int (*compareFunc)(const void *, const void *));

	// fires a draw() command at all items of a specific type that
	// intersect with the updateRect
	void				drawItems(
							BRect updateRect,
							uint32 whichType = DiagramItem::M_ANY,
							BRegion *updateRegion = 0);

	// returns in outRegion the region of items that lay "over" the given
	// DiagramItem in which; returns false if no items are above or the item
	// doesn't exist
	bool				getClippingAbove(
							DiagramItem *which,
							BRegion *outRegion);

public:					// *** selection accessors

	// returns the type of DiagramItems in the current selection
	// (currently only one type at a time is supported!)
	uint32			selectedType() const;
	
	// returns the number of items in the current selection
	uint32			countSelectedItems() const;
	
	// returns a pointer to the item in the list which is
	// at the given index; if none is found, this function
	// returns 0
	DiagramItem		   *selectedItemAt(
							uint32 index) const;
	
	// returns the ability of the group to handle multiple selections
	// as set in the constructor
	bool				multipleSelection() const
						{ return m_multiSelection; }						

public:					// *** selection related operations

	// selects the given item and optionally deselects all others
	// (thereby replacing the former selection)
	bool				selectItem(
							DiagramItem *which,
							bool deselectOthers = true);

	// simply deselects one item
	bool				deselectItem(
							DiagramItem *which);

	// performs a quicksort on the list of selected items with the
	// provided compare function (one is already defined in the DiagramItem
	// implementation)
	void				sortSelectedItems(
							int (*compareFunc)(const void *, const void *));

	// select all items of the given type
	bool				selectAll(
							uint32 itemType = DiagramItem::M_ANY);

	// deselect all items of the given type
	bool				deselectAll(
							uint32 itemType = DiagramItem::M_ANY);

	// moves all selected items by a given amount, taking
	// item alignment into account; in updateRegion the areas
	// that still require updating by the caller are returned
	void				dragSelectionBy(
							float x,
							float y,
							BRegion *updateRegion);

	// removes all selected items from the group
	void				removeSelection();

public:					// *** alignment related

	// set/get the 'item alignment' grid size; this is used when
	// items are being dragged and inserted
	void				setItemAlignment(
							float horizontal,
							float vertical)
						{ m_itemAlignment.Set(horizontal, vertical); }
	void				getItemAlignment(
							float *horizontal,
							float *vertical);

	// align a given point to the current grid
	void				align(
							float *x,
							float *y) const;
	BPoint				align(
							BPoint point) const;

protected:				// *** accessors

	// returns a pointer to the last item returned by the itemUnder()
	// function; this can be used by deriving classes to implement a
	// transit system
	DiagramItem		   *lastItemUnder()
						{ return m_lastItemUnder; }

	void				resetItemUnder()
						{ m_lastItemUnder = 0; }

private:				// *** data members

	// pointers to the item-lists (one list for each item type)
	BList			   *m_boxes;
	BList			   *m_wires;
	BList			   *m_endPoints;
	
	// pointer to the list containing the current selection
	BList			   *m_selection;
	
	// the DiagramItem type(s) of items this group will accept
	uint32				m_types;

	// specifies the "grid"-size for the frames of items
	BPoint				m_itemAlignment;

	// can multiple items be selected at once ?
	bool				m_multiSelection;

	// cached pointer to the item that was found in itemUnder()
	DiagramItem		   *m_lastItemUnder;
};

__END_CORTEX_NAMESPACE
#endif /* __DiagramItemGroup_H__ */
