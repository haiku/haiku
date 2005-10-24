//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		RootLayer.cpp
//	Author:			Gabe Yoder <gyoder@stny.rr.com>
//					DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@cotty.iren.ro>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Class used for the top layer of each workspace's Layer tree
//  
//------------------------------------------------------------------------------

#include <stdio.h>
#include <Window.h>
#include <List.h>
#include <Message.h>
#include <Entry.h>
#include <File.h>
#include <PortLink.h>

#include "Decorator.h"
#include "DisplayDriver.h"
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
					 Desktop *desktop, DisplayDriver *driver)
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
	fLayerData->SetHighColor(RGBColor(255, 255, 255));
	fLayerData->SetLowColor(fWorkspace[fActiveWksIndex]->BGColor());

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

	BPrivate::PortLink msg(fListenPort, -1);
	msg.StartMessage(B_QUIT_REQUESTED);
	msg.EndMessage();
	msg.Flush();

	status_t dummy;
	wait_for_thread(fThreadID, &dummy);

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
	int32 code = 0;
	status_t err = B_OK;
	RootLayer *oneRootLayer = (RootLayer*)data;
	BPrivate::PortLink messageQueue(-1, oneRootLayer->fListenPort);

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
	for (;;) {
		err = messageQueue.GetNextMessage(code);
		if (err < B_OK) {
			STRACE(("WorkingThread: messageQueue.GetNextMessage() failed: %s\n", strerror(err)));
			continue;
		}

		oneRootLayer->Lock();

		switch (code) {
			// We don't need to do anything with these two, so just pass them
			// onto the active application. Eventually, we will end up passing 
			// them onto the window which is currently under the cursor.
			case B_MOUSE_DOWN:
			case B_MOUSE_UP:
			case B_MOUSE_MOVED:
			case B_MOUSE_WHEEL_CHANGED:
				oneRootLayer->MouseEventHandler(code, messageQueue);
				break;

			case B_KEY_DOWN:
			case B_KEY_UP:
			case B_UNMAPPED_KEY_DOWN:
			case B_UNMAPPED_KEY_UP:
			case B_MODIFIERS_CHANGED:
				oneRootLayer->KeyboardEventHandler(code, messageQueue);
				break;

			case B_QUIT_REQUESTED:
				exit_thread(0);
				break;

			default:
				printf("RootLayer(%s)::WorkingThread received unexpected code %lx\n", oneRootLayer->Name(), code);
				break;
		}

		oneRootLayer->Unlock();

		// if we still have other messages in our queue, but we really want to quit
		if (oneRootLayer->fQuiting)
			break;
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

status_t
RootLayer::EnqueueMessage(BPrivate::PortLink &message)
{
	message.SetSenderPort(fListenPort);
	message.Flush();
	return B_OK;
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

		const BList&	windowList = fDesktop->WindowList();

		int32			ptrCount = windowList.CountItems();
		WinBorder		**ptrWin = (WinBorder**)windowList.Items();

		for (int32 i = 0; i < ptrCount; i++) {
			if (ptrWin[i]->Workspaces() & (0x00000001UL << index)) {
				fWorkspace[index]->AddWinBorder(ptrWin[i]);

				if (!ptrWin[i]->IsHidden()) {
					fWorkspace[index]->ShowWinBorder(ptrWin[i]);
				}
			}
		}

		fDesktop->Unlock();
	}
	// adjust our DrawData to workspace colors
	rgb_color bg = fWorkspace[index]->BGColor().GetColor32();
	if ((bg.red + bg.green + bg.blue) / 3 > 128)
		fLayerData->SetHighColor(RGBColor(0, 0, 0));
	else
		fLayerData->SetHighColor(RGBColor(255, 255, 255));
	fLayerData->SetLowColor(fWorkspace[index]->BGColor());

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
		if (oldWMState.Active && oldWMState.Active->Window()) {
			BMessage msg(B_WINDOW_ACTIVATED);
			msg.AddBool("active", false);
			oldWMState.Active->Window()->SendMessageToClient(&msg, B_NULL_TOKEN, false);
		}
		if (fWMState.Active && fWMState.Active->Window()) {
			BMessage msg(B_WINDOW_ACTIVATED);
			msg.AddBool("active", true);
			fWMState.Active->Window()->SendMessageToClient(&msg, B_NULL_TOKEN, false);
		}
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
			fWMState.WindowList.CountItems()) != 0) {
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
RootLayer::SetActive(WinBorder* newActive)
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

				returnValue = ActiveWorkspace()->AttemptToActivate(newActive);

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
	if (!(workspaceIndex & (0x00000001UL << fActiveWksIndex))) {
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
RootLayer::_ProcessMouseMovedEvent(PointerEvent &evt)
{
	Layer* target = LayerAt(evt.where);
	if (target == NULL) {
		CRITICAL("RootLayer::_ProcessMouseMovedEvent() 'target' can't be null.\n");
		return;
	}

	// change focus in FFM mode
	WinBorder* winBorderTarget = dynamic_cast<WinBorder*>(target);
	if (winBorderTarget) {
		DesktopSettings ds(gDesktop);
		// TODO: Focus should be a RootLayer option/feature, NOT a Workspace one!!!
		WinBorder* exFocus = Focus();
		if (ds.MouseMode() != B_NORMAL_MOUSE && exFocus != winBorderTarget) {
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

			lay->MouseMoved(evt, viewAction);
		}
	}

	if (!alreadyPresent)
		fMouseNotificationList.RemoveItem(target);

	fLastLayerUnderMouse = target;
	fLastMousePosition = evt.where;
}

void
RootLayer::MouseEventHandler(int32 code, BPrivate::PortLink& msg)
{
	switch (code) {
		case B_MOUSE_DOWN: {
			//printf("RootLayer::MouseEventHandler(B_MOUSE_DOWN)\n");
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down
			// 5) int32 - buttons down
			// 6) int32 - clicks

			PointerEvent evt;	
			evt.code = B_MOUSE_DOWN;
			msg.Read<int64>(&evt.when);
			msg.Read<float>(&evt.where.x);
			msg.Read<float>(&evt.where.y);
			msg.Read<int32>(&evt.modifiers);
			msg.Read<int32>(&evt.buttons);
			msg.Read<int32>(&evt.clicks);

			evt.where.ConstrainTo(fFrame);

			if (fLastMousePosition != evt.where) {
				// move cursor on screen
				GetHWInterface()->MoveCursorTo(evt.where.x, evt.where.y);

				_ProcessMouseMovedEvent(evt);
			}

			// We'll need this so that GetMouse can query for which buttons
			// are down.
			fButtons = evt.buttons;

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
					lay->MouseDown(evt);
			}

			// get the pointer for one of the first RootLayer's descendants
			Layer *primaryTarget = LayerAt(evt.where, false);
			primaryTarget->MouseDown(evt);

			break;
		}
		case B_MOUSE_UP: {
			//printf("RootLayer::MouseEventHandler(B_MOUSE_UP)\n");
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - modifier keys down

			PointerEvent evt;	
			evt.code = B_MOUSE_UP;
			msg.Read<int64>(&evt.when);
			msg.Read<float>(&evt.where.x);
			msg.Read<float>(&evt.where.y);
			msg.Read<int32>(&evt.modifiers);

			evt.where.ConstrainTo(fFrame);

			if (fLastMousePosition != evt.where) {
				// move cursor on screen
				GetHWInterface()->MoveCursorTo(evt.where.x, evt.where.y);

				_ProcessMouseMovedEvent(evt);
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
					lay->MouseUp(evt);
			}
			ClearNotifyLayer();

			if (!foundCurrent)
				fLastLayerUnderMouse->MouseUp(evt);

			// TODO: This is a quick fix to avoid the endless loop with windows created
			// with the B_ASYNCHRONOUS_CONTROLS flag, but please someone have a look into this.
			fButtons = 0;
			
			break;
		}
		case B_MOUSE_MOVED: {
			//printf("RootLayer::MouseEventHandler(B_MOUSE_MOVED)\n");
			// Attached data:
			// 1) int64 - time of mouse click
			// 2) float - x coordinate of mouse click
			// 3) float - y coordinate of mouse click
			// 4) int32 - buttons down
			
			PointerEvent evt;
			evt.code = B_MOUSE_MOVED;
			msg.Read<int64>(&evt.when);
			msg.Read<float>(&evt.where.x);
			msg.Read<float>(&evt.where.y);
			msg.Read<int32>(&evt.buttons);

			evt.where.ConstrainTo(fFrame);
			// move cursor on screen
			GetHWInterface()->MoveCursorTo(evt.where.x, evt.where.y);

			_ProcessMouseMovedEvent(evt);

			break;
		}
		case B_MOUSE_WHEEL_CHANGED: {
			//printf("RootLayer::MouseEventHandler(B_MOUSE_WHEEL_CHANGED)\n");
			// FEATURE: This is a tentative change: mouse wheel messages are always sent to the window
			// under the cursor. It's pretty stupid to send it to the active window unless a particular
			// view has locked focus via SetMouseEventMask
						
			PointerEvent evt;	
			evt.code = B_MOUSE_WHEEL_CHANGED;
			msg.Read<int64>(&evt.when);
			msg.Read<float>(&evt.wheel_delta_x);
			msg.Read<float>(&evt.wheel_delta_y);

			if (fLastLayerUnderMouse == NULL) {
				CRITICAL("RootLayer::MouseEventHandler(B_MOUSE_DOWN) fLastLayerUnderMouse is null!\n");
				break;
			}

			fLastLayerUnderMouse->MouseWheelChanged(evt);
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
RootLayer::KeyboardEventHandler(int32 code, BPrivate::PortLink& msg)
{
	switch (code) {
		case B_KEY_DOWN:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifier-independent ASCII code for the character
			// 4) int32 repeat count
			// 5) int32 modifiers
			// 6) int8[3] UTF-8 data generated
			// 7) Character string generated by the keystroke
			// 8) int8[16] state of all keys

			bigtime_t time;
			int32 scancode, modifiers;
			int8 utf[3] = { 0, 0, 0 };
			char *string = NULL;
			int8 keystates[16];
			int32 raw_char;
			int32 repeatcount;

			msg.Read<bigtime_t>(&time);
			msg.Read<int32>(&scancode);
			msg.Read<int32>(&raw_char);
			msg.Read<int32>(&repeatcount);
			msg.Read<int32>(&modifiers);
			msg.Read(utf, sizeof(utf));
			msg.ReadString(&string);
			msg.Read(keystates, sizeof(keystates));

			STRACE(("Key Down: 0x%lx\n",scancode));

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
					// TODO: Set to Safe Mode in KeyboardEventHandler:B_KEY_DOWN. (DisplayDriver API change)
					STRACE(("Safe Video Mode invoked - code unimplemented\n"));
					free(string);
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
					free(string);
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
					free(string);
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
				if (GetDisplayDriver()) {
					char filename[128];
					BEntry entry;

					int32 index = 1;
					do {
						sprintf(filename, "/boot/home/screen%ld.png", index++);
						entry.SetTo(filename);
					} while(entry.Exists());

					GetDisplayDriver()->DumpToFile(filename);

					free(string);
					break;
				}
			}

			// We got this far, so apparently it's safe to pass to the active
			// window.

			if (Focus()) {
				ServerWindow *win = Focus()->Window();
				if (win) {
					BMessage keymsg(B_KEY_DOWN);
					keymsg.AddInt64("when", time);
					keymsg.AddInt32("key", scancode);

					if (repeatcount > 1)
						keymsg.AddInt32("be:key_repeat", repeatcount);

					keymsg.AddInt32("modifiers", modifiers);
					keymsg.AddData("states", B_UINT8_TYPE, keystates, sizeof(int8) * 16);

					for (uint8 i = 0; i < 3; i++)
						keymsg.AddInt8("byte", utf[i]);

					keymsg.AddString("bytes", string);
					keymsg.AddInt32("raw_char", raw_char);

					win->SendMessageToClient(&keymsg, B_NULL_TOKEN, true);
				}
			}

			free(string);
			break;
		}

		case B_KEY_UP:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifier-independent ASCII code for the character
			// 4) int32 modifiers
			// 5) int8[3] UTF-8 data generated
			// 6) Character string generated by the keystroke
			// 7) int8[16] state of all keys

			bigtime_t time;
			int32 scancode, modifiers;
			int8 utf[3] = { 0, 0, 0 };
			char *string = NULL;
			int8 keystates[16];
			int32 raw_char;

			msg.Read<bigtime_t>(&time);
			msg.Read<int32>(&scancode);
			msg.Read<int32>(&raw_char);
			msg.Read<int32>(&modifiers);
			msg.Read(utf, sizeof(utf));
			msg.ReadString(&string);
			msg.Read(keystates, sizeof(keystates));

			STRACE(("Key Up: 0x%lx\n", scancode));

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

			if (Focus()) {
				ServerWindow *win = Focus()->Window();
				if (win) {
					BMessage keymsg(B_KEY_UP);
					keymsg.AddInt64("when", time);
					keymsg.AddInt32("key", scancode);
					keymsg.AddInt32("modifiers", modifiers);
					keymsg.AddData("states", B_UINT8_TYPE, keystates, sizeof(int8) * 16);

					for (uint8 i = 0; i < 3; i++) {
						keymsg.AddInt8("byte", utf[i]);
					}

					keymsg.AddString("bytes", string);
					keymsg.AddInt32("raw_char", raw_char);

					win->SendMessageToClient(&keymsg, B_NULL_TOKEN, true);
				}
			}

			free(string);
			break;
		}

		case B_UNMAPPED_KEY_DOWN:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifiers
			// 4) int8 state of all keys

			bigtime_t time;
			int32 scancode, modifiers;
			int8 keystates[16];
			
			msg.Read<bigtime_t>(&time);
			msg.Read<int32>(&scancode);
			msg.Read<int32>(&modifiers);
			msg.Read(keystates,sizeof(int8)*16);
	
			STRACE(("Unmapped Key Down: 0x%lx\n",scancode));
			
			if(Focus())
			{
				ServerWindow *win = Focus()->Window();
				if(win)
				{
					BMessage keymsg(B_UNMAPPED_KEY_DOWN);
					keymsg.AddInt64("when", time);
					keymsg.AddInt32("key", scancode);
					keymsg.AddInt32("modifiers", modifiers);
					keymsg.AddData("states", B_UINT8_TYPE, keystates, sizeof(int8) * 16);
					
					win->SendMessageToClient(&keymsg, B_NULL_TOKEN, true);
				}
			}
			
			break;
		}
		case B_UNMAPPED_KEY_UP:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 raw key code (scancode)
			// 3) int32 modifiers
			// 4) int8 state of all keys

			bigtime_t time;
			int32 scancode, modifiers;
			int8 keystates[16];
			
			msg.Read<bigtime_t>(&time);
			msg.Read<int32>(&scancode);
			msg.Read<int32>(&modifiers);
			msg.Read(keystates,sizeof(int8)*16);
	
			STRACE(("Unmapped Key Up: 0x%lx\n",scancode));
			
			if(Focus())
			{
				ServerWindow *win = Focus()->Window();
				if(win)
				{
					BMessage keymsg(B_UNMAPPED_KEY_UP);
					keymsg.AddInt64("when", time);
					keymsg.AddInt32("key", scancode);
					keymsg.AddInt32("modifiers", modifiers);
					keymsg.AddData("states", B_UINT8_TYPE, keystates, sizeof(int8) * 16);
					
					win->SendMessageToClient(&keymsg, B_NULL_TOKEN, true);
				}
			}
			
			break;
		}
		case B_MODIFIERS_CHANGED:
		{
			// Attached Data:
			// 1) int64 bigtime_t object of when the message was sent
			// 2) int32 modifiers
			// 3) int32 old modifiers
			// 4) int8 state of all keys
			
			bigtime_t time;
			int32 modifiers,oldmodifiers;
			int8 keystates[16];
			
			msg.Read<bigtime_t>(&time);
			msg.Read<int32>(&modifiers);
			msg.Read<int32>(&oldmodifiers);
			msg.Read(keystates,sizeof(int8)*16);

			if(Focus())
			{
				ServerWindow *win = Focus()->Window();
				if(win)
				{
					BMessage keymsg(B_MODIFIERS_CHANGED);
					keymsg.AddInt64("when", time);
					keymsg.AddInt32("modifiers", modifiers);
					keymsg.AddInt32("be:old_modifiers", oldmodifiers);
					keymsg.AddData("states", B_UINT8_TYPE, keystates, sizeof(int8) * 16);
					
					win->SendMessageToClient(&keymsg, B_NULL_TOKEN, true);
				}
			}

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

	gDesktop->SetWinBorderFeel(winBorder, newFeel);

	if (isVisible)
	{
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
						fLayerData);
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
								fLayerData);
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
							bitmapBounds.OffsetToCopy(logoPos), fLayerData);
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
