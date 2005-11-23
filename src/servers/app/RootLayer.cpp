/*
 * Copyright 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Gabe Yoder <gyoder@stny.rr.com>
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adrian Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	Class used for the top layer of each workspace's Layer tree */


#include "Decorator.h"
#include "DrawingEngine.h"
#include "HWInterface.h"
#include "Layer.h"
#include "RootLayer.h"
#include "ServerApp.h"
#include "ServerConfig.h"
#include "ServerProtocol.h"
#include "ServerScreen.h"
#include "ServerWindow.h"
#include "WinBorder.h"
#include "Workspace.h"
#include "WorkspacesLayer.h"

#include <File.h>
#include <List.h>
#include <Message.h>
#include <PortLink.h>
#include <Window.h>

#include <stdio.h>

#if DISPLAY_HAIKU_LOGO
#include "ServerBitmap.h"
#include "HaikuLogo.h"
static const float kGoldenProportion = (sqrtf(5.0) - 1.0) / 2.0;
#endif

//#define DEBUG_ROOTLAYER
#define APPSERVER_ROOTLAYER_SHOW_WORKSPACE_NUMBER

#ifdef DEBUG_ROOTLAYER
	#define STRACE(a) printf a
#else
	#define STRACE(a) /* nothing */
#endif

static RGBColor kDefaultWorkspaceColor = RGBColor(51, 102, 152);
static int32 kMaxWorkspaceCount = 32;

RootLayer::RootLayer(const char *name, int32 workspaceCount,
					 Desktop *desktop, DrawingEngine *driver)
	: Layer(BRect(0, 0, 0, 0), name, 0, B_FOLLOW_ALL, B_WILL_DRAW, driver),

	  fDesktop(desktop),
	  fDragMessage(NULL),
	  fMouseEventLayer(NULL),
	  fSavedEventMask(0),
	  fSavedEventOptions(0),
	  fAllRegionsLock("root layer region lock"),

	  fDirtyForRedraw(),

	  fActiveWksIndex(0),
	  fWsCount(0),
	  fWorkspace(new Workspace*[kMaxWorkspaceCount]),
	  fWorkspacesLayer(NULL),
	  fWMState(),
	  fWinBorderIndex(0),

	  fScreenShotIndex(1)
{
	//NOTE: be careful about this one.
	fRootLayer = this;

// Some stuff that will go away in the future but fills some gaps right now
#if ON_SCREEN_DEBUGGING_INFO
	  fDebugInfo.SetTo("");
#endif

#if DISPLAY_HAIKU_LOGO
	fLogoBitmap = new UtilityBitmap(BRect(0.0, 0.0, kLogoWidth - 1.0,
										  kLogoHeight - 1.0),
									kLogoFormat, 0);
	if (fLogoBitmap->IsValid()) {
		int32 size = min_c(sizeof(kLogoBits), fLogoBitmap->BitsLength());
		memcpy(fLogoBitmap->Bits(), kLogoBits, size);
	} else {
		delete fLogoBitmap;
		fLogoBitmap = NULL;
	}
#endif // DISPLAY_HAIKU_LOGO

	fHidden	= false;

	memset(fWorkspace, 0, sizeof(Workspace*) * kMaxWorkspaceCount);
	SetWorkspaceCount(workspaceCount);

	// TODO: this should eventually be replaced by a method to convert the monitor
	// number to an index in the name, i.e. workspace_settings_1 for screen #1
//	ReadWorkspaceData(WORKSPACE_DATA_LIST);

	// TODO: read these 4 from a configuration file.
//	fScreenWidth = 0;
//	fScreenHeight = 0;
//	fColorSpace = B_RGB32;
//	fFrequency = 60.f;

	// init the first, default workspace
	fWorkspace[fActiveWksIndex] = new Workspace(fActiveWksIndex, 0xFF00FF00,
												kDefaultWorkspaceColor);
	fDrawState->SetHighColor(RGBColor(255, 255, 255));
	fDrawState->SetLowColor(fWorkspace[fActiveWksIndex]->BGColor());

#if ON_SCREEN_DEBUGGING_INFO
	DebugInfoManager::Default()->SetRootLayer(this);
#endif

	fFrame = desktop->VirtualScreen().Frame();

	// RootLayer starts with valid visible regions
	fFullVisible.Set(Bounds());
	fVisible.Set(Bounds());

	// first make sure we are actualy visible
	MarkForRebuild(Bounds());
	MarkForRedraw(Bounds());

	TriggerRebuild();
	TriggerRedraw();
}


