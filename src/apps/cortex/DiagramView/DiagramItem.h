// DiagramItem.h (Cortex/DiagramView)
//
// * PURPOSE
//   Provides a base class for all items that can be handled 
//   by the DiagramView implementation. A basic interface with
//	 some common implementation is defined, with methods related
//   to drawing, mouse handling, selecting, dragging, comparison 
//   for sorting, and access to the drawing context which is the 
//   DiagramView instance.
//
// * HISTORY
//   c.lenz		25sep99		Begun
//

#ifndef __DiagramItem_H__
#define __DiagramItem_H__

#include <OS.h>
#include <InterfaceDefs.h>
#include <Region.h>

class BView;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class DiagramItemGroup;
class DiagramView;
class DiagramBox;

int compareSelectionTime(const void *lValue, const void *rValue);

class DiagramItem
{
	friend class DiagramItemGroup;
	friend class DiagramView;
	friend class DiagramBox;

public:					// *** types

	enum diagram_item_t {
		M_BOX		= 0x1,
		M_WIRE		= 0x2,
		M_ENDPOINT	= 0x4,
		M_ANY		= 0x7
	};

public:					// *** ctor/dtor

						DiagramItem(
							uint32 itemType);

	virtual				~DiagramItem();

public:					// *** accessors
	
	// returns the item type assigned in the ctor
	uint32				type() const 
						{ return m_type; }

	// returns pointer to the drawing context of the DiagramView 
	// object
	DiagramView		   *view() const
						{ return m_view; }

	// returns pointer to the DiagramItemGroup the item belongs to
	DiagramItemGroup   *group() const
						{ return m_group; }

	// returns true if the item is currently selected
	bool				isSelected() const
						{ return m_selected; }

public:					// *** operations

	// changes the selection state of the item, and updates the
	// m_selectionTime member in the process to ensure proper
	// sorting; calls the selected() hook if the state has
	// actually changed
	void				select();

	// sets the item to selected without changing m_selectionTime
	// to the time of selection but prior to the last "replacing"
	// selection (i.e. thru select()) use this method for additive 
	// selecting; still calls the selected() hook
	void				selectAdding();
	
	// deselects the item; calls the deselected() hook if the
	// state has actually changed
	void				deselect();

	// moves the items frame to a given point by calling moveBy with the 
	// absolute coords translated into relative shift amount
	void				moveTo(
							BPoint point,
							BRegion *updateRegion = 0)
						{ moveBy(point.x - frame().left, point.y - frame().top, updateRegion); }

	// resizes the items frame to given dimensions; simply calls the resizeBy
	// implementation
	void				resizeTo(
							float width,
							float height)
						{ resizeBy(width - frame().Width(), height - frame().Height()); }

public:					// *** hook functions

	// is called when the item has been attached to the DiagramView
	// and the view() pointer is valid
	virtual void		attachedToDiagram()
						{ /* does nothing */ }
	
	// is called just before the item is being detached from the 
	// the DiagramView
	virtual void		detachedFromDiagram()
						{ /* does nothing */ }
	
	// is called from the DiagramViews MouseDown() function after
	// finding out the mouse buttons and clicks quantity.
	virtual void		mouseDown(
							BPoint point,
							uint32 buttons,
							uint32 clicks)
						{/* does nothing */}

	// is called from the DiagramViews MouseMoved() when *no* message is being 
	// dragged, i.e. the mouse is simply floating above the item
	virtual void		mouseOver(
							BPoint point,
							uint32 transit)
						{/* does nothing */}

	// is called from the DiagramViews MouseMoved() when a message is being
	// dragged; always call the base class version when overriding!
	virtual void		messageDragged(
							BPoint point,
							uint32 transit,
							const BMessage *message)
						{/* does nothing */}

	// is called from the DiagramViews MessageReceived() function when an 
	// message has been received through Drag&Drop; always call the base
	// class version when overriding!
	virtual void		messageDropped(
							BPoint point,
							BMessage *message)
						{/* does nothing */}

	// is called when the item has been selected or deselected in some way
	virtual void		selected()
						{ /* does nothing */ }
	virtual void		deselected()
						{ /* does nothing */ }

public:					// *** interface definition

	// this function must be implemented by derived classes to return the
	// items frame rectangle in the DiagramViews coordinates
	virtual BRect		frame() const = 0;
	
	// this function should be implemented for non-rectangular subclasses
	// (like wires) to estimate how close a given point is to the object;
	// the default implementation returns 1.0 when the point lies within
	// the Frame() rect and 0.0 if not
	virtual float		howCloseTo(
							BPoint point) const;

	// this is the hook function called by DiagramView when it's time to
	// draw the object
	virtual void		draw(
							BRect updateRect) = 0;

	// should move the items frame by the specified amount and do the
	// necessary drawing instructions to update the display; if the
	// caller supplied a BRegion pointer in updateRegion, this method
	// should add other areas affected by the move to it (e.g. wire
	// frames)
	virtual void		moveBy(
							float x,
							float y,
							BRegion *updateRegion = 0)
						{ /* does nothing */ }

	// should resize the items frame by the specified amount
	virtual void		resizeBy(
							float horizontal,
							float vertical)
						{ /* does nothing */ }

protected:				// *** selecting/dragging

	// turn on/off the built-in selection handling
	void				makeSelectable(
							bool selectable)
							{ m_selectable = selectable; }
	bool				isSelectable() const
							{ return m_selectable; }

	// turn on/off the built-in drag & drop handling
	void				makeDraggable(
							bool draggable)
							{ m_draggable = draggable; }
	bool				isDraggable() const
							{ return m_draggable; }

protected:				// *** compare functions

	// compares the time when each item was last selected and
	// returns -1 for the most recent.
	friend int			compareSelectionTime(
							const void *lValue,
							const void *rValue);

protected:				// *** internal methods	

	// called only by DiagramItemGroup objects in the method
	// addItem()
	virtual void		_setOwner(
							DiagramView *owner) 
						{ m_view = owner; }

private:				// *** data members

	// the items type (M_BOX, M_WIRE or M_ENDPOINT)
	uint32				m_type;

	// a pointer to the drawing context (the DiagramView instance)
	DiagramView		   *m_view;

	// a pointer to the DiagramItemGroup the item belongs to
	DiagramItemGroup   *m_group;

	// can the object be dragged
	bool				m_draggable;

	// can the object be selected
	bool				m_selectable;

	// is the object currently selected
	bool				m_selected;

	// when was the object selected the last time or added (used 
	// for drawing order)
	bigtime_t			m_selectionTime;

	// stores the most recent time a item was selected thru 
	// the select() method
	static bigtime_t	m_lastSelectionTime;
	
	// counts the number of selections thru selectAdding()
	// since the last call to select()
	static int32		m_countSelected;
};

__END_CORTEX_NAMESPACE
#endif /* __DiagramItem_H__ */
