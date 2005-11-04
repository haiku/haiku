/*
 * Copyright 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Gabe Yoder <gyoder@stny.rr.com>
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adrian Oanca <adioanca@cotty.iren.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	Class used for the top layer of each workspace's Layer tree */


#include <stdio.h>

#include <Window.h>
#include <List.h>
#include <Message.h>
#include <Entry.h>
#include <File.h>
#include <PortLink.h>

#include "Decorator.h"
#include "DisplayDriverPainter.h"
#include "Globals.h"
#include "HWInterface.h"
#include "Layer.h"
#include "ServerApp.h"
#include "ServerConfig.h"
#include "ServerProtocol.h"
#include "ServerScreen.h"
#include "ServerWindow.h"
#include "WinBorder.h"
#include "Workspace.h"
#include "WorkspacesLayer.h"

#include "RootLayer.h"

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
	  fLastLayerUnderMouse(this),
	  fNotifyLayer(NULL),
	  fSavedEventMask(0),
	  fSavedEventOptions(0),
	  fAllRegionsLock("root layer region lock"),

	  fThreadID(B_ERROR),
	  fListenPort(-1),

	  fButtons(0),
	  fLastMousePosition(0.0, 0.0),

	  fActiveWksIndex(0),
	  fWsCount(0),
	  fWorkspace(new Workspace*[kMaxWorkspaceCount]),
	  fWorkspacesLayer(NULL),
	  fWMState(),
	  fWinBorderIndex(0),

	  fScreenShotIndex(1),
	  fQuiting(false)
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

	// Spawn our working thread
	fThreadID = spawn_thread(WorkingThread, name, B_DISPLAY_PRIORITY, this);

	fListenPort = find_port(SERVER_INPUT_PORT);
	if (fListenPort == B_NAME_NOT_FOUND) {
		fListenPort = -1;
	}
#if ON_SCREEN_DEBUGGING_INFO
	DebugInfoManager::Default()->SetRootLayer(this);
#endif

	fFrame = desktop->VirtualScreen().Frame();
}