RootLayer::~RootLayer()
{
	delete fDragMessage;

	for (int32 i = 0; i < fWsCount; i++)
		delete fWorkspace[i];
	delete[] fWorkspace;

	// RootLayer object just uses Screen objects, it is not allowed to delete them.

#if DISPLAY_HAIKU_LOGO
	delete fLogoBitmap;
#endif
}


void
RootLayer::GoChangeWinBorderFeel(WinBorder *winBorder, int32 newFeel)
{
	Lock();
	change_winBorder_feel(winBorder, newFeel);
	Unlock();
}


void
RootLayer::MoveBy(float x, float y)
{
}


void
RootLayer::ResizeBy(float x, float y)
{
	if (x == 0 && y == 0)
		return;

	// TODO: we should be able to update less here

	//BRect previous = fFrame;

	fFrame.right += x;
	fFrame.bottom += y;

	fFullVisible.Set(Bounds());
	fVisible.Set(Bounds());

	BRegion region(fFrame);
	MarkForRebuild(region);
	TriggerRebuild();

	//region.Exclude(previous);

	MarkForRedraw(region);
	TriggerRedraw();
}


Layer*
RootLayer::FirstChild() const
{
	fWinBorderIndex = 0;
	return static_cast<Layer*>(fWMState.WindowList.ItemAt(fWinBorderIndex++));
}

Layer*
RootLayer::NextChild() const
{
	return static_cast<Layer*>(fWMState.WindowList.ItemAt(fWinBorderIndex++));
}

Layer*
RootLayer::PreviousChild() const
{
	return static_cast<Layer*>(fWMState.WindowList.ItemAt(fWinBorderIndex--));
}

Layer*
RootLayer::LastChild() const
{
	fWinBorderIndex = fWMState.WindowList.CountItems()-1;
	return static_cast<Layer*>(fWMState.WindowList.ItemAt(fWinBorderIndex--));
}


void
RootLayer::AddWinBorder(WinBorder* winBorder)
{
	if (!winBorder->IsHidden())
	{
		CRITICAL("RootLayer::AddWinBorder - winBorder must be hidden\n");
		return;
	}

	// Subset modals also need to have a main window before appearing in workspace list.
	int32 feel = winBorder->Feel();
	if (feel != B_FLOATING_SUBSET_WINDOW_FEEL && feel != B_MODAL_SUBSET_WINDOW_FEEL) {
		uint32 workspaces = winBorder->Workspaces();
		// add to current workspace
		if (workspaces == 0)
			fWorkspace[fActiveWksIndex]->AddWinBorder(winBorder);
		else {
			// add to desired workspaces
			for (int32 i = 0; i < fWsCount; i++) {
				if (fWorkspace[i] && (workspaces & (0x00000001UL << i)))
					fWorkspace[i]->AddWinBorder(winBorder);
			}
		}
	}

	// we _DO_NOT_ need to invalidate here. At this point our WinBorder is hidden!

	// set some internals
	winBorder->SetRootLayer(this);
	winBorder->fParent = this;
}


void
RootLayer::RemoveWinBorder(WinBorder* winBorder)
{
	// Note: removing a subset window is also permited/performed.

	if (!winBorder->IsHidden())
	{
		CRITICAL("RootLayer::RemoveWinBorder - winBorder must be hidden\n");
		return;
	}

	// security measure: in case of windows whose workspace count is 0(current workspace),
	// we don't know the exact workspaces to remove it from. And how all modal and floating
	// windows have 0 as a workspace index, this action is fully justified.
	for (int32 i = 0; i < fWsCount; i++) {
		if (fWorkspace[i])
			fWorkspace[i]->RemoveWinBorder(winBorder);
	}

	// we _DO_NOT_ need to invalidate here. At this point our WinBorder is hidden!

	LayerRemoved(winBorder);

	// set some internals
	winBorder->SetRootLayer(NULL);
	winBorder->fParent = NULL;
}


