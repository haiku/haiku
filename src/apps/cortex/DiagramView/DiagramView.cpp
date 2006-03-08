// DiagramView.cpp

#include "DiagramView.h"
#include "DiagramDefs.h"
#include "DiagramBox.h"
#include "DiagramEndPoint.h"
#include "DiagramWire.h"

#include <Bitmap.h>
#include <Message.h>
#include <ScrollBar.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT (x)
#define D_HOOK(x) //PRINT (x)
#define D_MESSAGE(x) //PRINT (x)
#define D_MOUSE(x) //PRINT (x)
#define D_DRAW(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

DiagramView::DiagramView(
	BRect frame,
	const char *name,
	bool multiSelection,
	uint32 resizingMode,
	uint32 flags)
	: BView(frame, name, resizingMode, B_WILL_DRAW | B_FRAME_EVENTS | flags),
	  DiagramItemGroup(DiagramItem::M_BOX | DiagramItem::M_WIRE),
	  m_lastButton(0),
	  m_clickCount(0),
	  m_lastClickPoint(-1.0, -1.0),
	  m_pressedButton(0),
	  m_draggedWire(0),
	  m_useBackgroundBitmap(false),
	  m_backgroundBitmap(0)
{
	D_METHOD(("DiagramView::DiagramView()\n"));
	SetViewColor(B_TRANSPARENT_COLOR);
	m_dataRect = Bounds();
}

DiagramView::~DiagramView()
{
	D_METHOD(("DiagramView::~DiagramView()\n"));
}

// -------------------------------------------------------- //
// *** hook functions
// -------------------------------------------------------- //

void DiagramView::messageDragged(
	BPoint point,
	uint32 transit,
	const BMessage *message)
{
	D_METHOD(("DiagramView::messageDragged()\n"));
	switch (message->what)
	{
		case M_WIRE_DRAGGED:
		{
			D_MESSAGE(("DiagramView::messageDragged(M_WIRE_DROPPED)\n"));
			if (!m_draggedWire)
			{
				DiagramEndPoint *fromEndPoint;
				if (message->FindPointer("from", reinterpret_cast<void **>(&fromEndPoint)) == B_OK)
				{
					_beginWireTracking(fromEndPoint);
				}
			}
			trackWire(point);
			break;
		}
	}
}

void DiagramView::messageDropped(
	BPoint point,
	BMessage *message)
{
	D_METHOD(("DiagramView::messageDropped()\n"));
	switch (message->what)
	{
		case M_WIRE_DRAGGED:
		{
			D_MESSAGE(("DiagramView::messageDropped(M_WIRE_DROPPED)\n"));
			DiagramEndPoint *fromWhich = 0;
			if (message->FindPointer("from", reinterpret_cast<void **>(&fromWhich)) == B_OK)
			{
				connectionAborted(fromWhich);
			}
			break;
		}
	}
}

// -------------------------------------------------------- //
// *** derived from BView
// -------------------------------------------------------- //

// initial scrollbar update [e.moon 16nov99]
void DiagramView::AttachedToWindow()
{
	D_METHOD(("DiagramView::AttachedToWindow()\n"));
	_updateScrollBars();
}

void DiagramView::Draw(
	BRect updateRect)
{
	D_METHOD(("DiagramView::Draw()\n"));
	drawBackground(updateRect);
	drawItems(updateRect, DiagramItem::M_WIRE);
	drawItems(updateRect, DiagramItem::M_BOX);
}

void DiagramView::FrameResized(
	float width,
	float height)
{
	D_METHOD(("DiagramView::FrameResized()\n"));
	_updateScrollBars();
}

void DiagramView::GetPreferredSize(
	float *width,
	float *height) {
	D_HOOK(("DiagramView::GetPreferredSize()\n"));

	*width = m_dataRect.Width() + 10.0;
	*height = m_dataRect.Height() + 10.0;	
}

