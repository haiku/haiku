// DiagramItem.h (Cortex/DiagramView)
//
// * PURPOSE
//   DiagramItem subclass providing a basic implementation
//   of 'connectable' points inside a DiagramBox
//
// * HISTORY
//   c.lenz		25sep99		Begun
//

#ifndef __DiagramEndPoint_H__
#define __DiagramEndPoint_H__

#include <Looper.h>
#include <Region.h>

#include "DiagramItem.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class DiagramWire;

class DiagramEndPoint : public DiagramItem
{

public:					// *** ctor/dtor

						DiagramEndPoint(
							BRect frame);

	virtual				~DiagramEndPoint();

public:					// *** accessors

	DiagramWire		   *wire() const
						{ return m_wire; }

public:					// *** hook functions

	// is called from draw() to do the actual drawing
	virtual void		drawEndPoint() = 0;

	// should return the BPoint that a connected wire uses as
	// a drawing offset; this implementation returns the center 
	// point of the EndPoint
	virtual BPoint		connectionPoint() const;

	// is called from messageDragged() and messageDropped() to 
	// let you verify that a connection is okay
	virtual bool		connectionRequested(
							DiagramEndPoint *fromWhich);
	
	// is called whenever a connection has been made or a
	// temporary wire drag has been accepted
	virtual void		connected()
						{ /* does nothing */ }

	// is called whenever a connection has been broken or a
	// temporary wire drag has finished
	virtual void		disconnected()
						{ /* does nothing */ }

public:					// *** derived from DiagramItem

	// returns the EndPoints frame rectangle
	BRect				frame() const
						{ return m_frame; }

	// prepares the drawing stack and clipping region, then
	// calls drawEndPoint
	void				draw(
							BRect updateRect);

	// is called from the parent DiagramBox's mouseDown() 
	// implementation if the EndPoint was hit; this version
	// initiates selection and dragging (i.e. connecting)
	virtual void		mouseDown(
							BPoint point,
							uint32 buttons,
							uint32 clicks);

	// is called from the parent DiagramBox's mouseOver()
	// implementation if the mouse is over the EndPoint
	virtual void		mouseOver(
							BPoint point,
							uint32 transit)
						{ /* does nothing */ }

	// is called from the parent DiagramBox's messageDragged()
	// implementation of a message is being dragged of the
	// EndPoint; this implementation handles only wire drags
	// (i.e. M_WIRE_DRAGGED messages)
	virtual void		messageDragged(
							BPoint point,
							uint32 transit,
							const BMessage *message);
							
	// is called from the parent DiagramBox's messageDropped()
	// implementation if the message was dropped on the EndPoint;
	// this version handles wire dropping (connecting)
	virtual void		messageDropped(
							BPoint point,
							BMessage *message);
								
	// moves the EndPoint by the specified amount and returns in
	// updateRegion the frames of connected wires if there are any
	// NOTE: no drawing/refreshing is done in this method, that's
	// up to the parent
	void				moveBy(
							float x,
							float y,
							BRegion *updateRegion);
		
	// resize the EndPoints frame rectangle without redraw
	virtual void		resizeBy(
							float horizontal,
							float vertical);

public:					// *** connection methods

	// connect/disconnect and set the wire pointer accordingly
	void				connect(
							DiagramWire *wire);
	void				disconnect();

	// returns true if the EndPoint is currently connected
	bool				isConnected() const
						{ return m_connected; }

	// returns true if the EndPoint is currently in a connection
	// process (i.e. it is either being dragged somewhere, or a
	// M_WIRE_DRAGGED message is being dragged over it
	bool				isConnecting() const
						{ return m_connecting; }

private:				// *** data

	// the endpoints' frame rectangle
	BRect				m_frame;

	// pointer to the wire object this endpoint might be
	// connected to
	DiagramWire		   *m_wire;

	// indicates if the endpoint is currently connected
	// to another endpoint
	bool				m_connected;
	
	// indicates that a connection is currently being made
	// but has not been confirmed yet
	bool				m_connecting;
};

__END_CORTEX_NAMESPACE
#endif // __DiagramEndPoint_H__