void
RootLayer::AddSubsetWinBorder(WinBorder *winBorder, WinBorder *toWinBorder)
{
	// SUBSET windows _must_ have their workspaceIndex set to 0x0
	if (winBorder->Workspaces() != 0UL)
	{
		CRITICAL("SUBSET windows _must_ have their workspaceIndex set to 0x0\n");
		return;
	}

	// there is no point in continuing - this subset window won't be shown,
	// from 'toWinBorder's point of view
	if (winBorder->IsHidden() || toWinBorder->IsHidden())
	{
		return;
	}

	bool invalidate	= false;
	bool invalid;
	Workspace::State oldWMState;
	ActiveWorkspace()->GetState(&oldWMState);

	// we try to add WinBorders to all workspaces. If they are not needed, nothing will be done.
	// If they are needed, Workspace automaticaly allocates space and inserts them.
	for (int32 i = 0; i < fWsCount; i++) {
		invalid = false;

		if (fWorkspace[i] && fWorkspace[i]->HasWinBorder(toWinBorder))
			invalid = fWorkspace[i]->ShowWinBorder(winBorder, false);

		if (fActiveWksIndex == i)
			invalidate = invalid;
	}

	if (invalidate)
		RevealNewWMState(oldWMState);
}


void
RootLayer::RemoveSubsetWinBorder(WinBorder *winBorder, WinBorder *fromWinBorder)
{
	// there is no point in continuing - this subset window is not visible
	// at least not visible from 'fromWinBorder's point of view.
	if (winBorder->IsHidden() || fromWinBorder->IsHidden())
		return;

	bool invalidate	= false;
	bool invalid;
	Workspace::State oldWMState;
	ActiveWorkspace()->GetState(&oldWMState);

	// we try to remove from all workspaces. If winBorder is not in there, nothing will be done.
	for (int32 i = 0; i < fWsCount; i++) {
		invalid = false;

		if (fWorkspace[i] && fWorkspace[i]->HasWinBorder(fromWinBorder))
			invalid = fWorkspace[i]->HideWinBorder(winBorder);

		if (fActiveWksIndex == i)
			invalidate = invalid;
	}

	if (invalidate)
		RevealNewWMState(oldWMState);
}

// NOTE: This must be called by RootLayer's thread!!!!
bool RootLayer::SetActiveWorkspace(int32 index)
{
	STRACE(("RootLayer(%s)::SetActiveWorkspace(%ld)\n", Name(), index));

	// nice try!
	if (index >= fWsCount || index == fActiveWksIndex || index < 0)
		return false;

	// you cannot switch workspaces on R5 if there is an event mask BView
//	if (fNotifyLayer && fNotifyLayer->Owner())
//		return false;

	// if you're dragging something you are allowed to change workspaces only if
	// the Layer being dragged is a B_NORMAL_WINDOW_FEEL WinBorder.
	WinBorder *draggedWinBorder = dynamic_cast<WinBorder*>(fMouseEventLayer);
	if (fMouseEventLayer != NULL) {
		if (draggedWinBorder) {
			if (draggedWinBorder->Feel() != B_NORMAL_WINDOW_FEEL)
				return false;
		} else
			return false;
	}

	// if fWorkspace[index] object does not exist, create and add allowed WinBorders
	if (!fWorkspace[index]) {
		// TODO: we NEED datas from a file!!!
		fWorkspace[index] = new Workspace(index, 0xFF00FF00, kDefaultWorkspaceColor);

		// we need to lock the window list here so no other window can be created 
		fDesktop->Lock();

		const BObjectList<WinBorder>& windowList = fDesktop->WindowList();
		int32 windowCount = windowList.CountItems();

		for (int32 i = 0; i < windowCount; i++) {
			WinBorder* winBorder = windowList.ItemAt(i);

			// is WinBorder on this workspace?
			if (winBorder->Workspaces() & (0x00000001UL << index)) {
				fWorkspace[index]->AddWinBorder(winBorder);

				if (!winBorder->IsHidden())
					fWorkspace[index]->ShowWinBorder(winBorder);
			}
		}

		fDesktop->Unlock();
	}
	// adjust our DrawData to workspace colors
	rgb_color bg = fWorkspace[index]->BGColor().GetColor32();
	if ((bg.red + bg.green + bg.blue) / 3 > 128)
		fDrawState->SetHighColor(RGBColor(0, 0, 0));
	else
		fDrawState->SetHighColor(RGBColor(255, 255, 255));
	fDrawState->SetLowColor(fWorkspace[index]->BGColor());

	// save some datas
	int32 exIndex = ActiveWorkspaceIndex();
	Workspace::State oldWMState;
	ActiveWorkspace()->GetState(&oldWMState);

	// send the workspace changed message for the old workspace
	{
		Layer *layer;
		for (int32 i = 0; (layer = static_cast<Layer*>(fWMState.WindowList.ItemAt(i++))); ) {
			layer->WorkspaceActivated(fActiveWksIndex, false);
		}
	}

	fActiveWksIndex	= index;

	if (draggedWinBorder && !ActiveWorkspace()->HasWinBorder(draggedWinBorder)) {
		// Workspace class expects a window to be hidden when it's about to be removed.
		// As we surely know this windows is visible, we simply set fHidden to true and then
		// change it back when adding winBorder to the current workspace.
		draggedWinBorder->fHidden = true;
		fWorkspace[exIndex]->HideWinBorder(draggedWinBorder);
		fWorkspace[exIndex]->RemoveWinBorder(draggedWinBorder);

		draggedWinBorder->fHidden = false;
		ActiveWorkspace()->AddWinBorder(draggedWinBorder);
		ActiveWorkspace()->ShowWinBorder(draggedWinBorder);

		// TODO: can you call SetWinBorderWorskpaces() instead of this?
		uint32 wks = draggedWinBorder->Workspaces();
		BMessage changedMsg(B_WORKSPACES_CHANGED);
		changedMsg.AddInt64("when", real_time_clock_usecs());
		changedMsg.AddInt32("old", wks);
		wks &= ~(0x00000001 << exIndex);
		wks |= (0x00000001 << fActiveWksIndex);
		changedMsg.AddInt32("new", wks);
		draggedWinBorder->QuietlySetWorkspaces(wks);
		draggedWinBorder->Window()->SendMessageToClient(&changedMsg, B_NULL_TOKEN);
	}

	RevealNewWMState(oldWMState);

	// send the workspace changed message for the new workspace
	{
		Layer *layer;
		for (int32 i = 0; (layer = static_cast<Layer*>(fWMState.WindowList.ItemAt(i++))); ) {
			layer->WorkspaceActivated(fActiveWksIndex, true);
		}
	}

	// workspace changed
	return true;
}


