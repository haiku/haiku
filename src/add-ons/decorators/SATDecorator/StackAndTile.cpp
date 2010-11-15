/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include "StackAndTile.h"

#include <Debug.h>

#include "StackAndTilePrivate.h"

#include "Desktop.h"
#include "SATWindow.h"
#include "Tiling.h"
#include "Window.h"


using namespace std;


StackAndTile::StackAndTile()
	:
	fSATKeyPressed(false),
	fCurrentSATWindow(NULL),
	fTabIsShifting(false)
{

}


StackAndTile::~StackAndTile()
{

}


int32
StackAndTile::Identifier()
{
	return BPrivate::kMagicSATIdentifier;
}


void
StackAndTile::ListenerRegistered(Desktop* desktop)
{
	WindowList& windows = desktop->AllWindows();
	for (Window *window = windows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList))
		WindowAdded(window);
}


void
StackAndTile::ListenerUnregistered()
{
	for (SATWindowMap::iterator it = fSATWindowMap.begin();
		it != fSATWindowMap.end(); it++) {
		SATWindow* satWindow = it->second;
		delete satWindow;
	}
	fSATWindowMap.clear();
}


bool
StackAndTile::HandleMessage(Window* sender, BPrivate::ServerLink& link)
{
	SATWindow* satWindow = GetSATWindow(sender);
	if (!satWindow)
		return false;

	return satWindow->HandleMessage(satWindow, link);
}


void
StackAndTile::WindowAdded(Window* window)
{
	SATWindow* satWindow = new (std::nothrow)SATWindow(this, window);
	if (!satWindow)
		return;

	ASSERT(fSATWindowMap.find(window) == fSATWindowMap.end());
	fSATWindowMap[window] = satWindow;
}


void
StackAndTile::WindowRemoved(Window* window)
{
	STRACE_SAT("StackAndTile::WindowRemoved %s\n", window->Title());

	SATWindowMap::iterator it = fSATWindowMap.find(window);
	if (it == fSATWindowMap.end())
		return;

	SATWindow* satWindow = it->second;
	// delete SATWindow
	delete satWindow;
	fSATWindowMap.erase(it);
}


void
StackAndTile::KeyPressed(uint32 what, int32 key, int32 modifiers)
{
	// switch to and from stacking and snapping mode
	if (what == B_MODIFIERS_CHANGED) {
		bool wasPressed = fSATKeyPressed;
		fSATKeyPressed = modifiers & B_OPTION_KEY;
		if (wasPressed && !fSATKeyPressed)
			_StopSAT();
		if (!wasPressed && fSATKeyPressed)
			_StartSAT();
	}

	return;
}


void
StackAndTile::MouseDown(Window* window, BMessage* message, const BPoint& where)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow || !satWindow->GetDecorator())
		return;

	// we are only interested in single clicks
	if (message->FindInt32("clicks") == 2)
		return;
	int32 modifiers = message->FindInt32("modifiers");
	int32 buttons = message->FindInt32("buttons");
	click_type clickArea = satWindow->GetDecorator()->MouseAction(message,
		where, buttons, modifiers);
	if (clickArea != CLICK_DRAG && clickArea < CLICK_RESIZE)
		return;

	ASSERT(fCurrentSATWindow == NULL);
	fCurrentSATWindow = satWindow;

	if (!SATKeyPressed())
		return;

	_StartSAT();
}


void
StackAndTile::MouseUp(Window* window, BMessage* message, const BPoint& where)
{
	if (fTabIsShifting) {
		SATWindow*	satWindow = GetSATWindow(window);
		if (satWindow) {
			fTabIsShifting = false;
			satWindow->TabLocationMoved(satWindow->GetWindow()->TabLocation(),
				fTabIsShifting);
		}
	}

	if (fSATKeyPressed)
		_StopSAT();

	fCurrentSATWindow = NULL;
}


void
StackAndTile::WindowMoved(Window* window)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;

	if (SATKeyPressed() && fCurrentSATWindow)
		satWindow->FindSnappingCandidates();
	else
		satWindow->DoGroupLayout();
}


