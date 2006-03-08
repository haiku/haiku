// DiagramEndPoint.cpp

#include "DiagramEndPoint.h"
#include "DiagramDefs.h"
#include "DiagramWire.h"
#include "DiagramView.h"

#include <Message.h>
#include <Messenger.h>
__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)
#define D_MOUSE(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

DiagramEndPoint::DiagramEndPoint(
	BRect frame)
	: DiagramItem(DiagramItem::M_ENDPOINT),
	  m_frame(frame),
	  m_wire(0),
	  m_connected(false),
	  m_connecting(false)
{
	D_METHOD(("DiagramEndPoint::DiagramEndPoint()\n"));
	makeDraggable(true);
}

DiagramEndPoint::~DiagramEndPoint()
{
	D_METHOD(("DiagramEndPoint::~DiagramEndPoint()\n"));
}

// -------------------------------------------------------- //
// *** hook functions
// -------------------------------------------------------- //

BPoint DiagramEndPoint::connectionPoint() const
{
	D_METHOD(("DiagramEndPoint::connectionPoint()\n"));
	return BPoint(frame().left + frame().Height() / 2.0, frame().top + frame().Width() / 2.0);
}

bool DiagramEndPoint::connectionRequested(
	DiagramEndPoint *fromWhich)
{
	D_METHOD(("DiagramEndPoint::connectionRequested()\n"));
	if (!isConnected())
	{
		return true;
	}
	return false;
}

// -------------------------------------------------------- //
// *** derived from DiagramItem
// -------------------------------------------------------- //

void DiagramEndPoint::draw(
	BRect updateRect)
{
	D_METHOD(("DiagramEndPoint::draw()\n"));
	if (view())
	{
		view()->PushState();
		{
			BRegion region;
			region.Include(frame() & updateRect);
			view()->ConstrainClippingRegion(&region);
			drawEndPoint();
		}
		view()->PopState();
	}
}

void DiagramEndPoint::mouseDown(
	BPoint point,
	uint32 buttons,
	uint32 clicks)
{
	D_MOUSE(("DiagramEndPoint::mouseDown()\n"));
	if (clicks == 1)
	{
		if (isSelectable())
		{
			BMessage selectMsg(M_SELECTION_CHANGED);
			if (modifiers() & B_SHIFT_KEY)
			{
				selectMsg.AddBool("replace", false);
			}
			else
			{
				selectMsg.AddBool("replace", true);
			}
			selectMsg.AddPointer("item", reinterpret_cast<void *>(this));
			DiagramView* v = view();
			BMessenger(v).SendMessage(&selectMsg);
		}
		if (isDraggable() && (buttons == B_PRIMARY_MOUSE_BUTTON))
		{
			BMessage dragMsg(M_WIRE_DRAGGED);
			dragMsg.AddPointer("from", static_cast<void *>(this));
			view()->DragMessage(&dragMsg, BRect(0.0, 0.0, -1.0, -1.0), view());
			view()->messageDragged(point, B_INSIDE_VIEW, &dragMsg);
		}
	}
}

void DiagramEndPoint::messageDragged(
	BPoint point,
	uint32 transit,
	const BMessage *message)
{
	D_MOUSE(("DiagramEndPoint::messageDragged()\n"));
	switch (message->what)
	{
		case M_WIRE_DRAGGED:
		{
			D_MESSAGE(("DiagramEndPoint::messageDragged(M_WIRE_DRAGGED)\n"));
			switch (transit)
			{
				case B_INSIDE_VIEW:
				{
					//PRINT((" -> transit: B_INSIDE_VIEW\n"));
					// this is a WORK-AROUND caused by the unreliability
					// of BViews DragMessage() routine !!
					if (isConnecting())
					{
						break;
					}
					else if (isConnected())
					{
						view()->trackWire(point);
					}
					/* this should be enough in theory:
					if (!isConnecting())
					{
						view()->trackWire(point);
					}
					//break;*/
				}
				case B_ENTERED_VIEW:
				{
					//PRINT((" -> transit: B_ENTERED_VIEW\n"));
					DiagramEndPoint *endPoint;
					if ((message->FindPointer("from", reinterpret_cast<void **>(&endPoint)) == B_OK)
					 && (endPoint != this))
					{
						if (connectionRequested(endPoint))
						{
							view()->trackWire(connectionPoint());
							if (!isConnecting())
							{
								m_connecting = true;
								connected();
							}
						}
						else
						{
							view()->trackWire(point);
							if (isConnecting())
							{
								m_connecting = false;
								disconnected();
							}
						}
					}
					break;
				}
				case B_EXITED_VIEW:
				{
					//PRINT((" -> transit: B_EXITED_VIEW\n"));
					if (isConnecting())
					{
						m_connecting = false;
						disconnected();
					}
					break;
				}
			}
			break;
		}
		default:
		{
			DiagramItem::messageDragged(point, transit, message);
		}
	}
}

void DiagramEndPoint::messageDropped(
	BPoint point,
	BMessage *message)
{
	D_MESSAGE(("DiagramEndPoint::messageDropped()\n"));
	switch (message->what)
	{
		case M_WIRE_DRAGGED:
		{
			D_MESSAGE(("DiagramEndPoint::messageDropped(M_WIRE_DRAGGED)\n"));
			DiagramEndPoint *endPoint;
			if ((message->FindPointer("from", reinterpret_cast<void **>(&endPoint)) == B_OK)
			 && (endPoint != this))
			{
				bool success = connectionRequested(endPoint);
				BMessage dropMsg(M_WIRE_DROPPED);
				dropMsg.AddPointer("from", reinterpret_cast<void *>(endPoint));
				dropMsg.AddPointer("to", reinterpret_cast<void *>(this));
				dropMsg.AddBool("success", success);
				DiagramView* v = view();
				BMessenger(v).SendMessage(&dropMsg);
				if (isConnecting())
				{
					m_connecting = false;
					disconnected();
				}
			}
			return;
		}
		default:
		{
			DiagramItem::messageDropped(point, message);
		}
	}
}

// -------------------------------------------------------- //
// *** frame related operations
// -------------------------------------------------------- //

void DiagramEndPoint::moveBy(
	float x,
	float y,
	BRegion *updateRegion)
{
	D_METHOD(("DiagramEndPoint::moveBy()\n"));
	if (isConnected() && m_wire && updateRegion)
	{	
		updateRegion->Include(m_wire->frame());
		m_frame.OffsetBy(x, y);
		m_wire->endPointMoved(this);
		updateRegion->Include(m_wire->frame());
	}
	else
	{
		m_frame.OffsetBy(x, y);
		if (m_wire)
		{
			m_wire->endPointMoved(this);	
		}
	}
}

void DiagramEndPoint::resizeBy(
	float horizontal,
	float vertical)
{
	D_METHOD(("DiagramEndPoint::resizeBy()\n"));
	m_frame.right += horizontal;
	m_frame.bottom += vertical;
}

// -------------------------------------------------------- //
// *** connection methods
// -------------------------------------------------------- //

void DiagramEndPoint::connect(
	DiagramWire *wire)
{
	D_METHOD(("DiagramEndPoint::connect()\n"));
	if (!m_connected && wire)
	{
		m_connected = true;
		m_wire = wire;
		makeDraggable(false);
		connected();
	}
}

void DiagramEndPoint::disconnect()
{
	D_METHOD(("DiagramEndPoint::disconnect()\n"));
	if (m_connected)
	{
		m_connected = false;
		m_wire = 0;
		makeDraggable(true);
		disconnected();
	}
}

// END -- DiagramEndPoint.cpp --