void DiagramView::MessageReceived(
	BMessage *message)
{
	D_METHOD(("DiagramView::MessageReceived()\n"));
	switch (message->what)
	{
		case M_SELECTION_CHANGED:
		{
			D_MESSAGE(("DiagramView::MessageReceived(M_SELECTION_CHANGED)\n"));
			DiagramItem *item;
			if (message->FindPointer("item", reinterpret_cast<void **>(&item)) == B_OK)
			{
				bool deselectOthers = true;
				message->FindBool("replace", &deselectOthers);
				if (item->isSelected() && !deselectOthers && multipleSelection())
				{
					if (deselectItem(item))
					{
						selectionChanged();
					}
				}
				else if (selectItem(item, deselectOthers))
				{
					sortItems(item->type(), &compareSelectionTime);
					selectionChanged();
				}
			}
			break;
		}
		case M_WIRE_DROPPED:
		{
			D_MESSAGE(("DiagramView::MessageReceived(M_WIRE_DROPPED)\n"));
			DiagramEndPoint *fromWhich = 0;
			DiagramEndPoint *toWhich = 0;
			bool success = false;
			if (message->FindPointer("from", reinterpret_cast<void **>(&fromWhich)) == B_OK)
			{
				if ((message->FindPointer("to", reinterpret_cast<void **>(&toWhich)) == B_OK)
				 && (message->FindBool("success", &success) == B_OK))
				{
					if (success && fromWhich && toWhich)
					{
						_endWireTracking();
						DiagramWire *wire = createWire(fromWhich, toWhich);
						if (wire && addItem(wire))
						{
							connectionEstablished(fromWhich, toWhich);
							break;
						}
					}
				}
			}
			connectionAborted(fromWhich);
			break;
		}
		default:
		{
			if (message->WasDropped())
			{
				BPoint point = ConvertFromScreen(message->DropPoint());
				DiagramItem *item = itemUnder(point);
				if (item)
				{
					item->messageDropped(point, message);
					return;
				}
				else
				{
					messageDropped(point, message);
				}
			}
			else
			{
				BView::MessageReceived(message);
			}
		}
	}
}

void DiagramView::KeyDown(
	const char *bytes,
	int32 numBytes)
{
	D_METHOD(("DiagramView::KeyDown()\n"));
	switch (bytes[0])
	{
		case B_LEFT_ARROW:
		{
			float x;
			getItemAlignment(&x, 0);
			BRegion updateRegion;
			dragSelectionBy(-x, 0.0, &updateRegion);
			for (int32 i = 0; i < updateRegion.CountRects(); i++)
				Invalidate(updateRegion.RectAt(i));
			updateDataRect();
			break;
		}
		case B_RIGHT_ARROW:
		{
			float x;
			getItemAlignment(&x, 0);
			BRegion updateRegion;
			dragSelectionBy(x, 0.0, &updateRegion);
			for (int32 i = 0; i < updateRegion.CountRects(); i++)
				Invalidate(updateRegion.RectAt(i));
			updateDataRect();
			break;
		}
		case B_UP_ARROW:
		{
			float y;
			getItemAlignment(0, &y);
			BRegion updateRegion;
			dragSelectionBy(0.0, -y, &updateRegion);
			for (int32 i = 0; i < updateRegion.CountRects(); i++)
				Invalidate(updateRegion.RectAt(i));
			updateDataRect();
			break;
		}
		case B_DOWN_ARROW:
		{
			float y;
			getItemAlignment(0, &y);
			BRegion updateRegion;
			dragSelectionBy(0.0, y, &updateRegion);
			for (int32 i = 0; i < updateRegion.CountRects(); i++)
				Invalidate(updateRegion.RectAt(i));
			updateDataRect();
			break;
		}
		default:
		{
			BView::KeyDown(bytes, numBytes);
			break;
		}
	}
}

