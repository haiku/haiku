/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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
	return BPoint(Frame().left + Frame().Height() / 2.0, Frame().top + Frame().Width() / 2.0);
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

void DiagramEndPoint::Draw(
	BRect updateRect)
{
	D_METHOD(("DiagramEndPoint::Draw()\n"));
	if (view())
	{
		view()->PushState();
		{
			BRegion region;
			region.Include(Frame() & updateRect);
			view()->ConstrainClippingRegion(&region);
			drawEndPoint();
		}
		view()->PopState();
	}
}

void DiagramEndPoint::MouseDown(
	BPoint point,
	uint32 buttons,
	uint32 clicks)
{
	D_MOUSE(("DiagramEndPoint::MouseDown()\n"));
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
			view()->MessageDragged(point, B_INSIDE_VIEW, &dragMsg);
		}
	}
}

void DiagramEndPoint::MessageDragged(
	BPoint point,
	uint32 transit,
	const BMessage *message)
{
	D_MOUSE(("DiagramEndPoint::MessageDragged()\n"));
	switch (message->what)
	{
		case M_WIRE_DRAGGED:
		{
			D_MESSAGE(("DiagramEndPoint::MessageDragged(M_WIRE_DRAGGED)\n"));
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
			DiagramItem::MessageDragged(point, transit, message);
		}
	}
}

void DiagramEndPoint::MessageDropped(
	BPoint point,
	BMessage *message)
{
	D_MESSAGE(("DiagramEndPoint::MessageDropped()\n"));
	switch (message->what)
	{
		case M_WIRE_DRAGGED:
		{
			D_MESSAGE(("DiagramEndPoint::MessageDropped(M_WIRE_DRAGGED)\n"));
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
			DiagramItem::MessageDropped(point, message);
		}
	}
}

// -------------------------------------------------------- //
// *** frame related operations
// -------------------------------------------------------- //

void DiagramEndPoint::MoveBy(
	float x,
	float y,
	BRegion *updateRegion)
{
	D_METHOD(("DiagramEndPoint::MoveBy()\n"));
	if (isConnected() && m_wire && updateRegion)
	{	
		updateRegion->Include(m_wire->Frame());
		m_frame.OffsetBy(x, y);
		m_wire->endPointMoved(this);
		updateRegion->Include(m_wire->Frame());
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

void DiagramEndPoint::ResizeBy(
	float horizontal,
	float vertical)
{
	D_METHOD(("DiagramEndPoint::ResizeBy()\n"));
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
