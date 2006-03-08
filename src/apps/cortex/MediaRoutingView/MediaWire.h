// MediaWire.h
// c.lenz 10oct99
//
// HISTORY
//

#ifndef __MediaWire_H__
#define __MediaWire_H__

// DiagramView
#include "DiagramWire.h"
// NodeManager
#include "Connection.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class MediaJack;

class MediaWire :
	public		DiagramWire {
	typedef		DiagramWire _inherited;

public:					// *** constans

	// [e.moon 26oct99] moved definition to MediaWire.cpp
	static const float		M_WIRE_OFFSET;

public:					// *** ctor/dtor

	// input and output jack are set connected by this constructor
	// so be careful to only pass in valid pointers
						MediaWire(
							Connection connection,
							MediaJack *outputJack,
							MediaJack *inputJack);

	// special constructor used only in drag&drop sessions for
	// temporary visual indication of the connection process
	// the isStartPoint specifies if the given EndPoint is 
	// supposed to be treated as start or end point
						MediaWire(
							MediaJack *jack,
							bool isStartPoint);

	virtual				~MediaWire();
	
public:					// *** accessors

	Connection			connection;

public:					// *** derived from DiagramWire/Item

	// init the cached points and the frame rect
	virtual void		attachedToDiagram();

	// make sure no tooltip is being displayed for this wire
	// +++ DOES NOT WORK (yet)
	virtual void		detachedFromDiagram();

	// calculates and returns the frame rectangle of the wire
	virtual BRect		frame() const;

	// returns a value > 0.5 for points pretty much close to the
	// wire
	virtual float		howCloseTo(
							BPoint point) const;

	// does the actual drawing
	virtual void		drawWire();

	// displays the context-menu for right-clicks
	virtual void		mouseDown(
							BPoint point,
							uint32 buttons,
							uint32 clicks);	

	// changes the mouse cursor and starts a tooltip
	virtual void		mouseOver(
							BPoint point,
							uint32 transit);

	// also selects and invalidates the jacks connected by 
	// this wire
	virtual void		selected();
	
	// also deselectes and invalidates the jacks connected
	// by this wire
	virtual void		deselected();

	// updates the cached start & end points and the frame rect
	virtual void		endPointMoved(
							DiagramEndPoint *which = 0);

protected:					// *** operations

	// display a popup-menu at given point
	void				showContextMenu(
							BPoint point);

private:					// *** data members

	BPoint				m_startPoint;
	
	BPoint				m_endPoint;
	
	BPoint				m_startOffset;
	
	BPoint				m_endOffset;
	
	BRect				m_frame;
};

__END_CORTEX_NAMESPACE
#endif /* __MediaWire_H__ */