void
RootLayer::SetWinBorderWorskpaces(WinBorder *winBorder, uint32 oldIndex, uint32 newIndex)
{
	// if the active notify Layer is somehow related to winBorder, then
	// this window/WinBorder is not allowed to leave this workspace.
// TODO: this looks wrong: I doubt the window is supposed to open in two workspaces then
//	if (fNotifyLayer && (fNotifyLayer == winBorder || fNotifyLayer->Owner() == winBorder))
//		newIndex |= (0x00000001UL << fActiveWksIndex);

	uint32 localOldIndex = oldIndex;
	uint32 localNewIndex = newIndex;
	bool invalidate = false;
	bool invalid;

	// if we're in the current workspace only, reflect that in the workspace index
	if (localOldIndex == 0UL)
		localOldIndex |= (0x00000001UL << fActiveWksIndex);
	if (localNewIndex == 0UL)
		localNewIndex |= (0x00000001UL << fActiveWksIndex);

	// you *cannot* set workspaces index for a window other than a normal one!
	// Note: See ServerWindow class.
	if (winBorder->Feel() != B_NORMAL_WINDOW_FEEL || localOldIndex == localNewIndex)
		return;

	Workspace::State oldWMState;
	ActiveWorkspace()->GetState(&oldWMState);

	for (int32 i = 0; i < 32; i++) {
		if (fWorkspace[i]) {
			invalid = false;

			if (!(localNewIndex & (0x00000001UL << i)) && fWorkspace[i]->HasWinBorder(winBorder)) {
				if (!winBorder->IsHidden()) {
					// a little trick to force Workspace to properly pick the next front.
					winBorder->fHidden = true;
					invalid = fWorkspace[i]->HideWinBorder(winBorder);
					winBorder->fHidden = false;
				}
				fWorkspace[i]->RemoveWinBorder(winBorder);
			}

			if ((localNewIndex & (0x00000001UL << i)) && !fWorkspace[i]->HasWinBorder(winBorder)) {
				fWorkspace[i]->AddWinBorder(winBorder);
				if (!winBorder->IsHidden())
					invalid = fWorkspace[i]->ShowWinBorder(winBorder);
			}

			if (fActiveWksIndex == i)
				invalidate = invalid;
		}
	}

	winBorder->WorkspacesChanged(oldIndex, newIndex);

	if (invalidate)
		RevealNewWMState(oldWMState);
}


