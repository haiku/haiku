/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "Stacking.h"

#include <Debug.h>

#include "SATWindow.h"
#include "Window.h"


#define DEBUG_STACKING

#ifdef DEBUG_STACKING
#	define STRACE_STACKING(x...) debug_printf("SAT Stacking: "x)
#else
#	define STRACE_STACKING(x...) ;
#endif


SATStacking::SATStacking(SATWindow* window)
	:
	fSATWindow(window),
	fStackingCandidate(NULL)
{
	
}


SATStacking::~SATStacking()
{
	
}


bool
SATStacking::FindSnappingCandidates(SATGroup* group)
{
	_ClearSearchResult();

	Window* window = fSATWindow->GetWindow();
	BPoint leftTop = window->Decorator()->TabRect().LeftTop();

	for (int i = 0; i < group->CountItems(); i++) {
		SATWindow* satWindow = group->WindowAt(i);
		// search for stacking candidate
		Window* win = satWindow->GetWindow();
		if (win != window && win->Decorator()
			&& win->Decorator()->TabRect().Contains(leftTop)) {
			// remember window as the candidate for stacking
			fStackingCandidate = satWindow;
			_HighlightWindows(true);
			return true;
		}
	}

	return false;
}


bool
SATStacking::JoinCandidates()
{
	if (!fStackingCandidate)
		return false;
	SATGroup* group = fStackingCandidate->GetGroup();
	WindowArea* area = fStackingCandidate->GetWindowArea();
	if (!group || !area) {
		_ClearSearchResult();
		return false;
	}

	bool status = group->AddWindow(fSATWindow, area, fStackingCandidate);

	if (status) {
		area->WindowList().ItemAt(0)->SetStackedMode(true);
			// for the case we are the first added window
		fSATWindow->SetStackedMode(true);
	}

	fStackingCandidate->DoGroupLayout();
	_ClearSearchResult();
	return status;
}


void
SATStacking::DoGroupLayout()
{
	_AdjustWindowTabs();
}


void
SATStacking::RemovedFromArea(WindowArea* area)
{
	const SATWindowList& list = area->WindowList();
	if (list.CountItems() == 1)
		list.ItemAt(0)->SetStackedMode(false);
	else if (list.CountItems() > 0)
		list.ItemAt(0)->DoGroupLayout();

	fSATWindow->SetStackedMode(false);
}


void
SATStacking::TabLocationMoved(float location, bool shifting)
{
	if (!shifting) {
		_AdjustWindowTabs();
		return;
	}

	SATDecorator* decorator = fSATWindow->GetDecorator();
	Desktop* desktop = fSATWindow->GetWindow()->Desktop();
	WindowArea* area = fSATWindow->GetWindowArea();
	if (!desktop || !area || ! decorator)
		return;

	const SATWindowList& stackedWindows = area->WindowList();
	ASSERT(stackedWindows.CountItems() > 0);
	int32 windowIndex = stackedWindows.IndexOf(fSATWindow);
	ASSERT(windowIndex >= 0);
	float tabLength = stackedWindows.ItemAt(0)->GetDecorator()
		->StackedTabLength();
	float tabLengthZoom = tabLength + decorator->GetZoomOffsetToRight();

	float oldTabPosition = windowIndex * (tabLength + 1);
	if (fabs(oldTabPosition - location) < tabLength / 2)
		return;

	int32 neighbourIndex = windowIndex;
	if (oldTabPosition > location)
		neighbourIndex--;
	else
		neighbourIndex++;

	SATWindow* neighbour = stackedWindows.ItemAt(neighbourIndex);
	if (!neighbour)
		return;

	float newNeighbourPosition = windowIndex * (tabLength + 1);
	area->MoveWindowToPosition(fSATWindow, neighbourIndex);
	desktop->SetWindowTabLocation(neighbour->GetWindow(), newNeighbourPosition);

	// update zoom buttons
	if (windowIndex == stackedWindows.CountItems() - 1) {
		fSATWindow->SetStackedTabLength(tabLength, false);
		neighbour->SetStackedTabLength(tabLengthZoom, true);
	}
	else if (neighbourIndex == stackedWindows.CountItems() - 1) {
		neighbour->SetStackedTabLength(tabLength, false);
		fSATWindow->SetStackedTabLength(tabLengthZoom, true);
	}
}


void
SATStacking::_ClearSearchResult()
{
	if (!fStackingCandidate)
		return;

	_HighlightWindows(false);
	fStackingCandidate = NULL;
}


void
SATStacking::_HighlightWindows(bool highlight)
{
	Desktop* desktop = fSATWindow->GetWindow()->Desktop();
	if (!desktop)
		return;
	fStackingCandidate->HighlightTab(highlight);
	fSATWindow->HighlightTab(highlight);
}


bool
SATStacking::_AdjustWindowTabs()
{
	SATDecorator* decorator = fSATWindow->GetDecorator();
	Desktop* desktop = fSATWindow->GetWindow()->Desktop();
	WindowArea* area = fSATWindow->GetWindowArea();
	if (!desktop || !area || ! decorator)
		return false;

	if (!decorator->StackedMode())
		return false;

	BRect frame = fSATWindow->CompleteWindowFrame();

	const float kMaxTabWidth = 120.;

	const SATWindowList& stackedWindows = area->WindowList();

	float zoomOffset = decorator->GetZoomOffsetToRight();
	float tabBarLength = frame.Width() - zoomOffset;

	ASSERT(tabBarLength > 0);
	float tabLength = tabBarLength / stackedWindows.CountItems();
	if (tabLength > kMaxTabWidth)
		tabLength = kMaxTabWidth;

	float location = 0;
	for (int i = 0; i < stackedWindows.CountItems(); i++) {
		SATWindow* window = stackedWindows.ItemAt(i);
		if (i == stackedWindows.CountItems() - 1)
			window->SetStackedTabLength(tabLength - 1 + zoomOffset, true);
				// last tab
		else
			window->SetStackedTabLength(tabLength - 1, false);

		desktop->SetWindowTabLocation(window->GetWindow(), location);
		location += tabLength;
	}
	return true;
}
