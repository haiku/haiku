/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "Stacking.h"

#include <Debug.h>

#include "StackAndTilePrivate.h"

#include "SATWindow.h"
#include "Window.h"


#define DEBUG_STACKING

#ifdef DEBUG_STACKING
#	define STRACE_STACKING(x...) debug_printf("SAT Stacking: "x)
#else
#	define STRACE_STACKING(x...) ;
#endif


using namespace BPrivate;


const float kMaxTabWidth = 135.;


bool
StackingEventHandler::HandleMessage(SATWindow* sender,
	BPrivate::ServerLink& link)
{
	Desktop* desktop = sender->GetDesktop();
	StackAndTile* stackAndTile = sender->GetStackAndTile();

	int32 what;
	link.Read<int32>(&what);

	switch (what) {
		case kAddWindowToStack:
		{
			port_id port;
			int32 token;
			team_id team;
			link.Read<port_id>(&port);
			link.Read<int32>(&token);
			link.Read<team_id>(&team);
			int32 position;
			if (link.Read<int32>(&position) != B_OK)
				return false;

			WindowArea* area = sender->GetWindowArea();
			if (!area)
				return false;
			if (position < 0)
				position = area->WindowList().CountItems() - 1;

			SATWindow* parent = area->WindowList().ItemAt(position);
			Window* window = desktop->WindowForClientLooperPort(port);
			if (!parent || !window) {
				link.StartMessage(B_BAD_VALUE);
				link.Flush();
				break;
			}

			SATWindow* candidate = stackAndTile->GetSATWindow(window);
			if (!candidate)
				return false;
			if (!parent->StackWindow(candidate))
				return false;

			link.StartMessage(B_OK);
			link.Flush();
			break;
		}
		case kRemoveWindowFromStack:
		{
			port_id port;
			int32 token;
			team_id team;
			link.Read<port_id>(&port);
			link.Read<int32>(&token);
			if (link.Read<team_id>(&team) != B_OK)
				return false;

			SATGroup* group = sender->GetGroup();
			if (!group)
				return false;

			Window* window = desktop->WindowForClientLooperPort(port);
			if (!window) {
				link.StartMessage(B_BAD_VALUE);
				link.Flush();
				break;
			}
			SATWindow* candidate = stackAndTile->GetSATWindow(window);
			if (!candidate)
				return false;
			if (!group->RemoveWindow(candidate))
				return false;
			break;
		}
		case kRemoveWindowFromStackAt:
		{
			int32 position;
			if (link.Read<int32>(&position) != B_OK)
				return false;
			SATGroup* group = sender->GetGroup();
			WindowArea* area = sender->GetWindowArea();
			if (!area || !group)
				return false;
			SATWindow* removeWindow = area->WindowList().ItemAt(position);
			if (!removeWindow) {
				link.StartMessage(B_BAD_VALUE);
				link.Flush();
				break;
			}

			if (!group->RemoveWindow(removeWindow))
				return false;

			ServerWindow* window = removeWindow->GetWindow()->ServerWindow();
			link.StartMessage(B_OK);
			link.Attach<port_id>(window->ClientLooperPort());
			link.Attach<int32>(window->ClientToken());
			link.Attach<team_id>(window->ClientTeam());
			link.Flush();
			break;
		}
		case kCountWindowsOnStack:
		{
			WindowArea* area = sender->GetWindowArea();
			if (!area)
				return false;
			link.StartMessage(B_OK);
			link.Attach<int32>(area->WindowList().CountItems());
			link.Flush();
			break;
		}
		case kWindowOnStackAt:
		{
			int32 position;
			if (link.Read<int32>(&position) != B_OK)
				return false;
			WindowArea* area = sender->GetWindowArea();
			if (!area)
				return false;
			SATWindow* satWindow = area->WindowList().ItemAt(position);
			if (!satWindow) {
				link.StartMessage(B_BAD_VALUE);
				link.Flush();
				break;
			}

			ServerWindow* window = satWindow->GetWindow()->ServerWindow();
			link.StartMessage(B_OK);
			link.Attach<port_id>(window->ClientLooperPort());
			link.Attach<int32>(window->ClientToken());
			link.Attach<team_id>(window->ClientTeam());
			link.Flush();
			break;
		}
		case kStackHasWindow:
		{
			port_id port;
			int32 token;
			team_id team;
			link.Read<port_id>(&port);
			link.Read<int32>(&token);
			if (link.Read<team_id>(&team) != B_OK)
				return false;

			Window* window = desktop->WindowForClientLooperPort(port);
			if (!window) {
				link.StartMessage(B_BAD_VALUE);
				link.Flush();
				break;
			}
			SATWindow* candidate = stackAndTile->GetSATWindow(window);
			if (!candidate)
				return false;

			WindowArea* area = sender->GetWindowArea();
			if (!area)
				return false;
			link.StartMessage(B_OK);
			link.Attach<bool>(area->WindowList().HasItem(candidate));
			link.Flush();
			break;
		}
		default:
			return false;
	}
	return true;
}


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

	bool result = fStackingCandidate->StackWindow(fSATWindow);

	_ClearSearchResult();
	return result;
}


void
SATStacking::DoWindowLayout()
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

	const SATWindowList& stackedWindows = area->WindowList();

	float tabBarLength = frame.Width();

	ASSERT(tabBarLength > 0);
	float tabLength = tabBarLength / stackedWindows.CountItems();
	if (tabLength > kMaxTabWidth)
		tabLength = kMaxTabWidth;

	float location = 0;
	for (int i = 0; i < stackedWindows.CountItems(); i++) {
		SATWindow* window = stackedWindows.ItemAt(i);
		window->SetStackedTabLength(tabLength - 1);

		desktop->SetWindowTabLocation(window->GetWindow(), location);
		location += tabLength;
	}
	return true;
}
