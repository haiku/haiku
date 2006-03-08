// DiagramBox.cpp

#include "DiagramBox.h"
#include "DiagramDefs.h"
#include "DiagramEndPoint.h"
#include "DiagramView.h"

#include <Message.h>
#include <Messenger.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)
#define D_MOUSE(x) //PRINT (x)
#define D_DRAW(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor (public)
// -------------------------------------------------------- //

DiagramBox::DiagramBox(
	BRect frame,
	uint32 flags)
	: DiagramItem(DiagramItem::M_BOX),
	  DiagramItemGroup(DiagramItem::M_ENDPOINT),
	  m_frame(frame),
	  m_flags(flags)
{
	D_METHOD(("DiagramBox::DiagramBox()\n"));
	makeDraggable(true);
}

DiagramBox::~DiagramBox()
{
	D_METHOD(("DiagramBox::~DiagramBox()\n"));
}

// -------------------------------------------------------- //
// *** derived from DiagramItemGroup (public)
// -------------------------------------------------------- //

bool DiagramBox::addItem(
	DiagramItem *item)
{
	D_METHOD(("DiagramBox::addItem()\n"));
	if (item)
	{
		if (DiagramItemGroup::addItem(item))
		{
			if (m_view)
			{
				item->_setOwner(m_view);
				item->attachedToDiagram();
			}
			return true;
		}
	}
	return false;
}

bool DiagramBox::removeItem(
	DiagramItem *item)
{
	D_METHOD(("DiagramBox::removeItem()\n"));
	if (item)
	{
		item->detachedFromDiagram();
		if (DiagramItemGroup::removeItem(item))
		{
			item->_setOwner(0);
			return true;
		}
	}
	return false;
}

// -------------------------------------------------------- //
// *** derived from DiagramItem (public)
// -------------------------------------------------------- //

void DiagramBox::draw(
	BRect updateRect)
{
	D_DRAW(("DiagramBox::draw()\n"));
	if (view())
	{
		view()->PushState();
		{
			if (m_flags & M_DRAW_UNDER_ENDPOINTS)
			{
				BRegion region, clipping;
				region.Include(frame());
				if (group()->getClippingAbove(this, &clipping))
					region.Exclude(&clipping);
				view()->ConstrainClippingRegion(&region);
				drawBox();
				for (int32 i = 0; i < countItems(); i++)
				{
					DiagramItem *item = itemAt(i);
					if (region.Intersects(item->frame()))
					{
						item->draw(item->frame());
					}
				}
			}
			else
			{
				BRegion region, clipping;
				region.Include(frame());
				if (view()->getClippingAbove(this, &clipping))
					region.Exclude(&clipping);
				for (int32 i = 0; i < countItems(); i++)
				{
					DiagramItem *item = itemAt(i);
					BRect r;
					if (region.Intersects(r = item->frame()))
					{
						item->draw(r);
						region.Exclude(r);
					}
				}
				view()->ConstrainClippingRegion(&region);
				drawBox();
			}
		}
		view()->PopState();
	}
}

void DiagramBox::mouseDown(
	BPoint point,
	uint32 buttons,
	uint32 clicks)
{
	D_MOUSE(("DiagramBox::mouseDown()\n"));
	DiagramItem *item = itemUnder(point);
	if (item)
	{
		item->mouseDown(point, buttons, clicks);
	}
	else if (clicks == 1)
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
			BMessage dragMsg(M_BOX_DRAGGED);
			dragMsg.AddPointer("item", static_cast<void *>(this));
			dragMsg.AddPoint("offset", point - frame().LeftTop());
			view()->DragMessage(&dragMsg, BRect(0.0, 0.0, -1.0, -1.0), view());
		}
	}
}

void DiagramBox::mouseOver(
	BPoint point,
	uint32 transit)
{
	D_MOUSE(("DiagramBox::mouseOver()\n"));
	DiagramItem *last = lastItemUnder();
	if (last && (transit == B_EXITED_VIEW))
	{
		last->mouseOver(point, B_EXITED_VIEW);
		resetItemUnder();
	}
	else
	{
		DiagramItem *item = itemUnder(point);
		if (item)
		{
			if (item != last)
			{
				if (last)
					last->mouseOver(point, B_EXITED_VIEW);
				item->mouseOver(point, B_ENTERED_VIEW);
			}
			else
			{
				item->mouseOver(point, B_INSIDE_VIEW);
			}
		}
		else if (last)
		{
			last->mouseOver(point, B_EXITED_VIEW);
		}
	}
}

void DiagramBox::messageDragged(
	BPoint point,
	uint32 transit,
	const BMessage *message)
{
	D_MOUSE(("DiagramBox::messageDragged()\n"));
	DiagramItem *last = lastItemUnder();
	if (last && (transit == B_EXITED_VIEW))
	{
		last->messageDragged(point, B_EXITED_VIEW, message);
		resetItemUnder();
	}
	else
	{
		DiagramItem *item = itemUnder(point);
		if (item)
		{
			if (item != last)
			{
				if (last)
				{
					last->messageDragged(point, B_EXITED_VIEW, message);
				}
				item->messageDragged(point, B_ENTERED_VIEW, message);
			}
			else
			{
				item->messageDragged(point, B_INSIDE_VIEW, message);
			}
			return;
		}
		else if (last)
		{
			last->messageDragged(point, B_EXITED_VIEW, message);
		}
		if (message->what == M_WIRE_DRAGGED)
		{
			view()->trackWire(point);
		}
	}
}

void DiagramBox::messageDropped(
	BPoint point,
	BMessage *message)
{
	D_METHOD(("DiagramBox::messageDropped()\n"));
	DiagramItem *item = itemUnder(point);
	if (item)
	{
		item->messageDropped(point, message);
		return;
	}
}

// -------------------------------------------------------- //
// *** operations (public)
// -------------------------------------------------------- //

void DiagramBox::moveBy(
	float x,
	float y,
	BRegion *wireRegion)
{
	D_METHOD(("DiagramBox::moveBy()\n"));
	if (view())
	{
		view()->PushState();
		{
			for (int32 i = 0; i < countItems(); i++)
			{
				DiagramEndPoint *endPoint = dynamic_cast<DiagramEndPoint *>(itemAt(i));
				if (endPoint)
				{
					endPoint->moveBy(x, y, wireRegion);
				}
			}
			if (wireRegion)
			{
				wireRegion->Include(m_frame);
				m_frame.OffsetBy(x, y);
				wireRegion->Include(m_frame);
			}
			else
			{
				m_frame.OffsetBy(x, y);
			}
		}
		view()->PopState();
	}
}

void DiagramBox::resizeBy(
	float horizontal,
	float vertical)
{
	D_METHOD(("DiagramBox::resizeBy()\n"));
	m_frame.right += horizontal;
	m_frame.bottom += vertical;
}

// -------------------------------------------------------- //
// *** internal operations (private)
// -------------------------------------------------------- //

void DiagramBox::_setOwner(
	DiagramView *owner)
{
	D_METHOD(("DiagramBox::_setOwner()\n"));
	m_view = owner;
	for (int32 i = 0; i < countItems(DiagramItem::M_ENDPOINT); i++)
	{
		DiagramItem *item = itemAt(i);
		item->_setOwner(m_view);
		if (m_view)
		{
			item->attachedToDiagram();
		}
	}
}

// END -- DiagramBox.cpp --