void DiagramView::MouseDown(
	BPoint point)
{
	D_METHOD(("DiagramView::MouseDown()\n"));

	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);

	// update click count
	BMessage* message = Window()->CurrentMessage();
	int32 clicks = message->FindInt32("clicks");
	int32 buttons = message->FindInt32("buttons");

	bool moved = (fabs(point.x - m_lastClickPoint.x) > 2.0 || fabs(point.y - m_lastClickPoint.y) > 2.0);
	if (!moved && (buttons == m_lastButton) && (clicks > 1))
	{
		m_clickCount++;
	}
	else
	{
		m_clickCount = 1;
	}
	m_lastButton = buttons;
	m_lastClickPoint = point;
	m_lastDragPoint = ConvertToScreen(point);

	// [e.moon 16nov99] scroll on 3rd button
	m_pressedButton = buttons;
	if(m_pressedButton == B_TERTIARY_MOUSE_BUTTON) {
		return;
	}
 
	// was an item clicked ?
 	DiagramItem *item = itemUnder(point);
	if (item)
	{
		item->mouseDown(point, m_lastButton, m_clickCount);
	}
	else // no, the background was clicked
	{
		if (!(modifiers() & B_SHIFT_KEY) && deselectAll(DiagramItem::M_ANY))
			selectionChanged();
		if (multipleSelection() && (m_lastButton == B_PRIMARY_MOUSE_BUTTON) && !(modifiers() & B_CONTROL_KEY))
			_beginRectTracking(point);
		mouseDown(point, m_lastButton, m_clickCount);
	}
}

void DiagramView::MouseMoved(
	BPoint point,
	uint32 transit,
	const BMessage *message)
{
	D_METHOD(("DiagramView::MouseMoved()\n"));


	// [e.moon 16nov99] 3rd-button scrolling
	if(m_pressedButton == B_TERTIARY_MOUSE_BUTTON) {

		// fetch/store screen point; calculate distance travelled
		ConvertToScreen(&point);
		float xDelta = m_lastDragPoint.x - point.x;
		float yDelta = m_lastDragPoint.y - point.y;
		m_lastDragPoint = point;

		// constrain to scrollbar limits
		BScrollBar* hScroll = ScrollBar(B_HORIZONTAL);
		if(xDelta && hScroll) {
			float val = hScroll->Value();
			float min, max;
			hScroll->GetRange(&min, &max);

			if(val + xDelta < min)
				xDelta = 0;

			if(val + xDelta > max)
				xDelta = 0;
		}

		BScrollBar* vScroll = ScrollBar(B_VERTICAL);
		if(yDelta && vScroll) {
			float val = vScroll->Value();
			float min, max;
			vScroll->GetRange(&min, &max);

			if(val + yDelta < min)
				yDelta = 0;

			if(val + yDelta > max)
				yDelta = 0;
		}

		// scroll
		if(xDelta == 0.0 && yDelta == 0.0)
			return;

		ScrollBy(xDelta, yDelta);
		 return;
	}

	if (message)
	{
		switch (message->what)
		{
			case M_RECT_TRACKING:
			{
				BPoint origin;
				if (message->FindPoint("origin", &origin) == B_OK)
				{
					_trackRect(origin, point);
				}
				break;
			}
			case M_BOX_DRAGGED:
			{
				DiagramBox *box;
				BPoint offset;
				if ((message->FindPointer("item", reinterpret_cast<void **>(&box)) == B_OK)
				 && (message->FindPoint("offset", &offset) == B_OK))
				{
					if (box)
					{
						BRegion updateRegion;
						dragSelectionBy(point.x - box->frame().left - offset.x, 
										point.y - box->frame().top - offset.y,
										&updateRegion);
						updateDataRect();
						for (int32 i = 0; i < updateRegion.CountRects(); i++)
						{
							Invalidate(updateRegion.RectAt(i));
						}
					}
				}
				break;
			}
			default: // unkwown message -> redirect to messageDragged()
			{
				DiagramItem *last = lastItemUnder();
				if (transit == B_EXITED_VIEW)
				{
					if (last)
					{
						last->messageDragged(point, B_EXITED_VIEW, message);
						messageDragged(point, B_EXITED_VIEW, message);
					}
				}
				else
				{
					DiagramItem *item = itemUnder(point);
					if (item)
					{
						if (item != last)
						{
							if (last)
								last->messageDragged(point, B_EXITED_VIEW, message);
							item->messageDragged(point, B_ENTERED_VIEW, message);
						}
						else
						{
							item->messageDragged(point, B_INSIDE_VIEW, message);
						}
					}
					else if (last)
					{
						last->messageDragged(point, B_EXITED_VIEW, message);
						messageDragged(point, B_ENTERED_VIEW, message);
					}
					else
					{
						messageDragged(point, transit, message);
					}
				}
				break;
			}
		}
	}
	else // no message at all -> redirect to mouseOver()
	{
		DiagramItem *last = lastItemUnder();
		if ((transit == B_EXITED_VIEW) || (transit == B_OUTSIDE_VIEW))
		{
			if (last)
			{
				last->mouseOver(point, B_EXITED_VIEW);
				resetItemUnder();
				mouseOver(point, B_EXITED_VIEW);
			}
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
				mouseOver(point, B_ENTERED_VIEW);
			}
		}
	}
}