void
StackAndTile::WindowResized(Window* window)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	satWindow->Resized();

	if (SATKeyPressed() && fCurrentSATWindow)
		satWindow->FindSnappingCandidates();
	else {
		satWindow->DoGroupLayout();

		// Do a window layout for all windows. TODO: maybe do it a bit more
		// efficient
		SATGroup* group = satWindow->GetGroup();
		if (!group)
			return;
		for (int i = 0; i < group->CountItems(); i++) {
			SATWindow* listWindow = group->WindowAt(i);
			if (listWindow != satWindow)
				listWindow->DoWindowLayout();
		}
	}
}


void
StackAndTile::WindowActitvated(Window* window)
{
	SATWindow*	satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	_ActivateWindow(satWindow);
}


void
StackAndTile::WindowSentBehind(Window* window, Window* behindOf)
{
	SATWindow*	satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	SATGroup* group = satWindow->GetGroup();
	if (!group)
		return;
	Desktop* desktop = satWindow->GetWindow()->Desktop();
	if (!desktop)
		return;

	WindowIterator iter(group, true);
	for (SATWindow* listWindow = iter.NextWindow(); listWindow != NULL;
		listWindow = iter.NextWindow()) {
		if (listWindow != satWindow)
			desktop->SendWindowBehind(listWindow->GetWindow(), behindOf);
	}
}


void
StackAndTile::WindowWorkspacesChanged(Window* window, uint32 workspaces)
{
	SATWindow*	satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	SATGroup* group = satWindow->GetGroup();
	if (!group)
		return;
	Desktop* desktop = satWindow->GetWindow()->Desktop();
	if (!desktop)
		return;

	for (int i = 0; i < group->CountItems(); i++) {
		SATWindow* listWindow = group->WindowAt(i);
		if (listWindow != satWindow)
			desktop->SetWindowWorkspaces(listWindow->GetWindow(), workspaces);
	}
}


void
StackAndTile::WindowMinimized(Window* window, bool minimize)
{
	SATWindow*	satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	SATGroup* group = satWindow->GetGroup();
	if (!group)
		return;
	Desktop* desktop = satWindow->GetWindow()->Desktop();
	if (!desktop)
		return;

	for (int i = 0; i < group->CountItems(); i++) {
		SATWindow* listWindow = group->WindowAt(i);
		if (listWindow != satWindow)
			listWindow->GetWindow()->ServerWindow()->NotifyMinimize(minimize);
	}
}


void
StackAndTile::WindowTabLocationChanged(Window* window, float location)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;

	fTabIsShifting = true;
	satWindow->TabLocationMoved(location, fTabIsShifting);
}


void
StackAndTile::SizeLimitsChanged(Window* window, int32 minWidth, int32 maxWidth,
	int32 minHeight, int32 maxHeight)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	WindowArea* area = satWindow->GetWindowArea();
	if (!area)
		return;
	area->UpdateSizeLimits();
}


void
StackAndTile::WindowLookChanged(Window* window, window_look look)
{
	// if the decorator has been removed remove it from the stacking group
	if (look != B_NO_BORDER_WINDOW_LOOK)
		return;

	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	SATGroup* group = satWindow->GetGroup();
	if (!group)
		return;
	group->RemoveWindow(satWindow);	
}


bool
StackAndTile::SetDecoratorSettings(Window* window, const BMessage& settings)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return false;

	//return satWindow->SetSettings(settings);
	return false;
}


void
StackAndTile::GetDecoratorSettings(Window* window, BMessage& settings)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;

	//satWindow->GetSettings(&settings);
}


SATWindow*
StackAndTile::GetSATWindow(Window* window)
{
	SATWindowMap::const_iterator it = fSATWindowMap.find(
		window);
	if (it != fSATWindowMap.end())
		return it->second;

	// for now don't create window on the fly
	return NULL;
	// If we don't know this window, memory allocation might has been failed
	// previously. Try to add window now
/*	SATWindow* satWindow = new (std::nothrow)SATWindow(
		window, this);
	if (satWindow)
		fSATWindowMap[window] = satWindow;

	return satWindow;*/
}


void
StackAndTile::_StartSAT()
{
	STRACE_SAT("StackAndTile::_StartSAT()\n");
	if (!fCurrentSATWindow)
		return;

	// Remove window from the group.
	SATGroup* group = fCurrentSATWindow->GetGroup();
	if (!group)
		return;

	group->RemoveWindow(fCurrentSATWindow);
	// Bring window to the front. (in focus follow mouse this is not
	// automatically the case)
	_ActivateWindow(fCurrentSATWindow);

	fCurrentSATWindow->FindSnappingCandidates();
}


