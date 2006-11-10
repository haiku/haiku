// DiagramView.h (Cortex/DiagramView)
//
// * PURPOSE
//   BView and DiagramItemGroup derived class providing the
//   one and only drawing context all child DiagramItems will
//   use.
//
// * HISTORY
//   c.lenz		25sep99		Begun
//

#ifndef __DiagramView_H__
#define __DiagramView_H__

#include "DiagramItemGroup.h"

#include <Region.h>
#include <View.h>

//class BBitmap;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

//class DiagramBox;
class DiagramWire;
class DiagramEndPoint;

class DiagramView : public BView,
					public DiagramItemGroup
{

public:				// *** ctor/dtor

						DiagramView(
							BRect frame,
							const char *name,
							bool multiSelection,
							uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW);

	virtual				~DiagramView();

public:					// *** hook functions

	// is called from MessageReceived() if a wire has been dropped
	// on the background or on an incompatible endpoint
	virtual void		connectionAborted(
							DiagramEndPoint *fromWhich)
						{ /* does nothing */ }

	// is called from MessageReceived() if an endpoint has accepted
	// a wire-drop (i.e. connection)
	virtual void		connectionEstablished(
							DiagramEndPoint *fromWhich,
							DiagramEndPoint *toWhich)
						{ /* does nothing */ }

	// must be implemented to return an instance of the DiagramWire
	// derived class
	virtual DiagramWire *createWire(
							DiagramEndPoint *fromWhich,
							DiagramEndPoint *woWhich) = 0;

	// must be implemented to return an instance of the DiagramWire
	// derived class; this version is for temporary wires used in
	// drag & drop connecting
	virtual DiagramWire *createWire(
							DiagramEndPoint *fromWhich) = 0;
					
	// hook called from MouseDown() if the background was hit
	virtual void		mouseDown(
							BPoint point,
							uint32 buttons,
							uint32 clicks)
						{ /* does nothing */ }

	// hook called from MouseMoved() if the mouse is floating over
	// the background (i.e. with no message attached)
	virtual void		mouseOver(
							BPoint point,
							uint32 transit)
						{ /* does nothing */ }

	// hook called from MouseMoved() if a message is being dragged
	// over the background
	virtual void		messageDragged(
							BPoint point,
							uint32 transit,
							const BMessage *message);

	// hook called from MessageReceived() if a message has been
	// dropped over the background
	virtual void		messageDropped(
							BPoint point,
							BMessage *message);

public:					// derived from BView

	// initial scrollbar update [e.moon 16nov99]
	virtual void		AttachedToWindow();

	// draw the background and all items
	virtual void		Draw(
							BRect updateRect);

	// updates the scrollbars
	virtual void		FrameResized(
							float width,
							float height);

	// return data rect [c.lenz 1mar2000]
	virtual void		GetPreferredSize(
							float *width,
							float *height);

	// handles the messages M_SELECTION_CHANGED and M_WIRE_DROPPED
	// and passes a dropped message on to the item it was dropped on
	virtual void		MessageReceived(
							BMessage *message);

	// handles the arrow keys for moving DiagramBoxes
	virtual void		KeyDown(
							const char *bytes,
							int32 numBytes);

	// if an item is located at the click point, this function calls
	// that items mouseDown() method; else a deselectAll() command is
	// made and rect-tracking is initiated
	virtual void		MouseDown(
							BPoint point);

	// if an item is located under the given point, this function
	// calls that items messageDragged() hook if a message is being
	// dragged, and mouseOver() if not
	virtual void		MouseMoved(
							BPoint point,
							uint32 transit,
							const BMessage *message);

	// ends rect-tracking and wire-tracking
	virtual void		MouseUp(
							BPoint point);

public:					// *** derived from DiagramItemGroup

	// extends the DiagramItemGroup implementation by setting
	// the items owner and calling the attachedToDiagram() hook
	// on it
	virtual bool		addItem(
							DiagramItem *item);

	// extends the DiagramItemGroup implementation by calling 
	// the detachedToDiagram() hook on the item
	virtual bool		removeItem(
							DiagramItem *item);

public:					// *** operations

	// update the temporary wire to follow the mouse cursor
	void				trackWire(
							BPoint point);

	bool				isWireTracking() const
						{ return m_draggedWire; }

protected:				// *** internal operations

	// do the actual background drawing
	void				drawBackground(
							BRect updateRect);

	// returns the current background color
	rgb_color			backgroundColor() const
						{ return m_backgroundColor; }

	// set the background color; does not refresh the display
	void				setBackgroundColor(
							rgb_color color);
	
	// set the background bitmap; does not refresh the display
	void				setBackgroundBitmap(
							BBitmap *bitmap);

	// updates the region containing the rects of all boxes in
	// the view (and thereby the "data-rect") and then adapts
	// the scrollbars if necessary
	void				updateDataRect();

private:				// *** internal operations

	// setup a temporary wire for "live" dragging and attaches
	// a message to the mouse
	void				_beginWireTracking(
							DiagramEndPoint *fromEndPoint);

	// delete the temporary dragged wire and invalidate display
	void				_endWireTracking();
	
	// setups rect-tracking to additionally drag a message for
	// easier identification in MouseMoved()
	void				_beginRectTracking(
								BPoint origin);

	// takes care of actually selecting/deselecting boxes when
	// they intersect with the tracked rect
	void				_trackRect(
							BPoint origin,
							BPoint current);

	// updates the scrollbars (if there are any) to represent
	// the current data-rect
	void				_updateScrollBars();

private:				// *** data

	// the button pressed at the last mouse event
	int32				m_lastButton;

	// the number of clicks with the last button
	int32				m_clickCount;

	// the point last clicked in this view
	BPoint				m_lastClickPoint;

	// the button currently pressed (reset to 0 on mouse-up)
	// [e.moon 16nov99]
	int32					m_pressedButton;

	// last mouse position in screen coordinates [e.moon 16nov99]
	// only valid while m_pressedButton != 0
	BPoint				m_lastDragPoint;

	// a pointer to the temporary wire used for wire
	// tracking
	DiagramWire		   *m_draggedWire;
	
	// contains the rects of all DiagramBoxes in this view
	BRegion				m_boxRegion;

	// contains the rect of the view actually containing something
	// (i.e. DiagramBoxes) and all free space left/above of that
	BRect				m_dataRect;

	// true if a bitmap is used for the background; false
	// if a color is used
	bool				m_useBackgroundBitmap;

	// the background color of the view
	rgb_color			m_backgroundColor;
	
	// the background bitmap of the view
	BBitmap			   *m_backgroundBitmap;
};

__END_CORTEX_NAMESPACE
#endif // __DiagramView_H__