void DiagramView::MouseUp(
	BPoint point)
{
	D_METHOD(("DiagramView::MouseUp()\n"));
	if (multipleSelection())
		EndRectTracking();
	_endWireTracking();

	// [e.moon 16nov99] mark no button as down
	m_pressedButton = 0;
}

// -------------------------------------------------------- //
// *** derived from DiagramItemGroup (public)
// -------------------------------------------------------- //

bool DiagramView::addItem(
	DiagramItem *item)
{
	D_METHOD(("DiagramBox::addItem()\n"));
	if (item)
	{
		if (DiagramItemGroup::addItem(item))
		{
			item->_setOwner(this);
			item->attachedToDiagram();
			if (item->type() == DiagramItem::M_BOX)
			{
				updateDataRect();
			}
			return true;
		}
	}
	return false;
}

bool DiagramView::removeItem(
	DiagramItem *item)
{
	D_METHOD(("DiagramBox::removeItem()\n"));
	if (item)
	{
		item->detachedFromDiagram();
		if (DiagramItemGroup::removeItem(item))
		{
			item->_setOwner(0);
			if (item->type() == DiagramItem::M_BOX)
			{
				updateDataRect();
			}
			return true;
		}
	}
	return false;
}

// -------------------------------------------------------- //
// *** operations (public)
// -------------------------------------------------------- //

void DiagramView::trackWire(
	BPoint point)
{
	D_MOUSE(("DiagramView::trackWire()\n"));
	if (m_draggedWire)
	{
		BRegion region;
		region.Include(m_draggedWire->frame());
		m_draggedWire->m_dragEndPoint = point;
		m_draggedWire->endPointMoved();
		region.Include(m_draggedWire->frame());
		region.Exclude(&m_boxRegion);
		for (int32 i = 0; i < region.CountRects(); i++)
		{
			Invalidate(region.RectAt(i));
		}
	}
}

// -------------------------------------------------------- //
// *** internal operations (protected)
// -------------------------------------------------------- //

void DiagramView::drawBackground(
	BRect updateRect)
{
	D_METHOD(("DiagramView::drawBackground()\n"));
	if (m_useBackgroundBitmap)
	{
		BRegion region;
		region.Include(updateRect);
		region.Exclude(&m_boxRegion);
		BRect bounds = Bounds();
		PushState();
		{
			ConstrainClippingRegion(&region);
			for (float y = 0; y < bounds.bottom; y += m_backgroundBitmap->Bounds().Height())
			{
				for (float x = 0; x < bounds.right; x += m_backgroundBitmap->Bounds().Width())
				{
					DrawBitmapAsync(m_backgroundBitmap, BPoint(x, y));
				}
			}
		}
		PopState();
	}
	else
	{
		BRegion region;
		region.Include(updateRect);
		region.Exclude(&m_boxRegion);
		PushState();
		{
			SetLowColor(m_backgroundColor);
			FillRegion(&region, B_SOLID_LOW);
		}
		PopState();
	}
}

void DiagramView::setBackgroundColor(
	rgb_color color)
{
	D_METHOD(("DiagramView::setBackgroundColor()\n"));
	m_backgroundColor = color;
	m_useBackgroundBitmap = false;
}

void DiagramView::setBackgroundBitmap(
	BBitmap *bitmap)
{
	D_METHOD(("DiagramView::setBackgroundBitmap()\n"));
	if (m_backgroundBitmap)
	{
		delete m_backgroundBitmap;
	}
	m_backgroundBitmap = new BBitmap(bitmap);
	m_useBackgroundBitmap = true;
}