void
StackAndTile::_StopSAT()
{
	STRACE_SAT("StackAndTile::_StopSAT()\n");
	if (!fCurrentSATWindow)
		return;
	if (fCurrentSATWindow->JoinCandidates())
		_ActivateWindow(fCurrentSATWindow);
}


void
StackAndTile::_ActivateWindow(SATWindow* satWindow)
{
	SATGroup* group = satWindow->GetGroup();
	if (!group)
		return;
	Desktop* desktop = satWindow->GetWindow()->Desktop();
	if (!desktop)
		return;
	WindowArea* area = satWindow->GetWindowArea();
	if (!area)
		return;
	area->MoveToTopLayer(satWindow);

	//desktop->ActivateWindow(satWindow->GetWindow());

	WindowIterator iter(group);
	for (SATWindow* listWindow = iter.NextWindow(); listWindow != NULL;
		listWindow = iter.NextWindow()) {
		if (listWindow != satWindow)
			//desktop->SendWindowBehind(listWindow->GetWindow(),
			//	satWindow->GetWindow());
				desktop->ActivateWindow(listWindow->GetWindow());
	}

	desktop->ActivateWindow(satWindow->GetWindow());
}


GroupIterator::GroupIterator(StackAndTile* sat, Desktop* desktop)
	:
	fStackAndTile(sat),
	fDesktop(desktop),
	fCurrentGroup(NULL)
{
	RewindToFront();
}


void
GroupIterator::RewindToFront()
{
	fCurrentWindow = fDesktop->CurrentWindows().LastWindow();
}


SATGroup*
GroupIterator::NextGroup()
{
	SATGroup* group = NULL;
	do {
		Window* window = fCurrentWindow;
		if (window == NULL) {
			group = NULL;
			break;
		}
		fCurrentWindow = fCurrentWindow->PreviousWindow(
				fCurrentWindow->CurrentWorkspace());
		if (window->IsHidden()
			|| strcmp(window->Title(), "Deskbar") == 0
			|| strcmp(window->Title(), "Desktop") == 0)
			continue;

		SATWindow* satWindow = fStackAndTile->GetSATWindow(window);
		group = satWindow->GetGroup();
	} while (group == NULL || fCurrentGroup == group);

	fCurrentGroup = group;
	return fCurrentGroup;
}


WindowIterator::WindowIterator(SATGroup* group, bool reverseLayerOrder)
	:
	fGroup(group),
	fReverseLayerOrder(reverseLayerOrder)
{
	if (fReverseLayerOrder)
		_ReverseRewind();
	else
		Rewind();
}


void
WindowIterator::Rewind()
{
	fAreaIndex = 0;
	fWindowIndex = 0;
	fCurrentArea = fGroup->GetAreaList().ItemAt(fAreaIndex);
}


SATWindow*
WindowIterator::NextWindow()
{
	if (fReverseLayerOrder)
		return _ReverseNextWindow();

	if (fWindowIndex == fCurrentArea->LayerOrder().CountItems()) {
		fAreaIndex++;
		fWindowIndex = 0;
		fCurrentArea = fGroup->GetAreaList().ItemAt(fAreaIndex);
		if (!fCurrentArea)
			return NULL;
	}
	SATWindow* window = fCurrentArea->LayerOrder().ItemAt(fWindowIndex);
	fWindowIndex++;
	return window;
}


SATWindow*
WindowIterator::_ReverseNextWindow()
{
	if (fWindowIndex < 0) {
		fAreaIndex++;
		fCurrentArea = fGroup->GetAreaList().ItemAt(fAreaIndex);
		if (!fCurrentArea)
			return NULL;
		fWindowIndex = fCurrentArea->LayerOrder().CountItems() - 1;
	}
	SATWindow* window = fCurrentArea->LayerOrder().ItemAt(fWindowIndex);
	fWindowIndex--;
	return window;
}


void
WindowIterator::_ReverseRewind()
{
	Rewind();
	if (fCurrentArea)
		fWindowIndex = fCurrentArea->LayerOrder().CountItems() - 1;
}


SATSnappingBehaviour::~SATSnappingBehaviour()
{
	
}
