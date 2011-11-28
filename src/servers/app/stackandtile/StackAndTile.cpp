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
	fDesktop(NULL),
	fSATKeyPressed(false),
	fCurrentSATWindow(NULL)
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
	fDesktop = desktop;

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
StackAndTile::HandleMessage(Window* sender, BPrivate::LinkReceiver& link,
	BPrivate::LinkSender& reply)
{
	if (sender == NULL)
		return _HandleMessage(link, reply);

	SATWindow* satWindow = GetSATWindow(sender);
	if (!satWindow)
		return false;

	return satWindow->HandleMessage(satWindow, link, reply);
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


bool
StackAndTile::KeyPressed(uint32 what, int32 key, int32 modifiers)
{
	const int32 kRightOptionKey = 103;
	if (what == B_MODIFIERS_CHANGED
		|| (what == B_UNMAPPED_KEY_DOWN && key == kRightOptionKey)
		|| (what == B_UNMAPPED_KEY_UP && key == kRightOptionKey)) {
		// switch to and from stacking and snapping mode
		bool wasPressed = fSATKeyPressed;
		fSATKeyPressed = (what == B_MODIFIERS_CHANGED
			&& modifiers & B_OPTION_KEY)
			|| (what == B_UNMAPPED_KEY_DOWN && key == kRightOptionKey);
		if (wasPressed && !fSATKeyPressed)
			_StopSAT();
		if (!wasPressed && fSATKeyPressed)
			_StartSAT();
	}
// switch off group navigation because it clashes with tracker...
return false;
	if (!SATKeyPressed() || (modifiers & B_COMMAND_KEY) == 0
		|| what != B_KEY_DOWN)
		return false;

	const int kArrowKeyUp = 87;
	const int kArrowKeyDown = 98;

	switch (key) {
		case kArrowKeyDown:
		{
			SATWindow* frontWindow = GetSATWindow(fDesktop->FocusWindow());
			SATGroup* currentGroup = NULL;
			if (frontWindow)
				currentGroup = frontWindow->GetGroup();
			if (currentGroup && currentGroup->CountItems() <= 1)
				currentGroup = NULL;

			GroupIterator groups(this, fDesktop);
			bool currentFound = false;
			while (true) {
				SATGroup* group = groups.NextGroup();
				if (group == NULL)
					break;
				if (group->CountItems() <= 1)
					continue;

				if (currentGroup == NULL)
					currentFound = true;
						// if no group is selected just activate the first one
				else if (currentGroup == group) {
					currentFound = true;
					continue;
				}
				if (currentFound) {
					_ActivateWindow(group->WindowAt(0));
					if (currentGroup) {
						Window* window = currentGroup->WindowAt(0)->GetWindow();
						fDesktop->SendWindowBehind(window);
						WindowSentBehind(window, NULL);
					}
					return true;
				}
			}
			break;
		}

		case kArrowKeyUp:
		{
			SATWindow* frontWindow = GetSATWindow(fDesktop->FocusWindow());
			SATGroup* currentGroup = NULL;
			if (frontWindow)
				currentGroup = frontWindow->GetGroup();
			if (currentGroup && currentGroup->CountItems() <= 1)
				currentGroup = NULL;

			SATGroup* backmostGroup = NULL;
			GroupIterator groups(this, fDesktop);
			while (true) {
				SATGroup* group = groups.NextGroup();
				if (group == NULL)
					break;
				if (group->CountItems() <= 1)
					continue;
				// if no group is selected just activate the first one
				if (currentGroup == NULL) {
					_ActivateWindow(group->WindowAt(0));
					return true;
				}
				backmostGroup = group;
			}
			if (backmostGroup && backmostGroup != currentGroup) {
				_ActivateWindow(backmostGroup->WindowAt(0));
				return true;
			}
			break;
		}
	}
	return false;
}


void
StackAndTile::MouseDown(Window* window, BMessage* message, const BPoint& where)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow || !satWindow->GetDecorator())
		return;

	// fCurrentSATWindow is not zero if e.g. the secondary and the primary
	// mouse button are pressed at the same time
	if ((message->FindInt32("buttons") & B_PRIMARY_MOUSE_BUTTON) == 0 ||
		fCurrentSATWindow != NULL)
		return;

	// we are only interested in single clicks
	if (message->FindInt32("clicks") == 2)
		return;

	int32 tab;
	switch (satWindow->GetDecorator()->RegionAt(where, tab)) {
		case Decorator::REGION_TAB:
		case Decorator::REGION_LEFT_BORDER:
		case Decorator::REGION_RIGHT_BORDER:
		case Decorator::REGION_TOP_BORDER:
		case Decorator::REGION_BOTTOM_BORDER:
		case Decorator::REGION_LEFT_TOP_CORNER:
		case Decorator::REGION_LEFT_BOTTOM_CORNER:
		case Decorator::REGION_RIGHT_TOP_CORNER:
		case Decorator::REGION_RIGHT_BOTTOM_CORNER:
			break;

		default:
			return;
	}

	ASSERT(fCurrentSATWindow == NULL);
	fCurrentSATWindow = satWindow;

	if (!SATKeyPressed())
		return;

	_StartSAT();
}


void
StackAndTile::MouseUp(Window* window, BMessage* message, const BPoint& where)
{
	if (fSATKeyPressed)
		_StopSAT();

	fCurrentSATWindow = NULL;
}


void
StackAndTile::WindowMoved(Window* window)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
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
	if (satWindow == NULL)
		return;
	satWindow->Resized();

	if (SATKeyPressed() && fCurrentSATWindow)
		satWindow->FindSnappingCandidates();
	else
		satWindow->DoGroupLayout();
}