void
RootLayer::SetWorkspaceCount(int32 wksCount)
{
	if (wksCount < 1)
		wksCount = 1;
	if (wksCount > kMaxWorkspaceCount)
		wksCount = kMaxWorkspaceCount;

	for (int32 i = wksCount; i < fWsCount; i++) {
		delete fWorkspace[i];
		fWorkspace[i] = NULL;
	}
	
	fWsCount = wksCount;
}


void
RootLayer::ReadWorkspaceData(const char *path)
{
	BMessage msg, settings;
	BFile file(path,B_READ_ONLY);
	char string[20];

	if (file.InitCheck() == B_OK && msg.Unflatten(&file) == B_OK) {
		int32 count;
		
		if (msg.FindInt32("workspace_count", &count)!=B_OK)
			count = 9;

		SetWorkspaceCount(count);

		for (int32 i = 0; i < count; i++) {
			Workspace *ws=(Workspace*)fWorkspace[i];
			if (!ws)
				continue;

			sprintf(string,"workspace %ld",i);

			if (msg.FindMessage(string,&settings) == B_OK) {
				ws->GetSettings(settings);
				settings.MakeEmpty();
			} else
				ws->GetDefaultSettings();
		}
	} else {
		SetWorkspaceCount(9);

		for (int32 i = 0; i < 9; i++) {
			Workspace *ws=(Workspace*)fWorkspace[i];
			if (!ws)
				continue;

			ws->GetDefaultSettings();
		}
	}
}


void
RootLayer::SaveWorkspaceData(const char *path)
{
	BMessage msg,dummy;
	BFile file(path,B_READ_WRITE | B_CREATE_FILE);
	
	if(file.InitCheck()!=B_OK)
	{
		printf("ERROR: Couldn't save workspace data in RootLayer\n");
		return;
	}
	
	char string[20];
	int32 count=fWsCount;

	if(msg.Unflatten(&file)==B_OK)
	{
		// if we were successful in unflattening the file, it means we're
		// going to need to save over the existing data
		for(int32 i=0; i<count; i++)
		{
			sprintf(string,"workspace %ld",i);
			if(msg.FindMessage(string,&dummy)==B_OK)
				msg.RemoveName(string);
		}
	}
		
	for(int32 i=0; i<count; i++)
	{
		sprintf(string,"workspace %ld",i);

		Workspace *ws=(Workspace*)fWorkspace[i];
		
		if(!ws)
		{
			dummy.MakeEmpty();
			ws->PutSettings(&dummy,i);
			msg.AddMessage(string,&dummy);
		}
		else
		{
			// We're not supposed to have this happen, but we'll suck it up, anyway. :P
			Workspace::PutDefaultSettings(&msg,i);
		}
	}
}


void
RootLayer::HideWinBorder(WinBorder* winBorder)
{
	Lock();
	hide_winBorder(winBorder);
	Unlock();
}


void
RootLayer::ShowWinBorder(WinBorder* winBorder)
{
	Lock();
	show_winBorder(winBorder);
	Unlock();
}


