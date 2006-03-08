// DiagramWire.cpp

#include "DiagramWire.h"
#include "DiagramDefs.h"
#include "DiagramEndPoint.h"
#include "DiagramView.h"

#include <Message.h>
#include <Messenger.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

DiagramWire::DiagramWire(
	DiagramEndPoint *fromWhich,
	DiagramEndPoint *toWhich)
	: DiagramItem(DiagramItem::M_WIRE),
	  m_fromEndPoint(fromWhich),
	  m_toEndPoint(toWhich),
	  m_temporary(false)
{
	D_METHOD(("DiagramWire::DiagramWire()\n"));
	makeDraggable(false);
	ASSERT(m_fromEndPoint);
	m_fromEndPoint->connect(this);
	ASSERT(m_toEndPoint);
	m_toEndPoint->connect(this);
}

DiagramWire::DiagramWire(
	DiagramEndPoint *fromWhich,
	bool isStartPoint)
	: DiagramItem(DiagramItem::M_WIRE),
	  m_fromEndPoint(isStartPoint ? fromWhich : 0),
	  m_toEndPoint(isStartPoint ? 0 : fromWhich),
	  m_temporary(true),
	  m_dragEndPoint(fromWhich->connectionPoint())
{
	D_METHOD(("DiagramWire::DiagramWire(temp)\n"));
	makeDraggable(false);
	ASSERT(fromWhich);
	if (m_fromEndPoint)
	{
		m_fromEndPoint->connect(this);
	}
	else if (m_toEndPoint)
	{
		m_toEndPoint->connect(this);
	}
}

DiagramWire::~DiagramWire()
{
	D_METHOD(("DiagramWire::~DiagramWire()\n"));
	if (m_fromEndPoint)
	{
		m_fromEndPoint->disconnect();
	}
	if (m_toEndPoint)
	{
		m_toEndPoint->disconnect();
	}
}

// -------------------------------------------------------- //
// *** accessors
// -------------------------------------------------------- //

BPoint DiagramWire::startConnectionPoint() const
{
	D_METHOD(("DiagramWire::startConnectionPoint()\n"));
	if (m_fromEndPoint)
	{
		return m_fromEndPoint->connectionPoint();
	}
	else if (m_temporary)
	{
		return m_dragEndPoint;
	}
	return BPoint(-1.0, -1.0);
}

BPoint DiagramWire::endConnectionPoint() const
{
	D_METHOD(("DiagramWire::endConnectionPoint()\n"));
	if (m_toEndPoint)
	{
		return m_toEndPoint->connectionPoint();
	}
	else if (m_temporary)
	{
		return m_dragEndPoint;
	}
	return BPoint(-1.0, -1.0);
}

// -------------------------------------------------------- //
// *** derived from DiagramItem
// -------------------------------------------------------- //

float DiagramWire::howCloseTo(
	BPoint point) const
{
	D_METHOD(("DiagramWire::howCloseTo()\n"));
	BPoint a = startConnectionPoint();
	BPoint b = endConnectionPoint();
	float length, result;
	length = sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2));
	result = ((a.y - point.y) * (b.x - a.x)) - ((a.x - point.x) * (b.y - a.y));
	result = 3.0 - fabs(result / length);
	return result;
}

void DiagramWire::draw(
	BRect updateRect)
{
	D_METHOD(("DiagramWire::draw()\n"));
	if (view())
	{
		view()->PushState();
		{
			BRegion region, clipping;
			region.Include(frame() & updateRect);
			if (view()->getClippingAbove(this, &clipping))
				region.Exclude(&clipping);
			view()->ConstrainClippingRegion(&region);
			drawWire();
		}
		view()->PopState();
	}
}

void DiagramWire::mouseDown(
	BPoint point,
	uint32 buttons,
	uint32 clicks)
{
	D_METHOD(("DiagramWire::mouseDown()\n"));
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
	}
}

void DiagramWire::messageDragged(
	BPoint point,
	uint32 transit,
	const BMessage *message)
{
	D_METHOD(("DiagramWire::messageDragged()\n"));
	switch (message->what)
	{
		case M_WIRE_DRAGGED:
		{
			view()->trackWire(point);
			break;
		}
		default:
		{
			DiagramItem::messageDragged(point, transit, message);
		}
	}
}

// END -- DiagramWire.cpp --