void
StackAndTile::WindowActitvated(Window* window)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;
	_ActivateWindow(satWindow);
}


void
StackAndTile::WindowSentBehind(Window* window, Window* behindOf)
{
}


void
StackAndTile::WindowWorkspacesChanged(Window* window, uint32 workspaces)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;
	SATGroup* group = satWindow->GetGroup();
	if (group == NULL)
		return;
	Desktop* desktop = satWindow->GetWindow()->Desktop();
	if (desktop == NULL)
		return;

	for (int i = 0; i < group->CountItems(); i++) {
		SATWindow* listWindow = group->WindowAt(i);
		if (listWindow != satWindow)
			desktop->SetWindowWorkspaces(listWindow->GetWindow(), workspaces);
	}
}


void
StackAndTile::WindowHidden(Window* window, bool fromMinimize)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;
	SATGroup* group = satWindow->GetGroup();
	if (group == NULL)
		return;
	if (fromMinimize == false && group->CountItems() > 1)
		group->RemoveWindow(satWindow, false);
}


void
StackAndTile::WindowMinimized(Window* window, bool minimize)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;
	SATGroup* group = satWindow->GetGroup();
	if (group == NULL)
		return;
	Desktop* desktop = satWindow->GetWindow()->Desktop();
	if (desktop == NULL)
		return;

	for (int i = 0; i < group->CountItems(); i++) {
		SATWindow* listWindow = group->WindowAt(i);
		if (listWindow != satWindow)
			listWindow->GetWindow()->ServerWindow()->NotifyMinimize(minimize);
	}
}


void
StackAndTile::WindowTabLocationChanged(Window* window, float location,
	bool isShifting)
{

}


void
StackAndTile::SizeLimitsChanged(Window* window, int32 minWidth, int32 maxWidth,
	int32 minHeight, int32 maxHeight)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	satWindow->SetOriginalSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	// trigger a relayout
	WindowMoved(window);
}


void
StackAndTile::WindowLookChanged(Window* window, window_look look)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	satWindow->WindowLookChanged(look);
}


void
StackAndTile::WindowFeelChanged(Window* window, window_feel feel)
{
	// check if it is still a compatible feel
	if (feel == B_NORMAL_WINDOW_FEEL)
		return;
	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	SATGroup* group = satWindow->GetGroup();
	if (!group)
		return;
	if (group->CountItems() > 1)
		group->RemoveWindow(satWindow, false);
}


bool
StackAndTile::SetDecoratorSettings(Window* window, const BMessage& settings)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return false;

	return satWindow->SetSettings(settings);
}


void
StackAndTile::GetDecoratorSettings(Window* window, BMessage& settings)
{
	SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;

	satWindow->GetSettings(settings);
}


SATWindow*
StackAndTile::GetSATWindow(Window* window)
{
	if (window == NULL)
		return NULL;

	SATWindowMap::const_iterator it = fSATWindowMap.find(
		window);
	if (it != fSATWindowMap.end())
		return it->second;

	// TODO fix race condition with WindowAdded this method is called before
	// WindowAdded and a SATWindow is created twice!
	return NULL;

	// If we don't know this window, memory allocation might has been failed
	// previously. Try to add the window now.
	SATWindow* satWindow = new (std::nothrow)SATWindow(this, window);
	if (satWindow)
		fSATWindowMap[window] = satWindow;

	return satWindow;
}


SATWindow*
StackAndTile::FindSATWindow(uint64 id)
{
	for (SATWindowMap::const_iterator it = fSATWindowMap.begin();
		it != fSATWindowMap.end(); it++) {
		SATWindow* window = it->second;
		if (window->Id() == id)
			return window;
	}
	return NULL;
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

	group->RemoveWindow(fCurrentSATWindow, false);
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

	const WindowAreaList& areas = group->GetAreaList() ;
	for (int32 i = 0; i < areas.CountItems(); i++) {
		WindowArea* currentArea = areas.ItemAt(i);
		if (currentArea == area)
			continue;
		desktop->ActivateWindow(currentArea->TopWindow()->GetWindow());
	}

	desktop->ActivateWindow(satWindow->GetWindow());
}


bool
StackAndTile::_HandleMessage(BPrivate::LinkReceiver& link,
	BPrivate::LinkSender& reply)
{
	int32 what;
	link.Read<int32>(&what);

	switch (what) {
		case BPrivate::kSaveAllGroups:
		{
			BMessage allGroupsArchive;
			GroupIterator groups(this, fDesktop);
			while (true) {
				SATGroup* group = groups.NextGroup();
				if (group == NULL)
					break;
				if (group->CountItems() <= 1)
					continue;
				BMessage groupArchive;
				if (group->ArchiveGroup(groupArchive) != B_OK)
					continue;
				allGroupsArchive.AddMessage("group", &groupArchive);
			}
			int32 size = allGroupsArchive.FlattenedSize();
			char buffer[size];
			if (allGroupsArchive.Flatten(buffer, size) == B_OK) {
				reply.StartMessage(B_OK);
				reply.Attach<int32>(size);
				reply.Attach(buffer, size);
			} else
				reply.StartMessage(B_ERROR);
			reply.Flush();
			break;
		}

		case BPrivate::kRestoreGroup:
		{
			int32 size;
			if (link.Read<int32>(&size) == B_OK) {
				char buffer[size];
				BMessage group;
				if (link.Read(buffer, size) == B_OK
					&& group.Unflatten(buffer) == B_OK) {
					status_t status = SATGroup::RestoreGroup(group, this);
					reply.StartMessage(status);
					reply.Flush();
				}
			}
			break;
		}

		default:
			return false;
	}

	return true;
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