void
RootLayer::RevealNewWMState(Workspace::State &oldWMState)
{
	// clean fWMState
	fWMState.Focus = NULL;
	fWMState.Front = NULL;
	fWMState.Active = NULL;
	fWMState.WindowList.MakeEmpty();

	ActiveWorkspace()->GetState(&fWMState);

// BOOKMARK!
// TODO: there can be more than one window active at a time! ex: a normal + floating_app, with
// floating window having focus.
	// send window activation messages
	if (oldWMState.Active != fWMState.Active) {
		if (oldWMState.Active)
			oldWMState.Active->Activated(false);
		if (fWMState.Active) {
			fWMState.Active->Activated(true);
			fDesktop->EventDispatcher().SetFocus(&fWMState.Active->Window()->FocusMessenger());
		} else
			fDesktop->EventDispatcher().SetFocus(NULL);
	}

	// calculate the region that must be invalidated/redrawn

	bool redraw = false;
	// first, if the focus window changed, make sure the decorators reflect this state.
	BRegion dirtyRegion;
	if (oldWMState.Focus != fWMState.Focus) {
		if (oldWMState.Focus) {
			dirtyRegion.Include(&oldWMState.Focus->VisibleRegion());
			redraw = true;			
		}
		if (fWMState.Focus) {
			dirtyRegion.Include(&fWMState.Focus->VisibleRegion());
			redraw = true;			
		}
		if (redraw) {
			MarkForRedraw(dirtyRegion);
		}
	}

	bool invalidate = false;
	// check to see if window count changed over states
	if (oldWMState.WindowList.CountItems() != fWMState.WindowList.CountItems()) {
		invalidate = true;
	} else if (memcmp(oldWMState.WindowList.Items(), fWMState.WindowList.Items(),
			fWMState.WindowList.CountItems()*sizeof(void*)) != 0) {
		invalidate = true;
	}

	if (invalidate) {
		// clear visible areas for windows not visible anymore.
		int32 oldWindowCount = oldWMState.WindowList.CountItems();
		int32 newWindowCount = fWMState.WindowList.CountItems();
		BList oldStrippedList, newStrippedList;
		for (int32 i = 0; i < oldWindowCount; ++i) {
			Layer *layer = static_cast<Layer*>(oldWMState.WindowList.ItemAtFast(i));
			if (!layer)
				continue;

			bool stillPresent = false;
			for (int32 j = 0; j < newWindowCount; ++j)
				if (layer == fWMState.WindowList.ItemAtFast(j)) {
					stillPresent = true;
					break;
				}

			if (!stillPresent) {
				MarkForRebuild(layer->FullVisible());
				MarkForRedraw(layer->FullVisible());

				layer->_ClearVisibleRegions();
			}
			else {
				oldStrippedList.AddItem(layer);
			}
		}

		// for new windows, invalidate(rebuild & redraw) the maximum that it can occupy.
		for (int32 i = 0; i < newWindowCount; ++i) {
			Layer *layer = static_cast<Layer*>(fWMState.WindowList.ItemAtFast(i));
			if (!layer)
				continue;

			bool isNewWindow = true;
			for (int32 j = 0; j < oldWindowCount; ++j)
				if (layer == oldWMState.WindowList.ItemAtFast(j)) {
					isNewWindow = false;
					break;
				}
			if (isNewWindow) {
				BRegion invalid;

				// invalidate the maximum area which this layer/window can occupy.
				layer->GetOnScreenRegion(invalid);

				MarkForRebuild(invalid);
				MarkForRedraw(invalid);
			}
			else {
				newStrippedList.AddItem(layer);
			}
		}

		// if a window came in front ot others, invalidate its previously hidden area.
		oldWindowCount = oldStrippedList.CountItems();
		newWindowCount = newStrippedList.CountItems();
		for (int32 i = 0; i < oldWindowCount; ++i) {
			Layer *layer = static_cast<Layer*>(oldStrippedList.ItemAtFast(i));
			if (!layer)
				continue;
			if (i < newStrippedList.IndexOf(layer)) {
				BRegion invalid;

				// start by invalidating the maximum area which this layer/window can occupy.
				layer->GetOnScreenRegion(invalid);

				// no reason to invalidate what's currently visible.
				invalid.Exclude(&layer->FullVisible());

				MarkForRebuild(invalid);
				MarkForRedraw(invalid);
			}
		}
/*
// for debugging
GetDrawingEngine()->ConstrainClippingRegion(&fDirtyForRedraw);
RGBColor c(rand()%255,rand()%255,rand()%255);
GetDrawingEngine()->FillRect(BRect(0,0,800,600), c);
snooze(2000000);
GetDrawingEngine()->ConstrainClippingRegion(NULL);
*/
		// redraw of focus change is automaticaly done
		redraw = false;
		// trigger region rebuilding and redraw
		TriggerRebuild();
		TriggerRedraw();
	}
	else if (redraw) {
		MarkForRedraw(dirtyRegion);
		TriggerRedraw();
	}
}