void DiagramView::updateDataRect()
{
	D_METHOD(("DiagramView::updateDataRect()\n"));
	// calculate the area in which boxes display
	m_boxRegion.MakeEmpty();
	for (int32 i = 0; i < countItems(DiagramItem::M_BOX); i++)
	{
		m_boxRegion.Include(itemAt(i, DiagramItem::M_BOX)->frame());
	}
	// adapt the data rect to the new region of boxes
	BRect boxRect = m_boxRegion.Frame();
	m_dataRect.right = boxRect.right;
	m_dataRect.bottom = boxRect.bottom;
	// update the scroll bars
	_updateScrollBars();
}

// -------------------------------------------------------- //
// *** internal operations (private)
// -------------------------------------------------------- //

void DiagramView::_beginWireTracking(
	DiagramEndPoint *startPoint)
{
	D_METHOD(("DiagramView::beginWireTracking()\n"));
	m_draggedWire = createWire(startPoint);
	addItem(m_draggedWire);
	selectItem(m_draggedWire, true);
	Invalidate(startPoint->frame());
}

void DiagramView::_endWireTracking()
{
	D_METHOD(("DiagramView::endWireTracking()\n"));
	if (m_draggedWire)
	{
		removeItem(m_draggedWire);
		Invalidate(m_draggedWire->frame());
		DiagramEndPoint *endPoint = m_draggedWire->startPoint();
		if (!endPoint)
		{
			endPoint = m_draggedWire->endPoint();
		}
		if (endPoint)
		{
			Invalidate(endPoint->frame());
		}
		delete m_draggedWire;
		m_draggedWire = 0;
	}
}

void DiagramView::_beginRectTracking(
	BPoint origin)
{
	D_METHOD(("DiagramView::beginRectTracking()\n"));
	BMessage message(M_RECT_TRACKING);
	message.AddPoint("origin", origin);
	DragMessage(&message, BRect(0.0, 0.0, -1.0, -1.0));
	BView::BeginRectTracking(BRect(origin, origin), B_TRACK_RECT_CORNER);
}

void DiagramView::_trackRect(
	BPoint origin,
	BPoint current)
{
	D_METHOD(("DiagramView::trackRect()\n"));
	bool changed = false;
	BRect rect;
	rect.left = origin.x < current.x ? origin.x : current.x;
	rect.top = origin.y < current.y ? origin.y : current.y;
	rect.right = origin.x < current.x ? current.x : origin.x;
	rect.bottom = origin.y < current.y ? current.y : origin.y;
	for (int32 i = 0; i < countItems(DiagramItem::M_BOX); i++)
	{
		DiagramBox *box = dynamic_cast<DiagramBox *>(itemAt(i, DiagramItem::M_BOX));
		if (box)
		{
			if (rect.Intersects(box->frame()))
			{
				changed  |= selectItem(box, false);
			}
			else
			{
				changed |= deselectItem(box);
			}
		}
	}
	if (changed)
	{
		sortItems(DiagramItem::M_BOX, &compareSelectionTime);
		selectionChanged();
	}
}

void DiagramView::_updateScrollBars()
{
	D_METHOD(("DiagramView::_updateScrollBars()\n"));
	// fetch the vertical ScrollBar
	BScrollBar *scrollBar = ScrollBar(B_VERTICAL);
	if (scrollBar)
	{
		 // Disable the ScrollBar if the view is large enough to display
		 // the entire data-rect
		if (Bounds().Height() > m_dataRect.Height())
		{
			scrollBar->SetRange(0.0, 0.0);
			scrollBar->SetProportion(1.0);
		}
		else
		{
			scrollBar->SetRange(m_dataRect.top, m_dataRect.bottom - Bounds().Height());
			scrollBar->SetProportion(Bounds().Height() / m_dataRect.Height());
		}
	}
	scrollBar = ScrollBar(B_HORIZONTAL);
	if (scrollBar)
	{
		 // Disable the ScrollBar if the view is large enough to display
		 // the entire data-rect
		if (Bounds().Width() > m_dataRect.Width())
		{
			scrollBar->SetRange(0.0, 0.0);
			scrollBar->SetProportion(1.0);
		}
		else
		{
			scrollBar->SetRange(m_dataRect.left, m_dataRect.right - Bounds().Width());
			scrollBar->SetProportion(Bounds().Width() / m_dataRect.Width());
		}
	}
}

// END -- DiagramView.cpp --
