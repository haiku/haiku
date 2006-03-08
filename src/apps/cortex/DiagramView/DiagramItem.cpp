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
		view()->Invalidate(frame());
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
		view()->Invalidate(frame());
	}
}

void DiagramItem::deselect()
{
	D_METHOD(("DiagramItem::deselect()\n"));
	if (m_selected)
	{
		m_selected = false;
		deselected();
		view()->Invalidate(frame());
	}
}

// -------------------------------------------------------- //
// *** hook functions (public)
// -------------------------------------------------------- //

float DiagramItem::howCloseTo(
	BPoint point) const
{
	D_METHOD(("DiagramItem::howCloseTo()\n"));
	if (frame().Contains(point))
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
