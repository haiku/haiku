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

void DiagramWire::Draw(
	BRect updateRect)
{
	D_METHOD(("DiagramWire::Draw()\n"));
	if (view())
	{
		view()->PushState();
		{
			BRegion region, clipping;
			region.Include(Frame() & updateRect);
			if (view()->GetClippingAbove(this, &clipping))
				region.Exclude(&clipping);
			view()->ConstrainClippingRegion(&region);
			drawWire();
		}
		view()->PopState();
	}
}

void DiagramWire::MouseDown(
	BPoint point,
	uint32 buttons,
	uint32 clicks)
{
	D_METHOD(("DiagramWire::MouseDown()\n"));
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

void DiagramWire::MessageDragged(
	BPoint point,
	uint32 transit,
	const BMessage *message)
{
	D_METHOD(("DiagramWire::MessageDragged()\n"));
	switch (message->what)
	{
		case M_WIRE_DRAGGED:
		{
			view()->trackWire(point);
			break;
		}
		default:
		{
			DiagramItem::MessageDragged(point, transit, message);
		}
	}
}

// END -- DiagramWire.cpp --