bool
RootLayer::SetActive(WinBorder* newActive, bool activate)
{
	bool returnValue = false;
	uint32 workspaceIndex = newActive->Workspaces();
	if (workspaceIndex == 0)
		workspaceIndex = 0x00000001UL << fActiveWksIndex;

	for (int32 i = 0; i < kMaxWorkspaceCount; i++) {
		if (fWorkspace[i] && (workspaceIndex & (0x00000001UL << i))) {
			if (i == fActiveWksIndex) {
				Workspace::State oldWMState;
				ActiveWorkspace()->GetState(&oldWMState);

				if (activate)
					returnValue = ActiveWorkspace()->AttemptToActivate(newActive);
				else
					returnValue = ActiveWorkspace()->AttemptToMoveToBack(newActive);

				RevealNewWMState(oldWMState);		
			}
			else {
				fWorkspace[i]->AttemptToActivate(newActive);
				// NOTE: for multiple monitor support, if a workspaces is mapped to
				// a Screen, we must invalidate/redraw to see the change.
			}
		}
	}

	// If this WinBorder does not appear in current workspace,
	// change to the first one who does.	
	if (activate && !(workspaceIndex & (0x00000001UL << fActiveWksIndex))) {
		// find an workspace index in which this Layer appears
		for (int32 i = 0; i < kMaxWorkspaceCount; i++) {
			if (workspaceIndex & (0x00000001UL << i)) {
				SetActiveWorkspace(i);
				returnValue = Active() == newActive;
				break;
			}
		}
	}

	return returnValue;
}


Layer*
RootLayer::_ChildAt(BPoint where)
{
	if (VisibleRegion().Contains(where))
		return NULL;

	for (Layer* child = LastChild(); child; child = PreviousChild()) {
		if (child->FullVisible().Contains(where))
			return child;
	}

	return NULL;
}


//	#pragma mark - Input related methods


void
RootLayer::MouseEventHandler(BMessage *event)
{
	BPoint where;
	if (event->FindPoint("where", &where) != B_OK)
		return;

	Layer* layer = fMouseEventLayer;
	if (layer == NULL) {
		layer = _ChildAt(where);
		if (layer == NULL)
			return;
	}

	switch (event->what) {
		case B_MOUSE_DOWN:
			layer->MouseDown(event, where);
			break;

		case B_MOUSE_UP:
			layer->MouseUp(event, where);
			SetMouseEventLayer(NULL);
			break;

		case B_MOUSE_MOVED:
			layer->MouseMoved(event, where);
			break;
	}
}


void
RootLayer::SetMouseEventLayer(Layer* layer)
{
	fMouseEventLayer = layer;
}


void
RootLayer::LayerRemoved(Layer* layer)
{
	if (fMouseEventLayer == layer)
		fMouseEventLayer = NULL;
}


void
RootLayer::SetDragMessage(BMessage* msg)
{
	if (fDragMessage) {
		delete fDragMessage;
		fDragMessage = NULL;
	}

	if (msg)
		fDragMessage = new BMessage(*msg);
}


BMessage *
RootLayer::DragMessage(void) const
{
	return fDragMessage;
}


// PRIVATE methods

void
RootLayer::show_winBorder(WinBorder *winBorder)
{
	bool invalidate = false;
	bool invalid;
	Workspace::State oldWMState;
	ActiveWorkspace()->GetState(&oldWMState);

	winBorder->Show(false);

	for (int32 i = 0; i < fWsCount; i++) {
		invalid = false;

		if (fWorkspace[i]
			&& (fWorkspace[i]->HasWinBorder(winBorder)
				|| winBorder->Feel() == B_MODAL_SUBSET_WINDOW_FEEL
					// subset modals are a bit like floating windows, they are being added
					// and removed from workspace when there's at least a normal window
					// that uses them.
				|| winBorder->Level() == B_FLOATING_APP))
					// floating windows are inserted/removed on-the-fly so this window,
					// although needed may not be in workspace's list.
		{
			invalid = fWorkspace[i]->ShowWinBorder(winBorder);

			// ToDo: this won't work with FFM
			fWorkspace[i]->AttemptToSetFocus(winBorder);
		}

		if (fActiveWksIndex == i) {
			invalidate = invalid;

			if (dynamic_cast<class WorkspacesLayer *>(winBorder->FirstChild()) != NULL)
				SetWorkspacesLayer(winBorder->FirstChild());
		}
	}

	if (invalidate)
		RevealNewWMState(oldWMState);
}


void
RootLayer::hide_winBorder(WinBorder *winBorder)
{
	bool invalidate = false;
	bool invalid;
	Workspace::State oldWMState;
	ActiveWorkspace()->GetState(&oldWMState);

	winBorder->Hide(false);

	for (int32 i = 0; i < fWsCount; i++) {
		invalid = false;

		if (fWorkspace[i] && fWorkspace[i]->HasWinBorder(winBorder))
			invalid = fWorkspace[i]->HideWinBorder(winBorder);

		if (fActiveWksIndex == i) {
			invalidate = invalid;

			if (dynamic_cast<class WorkspacesLayer *>(winBorder->FirstChild()) != NULL)
				SetWorkspacesLayer(NULL);
		}
	}

	if (invalidate)
		RevealNewWMState(oldWMState);
}


