// DiagramBox.h (Cortex/DiagramView.h)
//
// * PURPOSE
//   DiagramItem subclass providing a basic framework for
//   boxes in diagrams, i.e. objects that can contain various
//   endpoints
//
// * HISTORY
//   c.lenz		25sep99		Begun
//

#ifndef __DiagramBox_H__
#define __DiagramBox_H__

#include <Region.h>
#include <Window.h>

#include "DiagramItem.h"
#include "DiagramItemGroup.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class DiagramBox : public DiagramItem,
				   public DiagramItemGroup
{

public:					// *** flags

	enum flag_t
	{
		M_DRAW_UNDER_ENDPOINTS = 0x1
	};

public:					// *** ctor/dtor

						DiagramBox(
							BRect frame,
							uint32 flags = 0);

	virtual				~DiagramBox();

public:					// *** hook functions

	// is called from draw() to do the actual drawing
	virtual void		drawBox() = 0;

public:					// ** derived from DiagramItemGroup

	// extends the DiagramItemGroup implementation by setting
	// the items owner and calling the attachedToDiagram() hook
	// on it
	virtual bool		addItem(
							DiagramItem *item);

	// extends the DiagramItemGroup implementation by calling 
	// the detachedToDiagram() hook on the item
	virtual bool		removeItem(
							DiagramItem *item);

public:					// *** derived from DiagramItem

	// returns the Boxes frame rectangle
	BRect				frame() const
						{ return m_frame; }

	// prepares the drawing stack and clipping region, then
	// calls drawBox
	void				draw(
							BRect updateRect);

	// is called from the parent DiagramViews MouseDown() implementation 
	// if the Box was hit; this version initiates selection and dragging
	virtual void		mouseDown(
							BPoint point,
							uint32 buttons,
							uint32 clicks);

	// is called from the DiagramViews MouseMoved() when no message is being 
	// dragged, but the mouse has moved above the box
	virtual void		mouseOver(
							BPoint point,
							uint32 transit);
		
	// is called from the DiagramViews MouseMoved() when a message is being 
	// dragged over the DiagramBox
	virtual void		messageDragged(
							BPoint point,
							uint32 transit,
							const BMessage *message);

	// is called from the DiagramViews MessageReceived() function when a 
	// message has been dropped on the DiagramBox
	virtual void		messageDropped(
							BPoint point,
							BMessage *message);

public:					// *** operations

	// moves the box by a given amount, and returns in updateRegion the
	// frames of wires impacted by the move
	void				moveBy(
							float x,
							float y,
							BRegion *updateRegion);

	// resizes the boxes frame without doing any updating		
	virtual void		resizeBy(
							float horizontal,
							float vertical);

private:				// *** internal methods

	// is called by the DiagramView when added
	void				_setOwner(
							DiagramView *owner);

private:				// *** data

		// the boxes' frame rectangle
		BRect				m_frame;

		// flags:
		// 	M_DRAW_UNDER_ENDPOINTS -  don't remove EndPoint frames from
		//							the clipping region
		uint32				m_flags;
};

__END_CORTEX_NAMESPACE
#endif /* __DiagramBox_H__ */
