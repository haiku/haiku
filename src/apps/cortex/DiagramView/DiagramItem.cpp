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


// DiagramItem.cpp

#include "DiagramItem.h"
#include "DiagramView.h"

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_METHOD(x) //PRINT(x)

// -------------------------------------------------------- //
// *** static member initialization
// -------------------------------------------------------- //

bigtime_t DiagramItem::m_lastSelectionTime	= 0;
int32 DiagramItem::m_countSelected			= 0;

// -------------------------------------------------------- //
// *** ctor/dtor (public)
// -------------------------------------------------------- //

DiagramItem::DiagramItem(
	uint32 itemType)
	: m_type(itemType),
	  m_view(0),
	  m_group(0),
	  m_draggable(false),
	  m_selectable(true),
	  m_selected(false),
	  m_selectionTime(system_time())
{
	D_METHOD(("DiagramItem::DiagramItem()\n"));
}

DiagramItem::~DiagramItem()
{
	D_METHOD(("DiagramItem::~DiagramItem()\n"));
}

// -------------------------------------------------------- //
// *** operations (public)
// -------------------------------------------------------- //

void DiagramItem::select()
{
	D_METHOD(("DiagramItem::select()\n"));
	if (!m_selected)
	{
		m_selected = true;
		m_lastSelectionTime = m_selectionTime = system_time();
		m_countSelected = 1;
		selected();
		view()->Invalidate(Frame());
	}
}

void DiagramItem::selectAdding()
{
	D_METHOD(("DiagramItem::selectAdding()\n"));
	if (!m_selected)
	{
		m_selected = true;
		m_selectionTime = m_lastSelectionTime - m_countSelected++;
		selected();
		view()->Invalidate(Frame());
	}
}

void DiagramItem::deselect()
{
	D_METHOD(("DiagramItem::deselect()\n"));
	if (m_selected)
	{
		m_selected = false;
		deselected();
		view()->Invalidate(Frame());
	}
}

// -------------------------------------------------------- //
// *** hook functions (public)
// -------------------------------------------------------- //

float DiagramItem::howCloseTo(
	BPoint point) const
{
	D_METHOD(("DiagramItem::howCloseTo()\n"));
	if (Frame().Contains(point))
	{
		return 1.0;
	}
	return 0.0;
}

// -------------------------------------------------------- //
// *** compare functions (friend)
// -------------------------------------------------------- //

int __CORTEX_NAMESPACE__ compareSelectionTime(
	const void *lValue,
	const void *rValue)
{
	D_METHOD(("compareSelectionTime()\n"));
	int returnValue = 0;
	const DiagramItem *lItem = *(reinterpret_cast<const DiagramItem* const*>(reinterpret_cast<const void* const*>(lValue)));
	const DiagramItem *rItem = *(reinterpret_cast<const DiagramItem* const*>(reinterpret_cast<const void* const*>(rValue)));
	if (lItem->m_selectionTime < rItem->m_selectionTime)
	{
		returnValue = 1;
	}
	else if (lItem->m_selectionTime > rItem->m_selectionTime)
	{
		returnValue = -1;
	}
	return returnValue;
}

// END -- DiagramItem.cpp --