void
RootLayer::change_winBorder_feel(WinBorder *winBorder, int32 newFeel)
{
	bool isVisible = false;
	bool isVisibleInActiveWorkspace = false;

	Workspace::State oldWMState;
	ActiveWorkspace()->GetState(&oldWMState);

	if (!winBorder->IsHidden()) {
		isVisible = true;
		isVisibleInActiveWorkspace = ActiveWorkspace()->HasWinBorder(winBorder);
		// just hide, don't invalidate
		winBorder->Hide(false);
		// all workspaces must be up-to-date with this change of feel.
		for (int32 i = 0; i < kMaxWorkspaceCount; i++) {
			if (fWorkspace[i]) {
				fWorkspace[i]->HideWinBorder(winBorder);
				fWorkspace[i]->RemoveWinBorder(winBorder);
			}
		}
	}

	fDesktop->SetWinBorderFeel(winBorder, newFeel);

	if (isVisible) {
		// just show, don't invalidate
		winBorder->Show(false);
		// all workspaces must be up-to-date with this change of feel.
		for (int32 i = 0; i < kMaxWorkspaceCount; i++) {
			if (fWorkspace[i]) {
				fWorkspace[i]->AddWinBorder(winBorder);
				fWorkspace[i]->ShowWinBorder(winBorder);
			}
		}

		if (isVisibleInActiveWorkspace)
			RevealNewWMState(oldWMState);
	}
}

void
RootLayer::Draw(const BRect &r)
{
	fDriver->FillRect(r, fWorkspace[fActiveWksIndex]->BGColor());
#ifdef APPSERVER_ROOTLAYER_SHOW_WORKSPACE_NUMBER
	char	string[30];
	sprintf(string, "Workspace %ld", fActiveWksIndex + 1);
	fDriver->DrawString(string, strlen(string), BPoint(5, 15),
						fDrawState);
#endif // APPSERVER_ROOTLAYER_SHOW_WORKSPACE_NUMBER
#if ON_SCREEN_DEBUGGING_INFO
	BPoint location(5, 40);
	float textHeight = 15.0; // be lazy
	const char* p = fDebugInfo.String();
	const char* start = p;
	while (*p) {
		p++;
		if (*p == 0 || *p == '\n') {
			fDriver->DrawString(start, p - start, location,
								fDrawState);
			// NOTE: Eventually, this will be below the screen!
			location.y += textHeight;
			start = p + 1;
		}
	}
#endif // ON_SCREEN_DEBUGGING_INFO

#if DISPLAY_HAIKU_LOGO
	if (fLogoBitmap) {
		BPoint logoPos;
		logoPos.x = floorf(min_c(fFrame.left + fFrame.Width() * kGoldenProportion,
								 fFrame.right - (kLogoWidth + kLogoHeight / 2.0)));
		logoPos.y = floorf(fFrame.bottom - kLogoHeight * 1.5);
		BRect bitmapBounds = fLogoBitmap->Bounds();
		fDriver->DrawBitmap(fLogoBitmap, bitmapBounds,
							bitmapBounds.OffsetToCopy(logoPos), fDrawState);
	}
#endif // DISPLAY_HAIKU_LOGO
}

#if ON_SCREEN_DEBUGGING_INFO
void
RootLayer::AddDebugInfo(const char* string)
{
	if (Lock()) {
		fDebugInfo << string;
		MarkForRedraw(VisibleRegion());
		TriggerRedraw();
		Unlock();
	}
}
#endif // ON_SCREEN_DEBUGGING_INFO


void
RootLayer::MarkForRedraw(const BRegion &dirty)
{
	BAutolock locker(fAllRegionsLock);

	fDirtyForRedraw.Include(&dirty);
}

void
RootLayer::TriggerRedraw()
{
	BAutolock locker(fAllRegionsLock);
/*
// for debugging
GetDrawingEngine()->ConstrainClippingRegion(&fDirtyForRedraw);
RGBColor c(rand()%255,rand()%255,rand()%255);
GetDrawingEngine()->FillRect(BRect(0,0,800,600), c);
snooze(2000000);
GetDrawingEngine()->ConstrainClippingRegion(NULL);
*/
	_AllRedraw(fDirtyForRedraw);

	fDirtyForRedraw.MakeEmpty();
}