RootLayer::~RootLayer()
{
	fQuiting = true;

	BMessage quitMsg(B_QUIT_REQUESTED);
	ssize_t length = quitMsg.FlattenedSize();
	char buffer[length];
	if (quitMsg.Flatten(buffer,length) < B_OK) {
		// failed to flatten?
		kill_thread(fThreadID);
	}
	else{
		write_port(fListenPort, 0, buffer, length);

		status_t dummy;
		wait_for_thread(fThreadID, &dummy);
	}


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
RootLayer::RunThread()
{
	if (fThreadID > 0)
		resume_thread(fThreadID);
	else
		CRITICAL("Can not create any more threads.\n");
}

/*!
	\brief Thread function for handling input messages and calculating visible regions.
	\param data Pointer to the app_server to which the thread belongs
	\return Throwaway value - always 0
*/
int32
RootLayer::WorkingThread(void *data)
{
	RootLayer *oneRootLayer = (RootLayer*)data;

	// first make sure we are actualy visible
	oneRootLayer->Lock();
#ifndef NEW_CLIPPING
	oneRootLayer->RebuildFullRegion();
	oneRootLayer->GoInvalidate(oneRootLayer, oneRootLayer->Bounds());
#else
	oneRootLayer->rebuild_visible_regions(
		BRegion(oneRootLayer->Bounds()),
		BRegion(oneRootLayer->Bounds()),
		oneRootLayer->LastChild());

	oneRootLayer->fRedrawReg.Include(oneRootLayer->Bounds());
	oneRootLayer->RequestDraw(oneRootLayer->fRedrawReg, oneRootLayer->LastChild());
#endif
	oneRootLayer->Unlock();
	
	STRACE(("info: RootLayer(%s)::WorkingThread listening on port %ld.\n", oneRootLayer->Name(), oneRootLayer->fListenPort));
	while(!oneRootLayer->fQuiting) {

		BMessage *msg = oneRootLayer->ReadMessageFromPort(B_INFINITE_TIMEOUT);

		if (msg)
			oneRootLayer->fQueue.AddMessage(msg);

		int32 msgCount = port_count(oneRootLayer->fListenPort);
		for (int32 i = 0; i < msgCount; ++i) {
			msg = oneRootLayer->ReadMessageFromPort(0);
			if (msg)
				oneRootLayer->fQueue.AddMessage(msg);
		}

		//	loop as long as there are messages in the queue and the port is empty.
		bool dispatchNextMessage = true;
		while(dispatchNextMessage && !oneRootLayer->fQuiting) {
			BMessage *currentMessage = oneRootLayer->fQueue.NextMessage();
			
			if (!currentMessage)
				// no more messages
				dispatchNextMessage = false;
			else {

				oneRootLayer->Lock();

				switch (currentMessage->what) {
					// We don't need to do anything with these two, so just pass them
					// onto the active application. Eventually, we will end up passing 
					// them onto the window which is currently under the cursor.
					case B_MOUSE_DOWN:
					case B_MOUSE_UP:
					case B_MOUSE_MOVED:
					case B_MOUSE_WHEEL_CHANGED:
						oneRootLayer->MouseEventHandler(currentMessage);
					break;

					case B_KEY_DOWN:
					case B_KEY_UP:
					case B_UNMAPPED_KEY_DOWN:
					case B_UNMAPPED_KEY_UP:
					case B_MODIFIERS_CHANGED:
						oneRootLayer->KeyboardEventHandler(currentMessage);
					break;

					case B_QUIT_REQUESTED:
						exit_thread(0);
					break;

					default:
						printf("RootLayer(%s)::WorkingThread received unexpected code %lx\n", oneRootLayer->Name(), msg->what);
					break;
				}

				oneRootLayer->Unlock();

				delete currentMessage;

				//	Are any messages on the port?
				if (port_count(oneRootLayer->fListenPort) > 0)
					dispatchNextMessage = false;
			}
		}
	}
	return 0;
}


void
RootLayer::GoInvalidate(Layer *layer, const BRegion &region)
{
	BRegion invalidRegion(region);

	Lock();
#ifndef NEW_CLIPPING
	if (layer->fParent)
		layer = layer->fParent;

	layer->FullInvalidate(invalidRegion);
#else
	layer->do_Invalidate(invalidRegion);
#endif
	Unlock();
}

void
RootLayer::GoRedraw(Layer *layer, const BRegion &region)
{
	BRegion redrawRegion(region);
	
	Lock();
#ifndef NEW_CLIPPING
	layer->Invalidate(redrawRegion);
#else
	layer->do_Redraw(redrawRegion);
#endif
	Unlock();
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
	// TODO: implement
}


Layer*
RootLayer::FirstChild() const
{
	fWinBorderIndex = fWMState.WindowList.CountItems()-1;
	return static_cast<Layer*>(fWMState.WindowList.ItemAt(fWinBorderIndex--));
}

Layer*
RootLayer::NextChild() const
{
	return static_cast<Layer*>(fWMState.WindowList.ItemAt(fWinBorderIndex--));
}

Layer*
RootLayer::PreviousChild() const
{
	return static_cast<Layer*>(fWMState.WindowList.ItemAt(fWinBorderIndex++));
}

Layer*
RootLayer::LastChild() const
{
	fWinBorderIndex = 0;
	return static_cast<Layer*>(fWMState.WindowList.ItemAt(fWinBorderIndex++));
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
	if (feel != B_FLOATING_SUBSET_WINDOW_FEEL && feel != B_MODAL_SUBSET_WINDOW_FEEL)
	{
		uint32		wks = winBorder->Workspaces();
		// add to current workspace
		if (wks == 0)
		{
			fWorkspace[fActiveWksIndex]->AddWinBorder(winBorder);
		}
		// add to desired workspaces
		else
		{
			for (int32 i = 0; i < fWsCount; i++)
			{
				if (fWorkspace[i] && (wks & (0x00000001UL << i)))
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
	if (fNotifyLayer && fNotifyLayer->Owner())
		return false;

	// if you're dragging something you are allowed to change workspaces only if
	// the Layer being dragged is a B_NORMAL_WINDOW_FEEL WinBorder.
	WinBorder *draggedWinBorder = dynamic_cast<WinBorder*>(fNotifyLayer);
	if (fNotifyLayer) {
		if (draggedWinBorder) {
			if (draggedWinBorder->Feel() != B_NORMAL_WINDOW_FEEL)
				return false;
		}
		else
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
		draggedWinBorder->Window()->SendMessageToClient(&changedMsg, B_NULL_TOKEN, false);
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
	if (fNotifyLayer && (fNotifyLayer == winBorder || fNotifyLayer->Owner() == winBorder)) {
		newIndex |= (0x00000001UL << fActiveWksIndex);
	}

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
		if (fWMState.Active)
			fWMState.Active->Activated(true);
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
#ifndef NEW_CLIPPING
			fRedrawReg.Include(&dirtyRegion);
#else
// TODO: code for new clipping engine!
#endif
		}
	}

	bool invalidate = false;
	// check to see if window count changed over states
	if (oldWMState.WindowList.CountItems() != fWMState.WindowList.CountItems()) {
		invalidate = true;
	}
	else if (memcmp(oldWMState.WindowList.Items(), fWMState.WindowList.Items(),
			fWMState.WindowList.CountItems()*sizeof(void*)) != 0) {
		invalidate = true;
	}

	if (invalidate) {
		// clear visible areas for windows not visible anymore.
		int32 oldWindowCount = oldWMState.WindowList.CountItems();
		int32 newWindowCount = fWMState.WindowList.CountItems();
		bool stillPresent;
		Layer *layer;
		for (int32 i = 0; i < oldWindowCount; i++)
		{
			layer = static_cast<Layer*>(oldWMState.WindowList.ItemAtFast(i));
			stillPresent = false;
			for (int32 j = 0; j < newWindowCount; j++)
				if (layer == fWMState.WindowList.ItemAtFast(j))
					stillPresent = true;

			if (!stillPresent && layer)
#ifndef NEW_CLIPPING
				empty_visible_regions(layer);
#else
				layer->clear_visible_regions();
#endif
		}
		// redraw of focus change is automaticaly done
		redraw = false;
		// trigger region rebuilding and redraw
#ifndef NEW_CLIPPING
		GoInvalidate(this, fFull);
#else
		do_Invalidate(Bounds());
#endif
	}
	else if (redraw) {
#ifndef NEW_CLIPPING
		GoInvalidate(this, dirtyRegion);
#else
		do_Redraw(dirtyRegion);
#endif
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

//---------------------------------------------------------------------------
//				Workspace related methods
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//				Input related methods
//---------------------------------------------------------------------------
void
RootLayer::_ProcessMouseMovedEvent(BMessage *msg)
{
	BPoint where(0,0);
	msg->FindPoint("where", &where);

	Layer* target = LayerAt(where);
	if (target == NULL) {
		CRITICAL("RootLayer::_ProcessMouseMovedEvent() 'target' can't be null.\n");
		return;
	}

	// change focus in FFM mode
	WinBorder* winBorderTarget = dynamic_cast<WinBorder*>(target);
	if (winBorderTarget) {
		DesktopSettings desktopSettings(fDesktop);
		// TODO: Focus should be a RootLayer option/feature, NOT a Workspace one!!!
		WinBorder* exFocus = Focus();
		if (desktopSettings.MouseMode() != B_NORMAL_MOUSE && exFocus != winBorderTarget) {
			ActiveWorkspace()->AttemptToSetFocus(winBorderTarget);
			// Workspace::SetFocus() *attempts* to set a new focus WinBorder, it may not succeed
			if (exFocus != Focus()) {
				// TODO: invalidate border area and send message to client for the widgets to light up
				// What message? Is there a message on Focus change?
			}
		}
	}

	// add the layer under mouse to the list of layers to be notified on mouse moved
	bool alreadyPresent = fMouseNotificationList.HasItem(target);
	if (!alreadyPresent)
		fMouseNotificationList.AddItem(target);

	int32 count = fMouseNotificationList.CountItems();
	int32 viewAction;
	Layer *lay;
	for (int32 i = 0; i <= count; i++) {
		lay = static_cast<Layer*>(fMouseNotificationList.ItemAt(i));
		if (lay) {
			// set transit state
			if (lay == target)
				if (lay == fLastLayerUnderMouse)
					viewAction = B_INSIDE_VIEW;
				else
					viewAction = B_ENTERED_VIEW;
			else
				if (lay == fLastLayerUnderMouse)
					viewAction = B_EXITED_VIEW;
				else
					viewAction = B_OUTSIDE_VIEW;

			msg->AddInt32("transit", viewAction);
			lay->MouseMoved(msg);
		}
	}

	if (!alreadyPresent)
		fMouseNotificationList.RemoveItem(target);

	fLastLayerUnderMouse = target;
	fLastMousePosition = where;
}

void
RootLayer::MouseEventHandler(BMessage *msg)
{
	switch (msg->what) {
		case B_MOUSE_DOWN: {
			//printf("RootLayer::MouseEventHandler(B_MOUSE_DOWN)\n");

			BPoint where(0,0);

			msg->FindPoint("where", &where);
			// We'll need this so that GetMouse can query for which buttons are down.
			msg->FindInt32("buttons", &fButtons);

			where.ConstrainTo(Frame());

			if (fLastMousePosition != where) {
				// move cursor on screen
				GetHWInterface()->MoveCursorTo(where.x, where.y);

				// If this happens, it's input server's fault.
				// There might be additional fields an application expects in B_MOUSE_MOVED
				// message, and in this way it won't get them. 
				int64 when = 0;
				int32 buttons = 0;

				msg->FindInt64("when", &when);
				msg->FindInt32("buttons", &buttons);

				BMessage mouseMovedMsg(B_MOUSE_MOVED);
				mouseMovedMsg.AddInt64("when", when);
				mouseMovedMsg.AddInt32("buttons", buttons);
				mouseMovedMsg.AddPoint("where", where);

				_ProcessMouseMovedEvent(msg);
			}

			if (fLastLayerUnderMouse == NULL) {
				CRITICAL("RootLayer::MouseEventHandler(B_MOUSE_DOWN) fLastLayerUnderMouse is null!\n");
				break;
			}

			if (fLastLayerUnderMouse == this)
				break;

			int32 count = fMouseNotificationList.CountItems();
			Layer *lay;
			for (int32 i = 0; i <= count; i++) {
				lay = static_cast<Layer*>(fMouseNotificationList.ItemAt(i));
				if (lay)
					// NOTE: testing under R5 shows that it doesn't matter if a window is created 
					// with B_ASYNCHRONOUS_CONTROLS flag or not. B_MOUSE_DOWN is always transmited.
					lay->MouseDown(msg);
			}

			// get the pointer for one of the first RootLayer's descendants
			Layer *primaryTarget = LayerAt(where, false);
			primaryTarget->MouseDown(msg);

			break;
		}
		case B_MOUSE_UP: {
			//printf("RootLayer::MouseEventHandler(B_MOUSE_UP)\n");

			BPoint where(0,0);

			msg->FindPoint("where", &where);

			where.ConstrainTo(fFrame);

			if (fLastMousePosition != where) {
				// move cursor on screen
				GetHWInterface()->MoveCursorTo(where.x, where.y);

				// If this happens, it's input server's fault.
				// There might be additional fields an application expects in B_MOUSE_MOVED
				// message, and in this way it won't get them. 
				int64 when = 0;
				int32 buttons = 0;

				msg->FindInt64("when", &when);
				msg->FindInt32("buttons", &buttons);

				BMessage mouseMovedMsg(B_MOUSE_MOVED);
				mouseMovedMsg.AddInt64("when", when);
				mouseMovedMsg.AddInt32("buttons", buttons);
				mouseMovedMsg.AddPoint("where", where);

				_ProcessMouseMovedEvent(msg);
			}

			if (fLastLayerUnderMouse == NULL) {
				CRITICAL("RootLayer::MouseEventHandler(B_MOUSE_UP) fLastLayerUnderMouse is null!\n");
				break;
			}

			bool foundCurrent = fMouseNotificationList.HasItem(fLastLayerUnderMouse);
			int32 count = fMouseNotificationList.CountItems();
			Layer *lay;
			for (int32 i = 0; i <= count; i++) {
				lay = static_cast<Layer*>(fMouseNotificationList.ItemAt(i));
				if (lay)
					lay->MouseUp(msg);
			}
			ClearNotifyLayer();

			if (!foundCurrent)
				fLastLayerUnderMouse->MouseUp(msg);

			// TODO: This is a quick fix to avoid the endless loop with windows created
			// with the B_ASYNCHRONOUS_CONTROLS flag, but please someone have a look into this.
			fButtons = 0;
			
			break;
		}
		case B_MOUSE_MOVED: {
			//printf("RootLayer::MouseEventHandler(B_MOUSE_MOVED)\n");

			BPoint where(0,0);

			msg->FindPoint("where", &where);

			where.ConstrainTo(fFrame);
			// move cursor on screen
			GetHWInterface()->MoveCursorTo(where.x, where.y);

			_ProcessMouseMovedEvent(msg);

			break;
		}
		case B_MOUSE_WHEEL_CHANGED: {
			//printf("RootLayer::MouseEventHandler(B_MOUSE_WHEEL_CHANGED)\n");
			// FEATURE: This is a tentative change: mouse wheel messages are always sent to the window
			// under the cursor. It's pretty stupid to send it to the active window unless a particular
			// view has locked focus via SetMouseEventMask
						
			if (fLastLayerUnderMouse == NULL) {
				CRITICAL("RootLayer::MouseEventHandler(B_MOUSE_DOWN) fLastLayerUnderMouse is null!\n");
				break;
			}

			fLastLayerUnderMouse->MouseWheelChanged(msg);
			break;
		}
		default:
		{
			printf("RootLayer::MouseEventHandler(): WARNING: unknown message\n");
			break;
		}
	}
}


void
RootLayer::KeyboardEventHandler(BMessage *msg)
{
	switch (msg->what) {
		case B_KEY_DOWN:
		{
			int32 scancode = 0;
			int32 modifiers = 0;
			
			msg->FindInt32("key", &scancode);
			msg->FindInt32("modifiers", &modifiers);

			// F1-F12		
			if (scancode > 0x01 && scancode < 0x0e) {
				// Check for workspace change or safe video mode
#if !TEST_MODE
				if (scancode == 0x0d && (modifiers & (B_LEFT_COMMAND_KEY
											| B_LEFT_CONTROL_KEY | B_LEFT_SHIFT_KEY)) != 0)
#else
				if (scancode == 0x0d && (modifiers & (B_LEFT_CONTROL_KEY
											| B_LEFT_SHIFT_KEY | B_LEFT_OPTION_KEY)) != 0)
#endif
				{
					// TODO: Set to Safe Mode in KeyboardEventHandler:B_KEY_DOWN. (DrawingEngine API change)
					STRACE(("Safe Video Mode invoked - code unimplemented\n"));
					break;
				}

#if !TEST_MODE
				if (modifiers & B_CONTROL_KEY)
#else
				if (modifiers & (B_LEFT_SHIFT_KEY | B_LEFT_CONTROL_KEY))
#endif
				{
					STRACE(("Set Workspace %ld\n",scancode-1));
					SetActiveWorkspace(scancode - 2);
				#ifdef APPSERVER_ROOTLAYER_SHOW_WORKSPACE_NUMBER
					// to draw the current Workspace index on screen.
#ifndef NEW_CLIPPING
					BRegion	reg(fVisible);
					fDriver->ConstrainClippingRegion(&reg);
					Draw(reg.Frame());
					fDriver->ConstrainClippingRegion(NULL);
#endif
				#endif
					break;
				}	
			}

			// Tab key
#if !TEST_MODE
			if (scancode == 0x26 && (modifiers & B_CONTROL_KEY))
#else
			if (scancode == 0x26 && (modifiers & B_SHIFT_KEY))
#endif
			{
				STRACE(("Twitcher\n"));
				//ServerApp *deskbar = app_server->FindApp("application/x-vnd.Be-TSKB");
				//if(deskbar)
				//{
			// TODO: implement;
					printf("Send Twitcher message key to Deskbar - unimplmemented\n");
					break;
				//}
			}

#if !TEST_MODE
			// PrintScreen
			if (scancode == 0xe)
#else
			// Pause/Break
			if (scancode == 0x7f)
#endif
			{
				if (GetDrawingEngine()) {
					char filename[128];
					BEntry entry;

					int32 index = 1;
					do {
						sprintf(filename, "/boot/home/screen%ld.png", index++);
						entry.SetTo(filename);
					} while(entry.Exists());

					GetDrawingEngine()->DumpToFile(filename);

					break;
				}
			}

			// We got this far, so apparently it's safe to pass to the active
			// window.

			if (Focus())
				Focus()->KeyDown(msg);

			break;
		}

		case B_KEY_UP:
		{
			int32 scancode = 0;
			int32 modifiers = 0;
			
			msg->FindInt32("key", &scancode);
			msg->FindInt32("modifiers", &modifiers);

#if !TEST_MODE
			// Tab key
			if(scancode==0x26 && (modifiers & B_CONTROL_KEY))
			{
				//ServerApp *deskbar=app_server->FindApp("application/x-vnd.Be-TSKB");
				//if(deskbar)
				//{
					printf("Send Twitcher message key to Deskbar - unimplmemented\n");
					break;
				//}
			}
#else	// TEST_MODE
			if(scancode==0x26 && (modifiers & B_LEFT_SHIFT_KEY))
			{
				//ServerApp *deskbar=app_server->FindApp("application/x-vnd.Be-TSKB");
				//if(deskbar)
				//{
					printf("Send Twitcher message key to Deskbar - unimplmemented\n");
					break;
				//}
			}
#endif

			// We got this far, so apparently it's safe to pass to the active
			// window.

			if (Focus())
				Focus()->KeyUp(msg);

			break;
		}

		case B_UNMAPPED_KEY_DOWN:
		{
			if(Focus())
				Focus()->UnmappedKeyDown(msg);
			
			break;
		}
		case B_UNMAPPED_KEY_UP:
		{
			if(Focus())
				Focus()->UnmappedKeyUp(msg);
			
			break;
		}
		case B_MODIFIERS_CHANGED:
		{
			if(Focus())
				Focus()->ModifiersChanged(msg);

			break;
		}
		default:
			break;
	}
}

bool
RootLayer::AddToInputNotificationLists(Layer *lay, uint32 mask, uint32 options)
{
	if (!lay)
		return false;

	bool returnValue = true;

	Lock();

	if (mask & B_POINTER_EVENTS) {
		if (!fMouseNotificationList.HasItem(lay))
			fMouseNotificationList.AddItem(lay);
	}
	else {
		if (options == 0)
			fMouseNotificationList.RemoveItem(lay);
	}

	if (mask & B_KEYBOARD_EVENTS) {
		if (!fKeyboardNotificationList.HasItem(lay))
			fKeyboardNotificationList.AddItem(lay);
	}
	else {
		if (options == 0)
			fKeyboardNotificationList.RemoveItem(lay);
	}

	// TODO: set options!!!
	// B_NO_POINTER_HISTORY only! How? By telling to the input_server?
	
	Unlock();

	return returnValue;
}

bool
RootLayer::SetNotifyLayer(Layer *lay, uint32 mask, uint32 options)
{
	if (!lay)
		return false;

	bool returnValue = true;

	Lock();

	fNotifyLayer = lay;
	fSavedEventMask = lay->EventMask();
	fSavedEventOptions = lay->EventOptions();

	AddToInputNotificationLists(lay, mask, options);

	// TODO: set other options!!!
	// B_SUSPEND_VIEW_FOCUS
	// B_LOCK_WINDOW_FOCUS
	Unlock();

	return returnValue;
}

void
RootLayer::ClearNotifyLayer()
{
	if (fNotifyLayer) {
		Lock();
		// remove from notification list.
		AddToInputNotificationLists(fNotifyLayer, 0UL, 0UL);
		// set event masks
		fNotifyLayer->QuietlySetEventMask(fSavedEventMask);
		fNotifyLayer->QuietlySetEventOptions(fSavedEventOptions);
		// add to notification list with event masks set my BView::SetEventMask()
		AddToInputNotificationLists(fNotifyLayer, fSavedEventMask, fSavedEventOptions);

		fNotifyLayer = NULL;
		fSavedEventMask = 0UL;
		fSavedEventOptions = 0UL;

		Unlock();
	}
}

// LayerRemoved
void
RootLayer::LayerRemoved(Layer* layer)
{
	if (layer == fNotifyLayer)
		fNotifyLayer = NULL;

	if (layer == fLastLayerUnderMouse)
		fLastLayerUnderMouse = NULL;
}


void
RootLayer::SetDragMessage(BMessage* msg)
{
	if (fDragMessage) {
		delete fDragMessage;
		fDragMessage = NULL;
	}

	if (msg)
		fDragMessage	= new BMessage(*msg);
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

		if (fWorkspace[i] &&
				(fWorkspace[i]->HasWinBorder(winBorder) ||
				// subset modals are a bit like floating windows, they are being added
				// and removed from workspace when there's at least a normal window
				// that uses them.
				winBorder->Feel() == B_MODAL_SUBSET_WINDOW_FEEL ||
				// floating windows are inserted/removed on-the-fly so this window,
				// although needed may not be in workspace's list.
				winBorder->Level() == B_FLOATING_APP))
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
	// if the notify Layer is somehow related to winBorder, then
	// this window/WinBorder is not allowed to change feel.
	if (fNotifyLayer && (fNotifyLayer == winBorder || fNotifyLayer->Owner() == winBorder)) {
		return;
	}

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

#ifndef NEW_CLIPPING
void
RootLayer::empty_visible_regions(Layer *layer)
{
// TODO: optimize by avoiding recursion?
	// NOTE: first 'layer' must be a WinBorder
	layer->fFullVisible.MakeEmpty();
	layer->fVisible.MakeEmpty();

	Layer* child = layer->LastChild();
	while (child) {
		empty_visible_regions(child);
		child = layer->PreviousChild();
	}
}
#endif

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
		GoRedraw(this, BRegion(fFrame));
		Unlock();
	}
}
#endif // ON_SCREEN_DEBUGGING_INFO


// taken from BLooper
void *
RootLayer::ReadRawFromPort(int32 *msgCode, bigtime_t timeout)
{
	int8 *msgBuffer = NULL;
	ssize_t bufferSize;

	do {
		bufferSize = port_buffer_size_etc(fListenPort, B_RELATIVE_TIMEOUT, timeout);
	} while (bufferSize == B_INTERRUPTED);

	if (bufferSize < B_OK)
		return NULL;

	if (bufferSize > 0)
		msgBuffer = new int8[bufferSize];

	// we don't want to wait again here, since that can only mean
	// that someone else has read our message and our bufferSize
	// is now probably wrong
	bufferSize = read_port_etc(fListenPort, msgCode, msgBuffer, bufferSize,
					  B_RELATIVE_TIMEOUT, 0);
	if (bufferSize < B_OK) {
		delete[] msgBuffer;
		return NULL;
	}

	return msgBuffer;
}


BMessage *
RootLayer::ReadMessageFromPort(bigtime_t tout)
{
	int32 msgcode;
	BMessage* bmsg;

	void* msgbuffer = ReadRawFromPort(&msgcode, tout);
	if (!msgbuffer)
		return NULL;

	bmsg = ConvertToMessage(msgbuffer, msgcode);

	delete[] (int8*)msgbuffer;

	return bmsg;
}


BMessage*
RootLayer::ConvertToMessage(void* raw, int32 code)
{
	BMessage* bmsg = new BMessage(code);

	if (raw != NULL) {
		if (bmsg->Unflatten((const char*)raw) != B_OK) {
			printf("Convert To BMessage FAILED. port message code was: %ld - %c%c%c%c",
			code, (int8)(code >> 24), (int8)(code >> 16), (int8)(code >> 8), (int8)code  );
			delete bmsg;
			bmsg = NULL;
		}
	}

	return bmsg;
}
