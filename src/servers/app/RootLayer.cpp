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
	  fLastMouseMoved(this),
	  fMouseTargetWinBorder(NULL),
	  fViewAction(B_ENTERED_VIEW),
	  fEventMaskLayer(NULL),

	  fCursorManager(),

	  fAllRegionsLock("root layer region lock"),

	  fThreadID(B_ERROR),
	  fListenPort(-1),

	  fScreenPtrList(32),
	  fRows(0),
	  fColumns(0),

	  fScreenWidth(0),
	  fScreenHeight(0),
	  fColorSpace(B_RGB32),
	  fFrequency(60.0),

	  fButtons(0),
	  fLastMousePosition(0.0, 0.0),
//	  fMovingWindow(false),
//	  fResizingWindow(false),
	  fHaveWinBorderList(false),

	  fActiveWksIndex(0),
	  fWsCount(0),
	  fWorkspace(new Workspace*[kMaxWorkspaceCount]),
	  fWorkspacesLayer(NULL),

	  fWinBorderListLength(64),
	  fWinBorderList2((WinBorder**)malloc(fWinBorderListLength * sizeof(WinBorder*))),
	  fWinBorderList((WinBorder**)malloc(fWinBorderListLength * sizeof(WinBorder*))),
	  fWinBorderCount(0),
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


	// easy way to identify this class.
	fClassID = AS_ROOTLAYER_CLASS;
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

	free(fWinBorderList2);
	free(fWinBorderList);
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
	oneRootLayer->invalidate_layer(oneRootLayer, oneRootLayer->Bounds());
#else
	oneRootLayer->rebuild_visible_regions(
		BRegion(oneRootLayer->Bounds()),
		BRegion(oneRootLayer->Bounds()),
		oneRootLayer->BottomChild());

	oneRootLayer->fRedrawReg.Include(oneRootLayer->Bounds());
	oneRootLayer->RequestDraw(oneRootLayer->fRedrawReg, oneRootLayer->BottomChild());
#endif
	oneRootLayer->Unlock();
	
	STRACE(("info: RootLayer(%s)::WorkingThread listening on port %ld.\n", oneRootLayer->Name(), oneRootLayer->fListenPort));
	for (;;) {
		err = messageQueue.GetNextMessage(code);
		if (err < B_OK) {
			STRACE(("WorkingThread: messageQueue.GetNextReply failed\n"));
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

			case AS_ROOTLAYER_SHOW_WINBORDER:
			{
				WinBorder *winBorder = NULL;
				messageQueue.Read<WinBorder*>(&winBorder);
				oneRootLayer->show_winBorder(winBorder);
				break;
			}
			case AS_ROOTLAYER_HIDE_WINBORDER:
			{
				WinBorder *winBorder = NULL;
				messageQueue.Read<WinBorder*>(&winBorder);
				oneRootLayer->hide_winBorder(winBorder);
				break;
			}
			case AS_ROOTLAYER_DO_INVALIDATE:
			{
//printf("Adi: new message\n");
				BRegion invalidRegion;
				Layer *layer = NULL;
				messageQueue.Read<Layer*>(&layer);
				messageQueue.ReadRegion(&invalidRegion);
#ifndef NEW_CLIPPING
				oneRootLayer->invalidate_layer(layer, invalidRegion);
#else
				layer->do_Invalidate(invalidRegion);
#endif
				break;
			}
			case AS_ROOTLAYER_DO_REDRAW:
			{
				BRegion redrawRegion;
				Layer *layer = NULL;
				messageQueue.Read<Layer*>(&layer);
				messageQueue.ReadRegion(&redrawRegion);
#ifndef NEW_CLIPPING
				oneRootLayer->redraw_layer(layer, redrawRegion);
#else
				layer->do_Redraw(redrawRegion);
#endif
				break;
			}
			case AS_ROOTLAYER_LAYER_MOVE:
			{
				Layer *layer = NULL;
				float x, y;
				messageQueue.Read<Layer*>(&layer);
				messageQueue.Read<float>(&x);
				messageQueue.Read<float>(&y);
#ifndef NEW_CLIPPING
				layer->move_layer(x, y);
#else
				layer->do_MoveBy(x, y);
#endif
				break;
			}
			case AS_ROOTLAYER_LAYER_RESIZE:
			{
				Layer *layer = NULL;
				float x, y;
				messageQueue.Read<Layer*>(&layer);
				messageQueue.Read<float>(&x);
				messageQueue.Read<float>(&y);
#ifndef NEW_CLIPPING
				layer->resize_layer(x, y);
#else
				layer->do_ResizeBy(x, y);
#endif
				break;
			}
			case AS_ROOTLAYER_LAYER_SCROLL:
			{
				Layer *layer = NULL;
				float x, y;
				messageQueue.Read<Layer*>(&layer);
				messageQueue.Read<float>(&x);
				messageQueue.Read<float>(&y);
#ifndef NEW_CLIPPING
				// nothing
#else
				layer->do_ScrollBy(x, y);
#endif
				break;
			}
			case AS_ROOTLAYER_LAYER_COPYBITS:
			{
				Layer *layer = NULL;
				int32 xOffset, yOffset;
				BRect src, dst;
				messageQueue.Read<Layer*>(&layer);
				messageQueue.Read<BRect>(&src);
				messageQueue.Read<BRect>(&dst);
				messageQueue.Read<int32>(&xOffset);
				messageQueue.Read<int32>(&yOffset);

				layer->do_CopyBits(src, dst, xOffset, yOffset);

				break;
			}

			case AS_ROOTLAYER_ADD_TO_SUBSET:
			{
				WinBorder *winBorder = NULL;
				WinBorder *toWinBorder = NULL;
				messageQueue.Read<WinBorder*>(&winBorder);
				messageQueue.Read<WinBorder*>(&toWinBorder);
				oneRootLayer->fDesktop->AddWinBorderToSubset(winBorder, toWinBorder);
				break;
			}
			case AS_ROOTLAYER_REMOVE_FROM_SUBSET:
			{
				WinBorder *winBorder = NULL;
				WinBorder *fromWinBorder = NULL;
				messageQueue.Read<WinBorder*>(&winBorder);
				messageQueue.Read<WinBorder*>(&fromWinBorder);
				oneRootLayer->fDesktop->RemoveWinBorderFromSubset(winBorder, fromWinBorder);
				break;
			}
			case AS_ROOTLAYER_WINBORDER_SET_WORKSPACES:
			{
				WinBorder *winBorder = NULL;
				uint32 oldWks = 0, newWks = 0;

				messageQueue.Read<WinBorder*>(&winBorder);
				messageQueue.Read<uint32>(&oldWks);
				messageQueue.Read<uint32>(&newWks);
				oneRootLayer->SetWinBorderWorskpaces(winBorder, oldWks, newWks);
				break;
			}
			case AS_ROOTLAYER_DO_CHANGE_WINBORDER_FEEL:
			{
				WinBorder *winBorder = NULL;
				int32 newFeel = 0;

				messageQueue.Read<WinBorder*>(&winBorder);
				messageQueue.Read<int32>(&newFeel);
				oneRootLayer->change_winBorder_feel(winBorder, newFeel);
				break;
			}
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
RootLayer::GoInvalidate(const Layer *layer, const BRegion &region)
{
	BPrivate::PortLink msg(fListenPort, -1);
	msg.StartMessage(AS_ROOTLAYER_DO_INVALIDATE);
	msg.Attach<const Layer*>(layer);
	msg.AttachRegion(region);
	msg.Flush();
}

#ifndef NEW_CLIPPING
void RootLayer::invalidate_layer(Layer *layer, const BRegion &region)
{
	// NOTE: our thread (WorkingThread) is locked here.
	STRACE(("RootLayer::invalidate_layer(%s)\n", layer->Name()));

	if (layer->fParent)
		layer = layer->fParent;

	layer->FullInvalidate(region);
}
#endif

status_t
RootLayer::EnqueueMessage(BPrivate::PortLink &message)
{
	message.SetSenderPort(fListenPort);
	message.Flush();
	return B_OK;
}


void
RootLayer::GoRedraw(const Layer *layer, const BRegion &region)
{
	BPrivate::PortLink msg(fListenPort, -1);
	msg.StartMessage(AS_ROOTLAYER_DO_REDRAW);
	msg.Attach<const Layer*>(layer);
	msg.AttachRegion(region);
	msg.Flush();
}

#ifndef NEW_CLIPPING
void
RootLayer::redraw_layer(Layer *layer, const BRegion &region)
{
	// NOTE: our thread (WorkingThread) is locked here.

	layer->Invalidate(region);
}
#endif

void
RootLayer::GoChangeWinBorderFeel(const WinBorder *winBorder, int32 newFeel)
{
	BPrivate::PortLink msg(fListenPort, -1);
	msg.StartMessage(AS_ROOTLAYER_DO_CHANGE_WINBORDER_FEEL);
	msg.Attach<const WinBorder*>(winBorder);
	msg.Attach<int32>(newFeel);
	msg.Flush();
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


Layer *
RootLayer::TopChild() const
{
	fWinBorderIndex	= fWinBorderCount-1;

	if (fWinBorderIndex < fWinBorderCount && fWinBorderIndex >= 0)
		return fWinBorderList[fWinBorderIndex--];

	return NULL;
}

Layer* RootLayer::LowerSibling() const
{
	if (fWinBorderIndex < fWinBorderCount && fWinBorderIndex > 0)
		return fWinBorderList[fWinBorderIndex--];

	return NULL;
}

Layer* RootLayer::UpperSibling() const
{
	if (fWinBorderIndex < fWinBorderCount && fWinBorderIndex > 0)
		return fWinBorderList[fWinBorderIndex++];

	return NULL;
}

Layer* RootLayer::BottomChild() const
{
	fWinBorderIndex	= 0;

	if (fWinBorderIndex < fWinBorderCount && fWinBorderIndex >= 0)
		return fWinBorderList[fWinBorderIndex++];

	return NULL;
}


void RootLayer::AddWinBorder(WinBorder* winBorder)
{
	if (!winBorder->IsHidden())
	{
		CRITICAL("RootLayer::RemoveWinBorder - winBorder must be hidden\n");
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

void RootLayer::RemoveWinBorder(WinBorder* winBorder)
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
	for (int32 i = 0; i < fWsCount; i++)
		if (fWorkspace[i])
			fWorkspace[i]->RemoveWinBorder(winBorder);

	// we _DO_NOT_ need to invalidate here. At this point our WinBorder is hidden!

	// set some internals
	winBorder->SetRootLayer(NULL);
	winBorder->fParent = NULL;
}

void RootLayer::AddSubsetWinBorder(WinBorder *winBorder, WinBorder *toWinBorder)
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

	bool		invalidate	= false;
	bool		invalid;
	WinBorder	*exFocus	= FocusWinBorder();
	WinBorder	*exActive	= ActiveWinBorder();

	// we try to add WinBorders to all workspaces. If they are not needed, nothing will be done.
	// If they are needed, Workspace automaticaly allocates space and inserts them.
	for (int32 i = 0; i < fWsCount; i++)
	{
		invalid		= false;

		if (fWorkspace[i] && fWorkspace[i]->HasWinBorder(toWinBorder))
			invalid = fWorkspace[i]->ShowWinBorder(winBorder, false);

		if (fActiveWksIndex == i)
			invalidate = invalid;
	}

	if (invalidate)
		show_final_scene(exFocus, exActive);
}

void RootLayer::RemoveSubsetWinBorder(WinBorder *winBorder, WinBorder *fromWinBorder)
{
	// there is no point in continuing - this subset window is not visible
	// at least not visible from 'fromWinBorder's point of view.
	if (winBorder->IsHidden() || fromWinBorder->IsHidden())
	{
		return;
	}

	bool		invalidate	= false;
	bool		invalid;
	WinBorder	*exFocus = FocusWinBorder();
	WinBorder	*exActive	= ActiveWinBorder();

	// we try to remove from all workspaces. If winBorder is not in there, nothing will be done.
	for (int32 i = 0; i < fWsCount; i++)
	{
		invalid		= false;

		if (fWorkspace[i] && fWorkspace[i]->HasWinBorder(fromWinBorder))
			invalid = fWorkspace[i]->HideWinBorder(winBorder);

		if (fActiveWksIndex == i)
			invalidate = invalid;
	}

	if (invalidate)
		show_final_scene(exFocus, exActive);
}

// NOTE: This must be called by RootLayer's thread!!!!
bool RootLayer::SetActiveWorkspace(int32 index)
{
	STRACE(("RootLayer(%s)::SetActiveWorkspace(%ld)\n", Name(), index));

	// nice try!
	if (index >= fWsCount || index == fActiveWksIndex || index < 0)
		return false;

	// you cannot switch workspaces on R5 if there is an event mask layer
	if (fEventMaskLayer)
		return false;

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

				if (!ptrWin[i]->IsHidden())
					fWorkspace[index]->ShowWinBorder(ptrWin[i]);
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

	int32 exIndex = ActiveWorkspaceIndex();

	// send the workspace changed message for the old workspace
	{
		BMessage activatedMsg(B_WORKSPACE_ACTIVATED);
		activatedMsg.AddInt64("when", real_time_clock_usecs());
		activatedMsg.AddInt32("workspace", fActiveWksIndex);
		activatedMsg.AddBool("active", false);

		for (int32 i = 0; i < fWinBorderCount; i++)
			fWinBorderList[i]->Window()->SendMessageToClient(&activatedMsg, B_NULL_TOKEN, false);
	}

	fActiveWksIndex	= index;

	if (fMouseTargetWinBorder) {
//	if (fMovingWindow && fEventMaskLayer) {
//		WinBorder	*movingWinBorder = (WinBorder*)fEventMaskLayer;

//		movingWinBorder->MouseUp(DEC_NONE);

		// NOTE: DO NOT reset these 2 members! We still want to move our window in the new workspace
		//fEventMaskLayer	= NULL;
		//fMovingWindow = false;
//		fResizingWindow	= false;

		// Take the WinBorder we're currently dragging with us to the new workspace
		// NOTE: The code looks broken for WinBorders that show on multiple workspaces
		// at the same time.

		// only normal windows can change workspaces
		if (fMouseTargetWinBorder->Feel() == B_NORMAL_WINDOW_FEEL
			&& !ActiveWorkspace()->HasWinBorder(fMouseTargetWinBorder)) {
			// Workspace class expects a window to be hidden when it's about to be removed.
			// As we surely know this windows is visible, we simply set fHidden to true and then
			// change it back when adding winBorder to the current workspace.
			fMouseTargetWinBorder->fHidden = true;
			fWorkspace[exIndex]->HideWinBorder(fMouseTargetWinBorder);
			fWorkspace[exIndex]->RemoveWinBorder(fMouseTargetWinBorder);

			fMouseTargetWinBorder->fHidden = false;
			ActiveWorkspace()->AddWinBorder(fMouseTargetWinBorder);
			ActiveWorkspace()->ShowWinBorder(fMouseTargetWinBorder);

			// TODO: can you call SetWinBorderWorskpaces() instead of this?
			uint32 wks = fMouseTargetWinBorder->Workspaces();
			BMessage changedMsg(B_WORKSPACES_CHANGED);
			changedMsg.AddInt64("when", real_time_clock_usecs());
			changedMsg.AddInt32("old", wks);
			wks &= ~(0x00000001 << exIndex);
			wks |= (0x00000001 << fActiveWksIndex);
			changedMsg.AddInt32("new", wks);
			fMouseTargetWinBorder->QuietlySetWorkspaces(wks);
			fMouseTargetWinBorder->Window()->SendMessageToClient(&changedMsg, B_NULL_TOKEN, false);
		}
	}/* else if (fEventMaskLayer) {
		// is this a WinBorder object?
		if (!fEventMaskLayer->fOwner)
		{
			// this WinBorder captured the mouse for a reason. allow it to finish its task.
			((WinBorder*)fEventMaskLayer)->MouseUp(DEC_NONE);
		}

		fEventMaskLayer	= NULL;
		fMovingWindow = false;
		fResizingWindow	= false;
	}*/

	fHaveWinBorderList = true;
	get_workspace_windows();

	if (WorkspacesLayer() != NULL)
		GoRedraw(WorkspacesLayer(), WorkspacesLayer()->fVisible);

	// send the workspace changed message for the new workspace
	{
		BMessage	activatedMsg(B_WORKSPACE_ACTIVATED);
		activatedMsg.AddInt64("when", real_time_clock_usecs());
		activatedMsg.AddInt32("workspace", fActiveWksIndex);
		activatedMsg.AddBool("active", true);

		for (int32 i = 0; i < fWinBorderCount; i++)
			fWinBorderList[i]->Window()->SendMessageToClient(&activatedMsg, B_NULL_TOKEN, false);
	}

	// workspace changed
	return true;
}

void RootLayer::SetWinBorderWorskpaces(WinBorder *winBorder, uint32 oldWksIndex, uint32 newWksIndex)
{
	// you *cannot* set workspaces index for a window other than a normal one!
	// Note: See ServerWindow class.
	if (winBorder->Feel() != B_NORMAL_WINDOW_FEEL)
		return;

	bool		invalidate	= false;
	bool		invalid;
	WinBorder	*exFocus	= FocusWinBorder();
	WinBorder	*exActive	= ActiveWinBorder();

	for (int32 i = 0; i < 32; i++)
	{
		if (fWorkspace[i])
		{
			invalid = false;

			if (fWorkspace[i]->HasWinBorder(winBorder)
				&& !(newWksIndex & (0x00000001UL << i)))
			{
				if (!winBorder->IsHidden())
				{
					// a little trick to force Workspace to properly pick the next front.
					winBorder->fHidden = true;
					invalid = fWorkspace[i]->HideWinBorder(winBorder);
					winBorder->fHidden = false;
				}
				fWorkspace[i]->RemoveWinBorder(winBorder);
			}
			else
			if (newWksIndex & (0x00000001UL << i) &&
				!(oldWksIndex & (0x00000001UL << i)))
			{
				fWorkspace[i]->AddWinBorder(winBorder);
				if (!winBorder->IsHidden())
					invalid = fWorkspace[i]->ShowWinBorder(winBorder);
			}
			else
			{
				// do nothing. winBorder was, and it still is a member of this workspace
				// OR, winBorder wasn't and it will not be in this workspace
			}

			if (fActiveWksIndex == i)
				invalidate = invalid;
		}
	}

// TODO: look into this...
	if (fEventMaskLayer)
	{
		WinBorder	*wb	= fEventMaskLayer->fOwner?
							fEventMaskLayer->fOwner:
							(WinBorder*)fEventMaskLayer;
		if (!fWorkspace[fActiveWksIndex]->HasWinBorder(wb))
		{
/*			if (wb == fEventMaskLayer)
			{
				fMovingWindow	= false;
				fResizingWindow	= false;
				wb->MouseUp(DEC_NONE);
			}*/
			fEventMaskLayer	= NULL;
		}
	}

	BMessage	changedMsg(B_WORKSPACES_CHANGED);
	changedMsg.AddInt64("when", real_time_clock_usecs());
	changedMsg.AddInt32("old", oldWksIndex);
	changedMsg.AddInt32("new", newWksIndex);
	winBorder->QuietlySetWorkspaces(newWksIndex);
	winBorder->Window()->SendMessageToClient(&changedMsg, B_NULL_TOKEN, false);

	if (invalidate)
		show_final_scene(exFocus, exActive);
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

void RootLayer::ReadWorkspaceData(const char *path)
{
	BMessage msg, settings;
	BFile file(path,B_READ_ONLY);
	char string[20];
	
	if(file.InitCheck()==B_OK && msg.Unflatten(&file)==B_OK)
	{
		int32 count;
		
		if(msg.FindInt32("workspace_count",&count)!=B_OK)
			count=9;
		
		SetWorkspaceCount(count);
		
		for(int32 i=0; i<count; i++)
		{
			Workspace *ws=(Workspace*)fWorkspace[i];
			if(!ws)
				continue;
			
			sprintf(string,"workspace %ld",i);
			
			if(msg.FindMessage(string,&settings)==B_OK)
			{
				ws->GetSettings(settings);
				settings.MakeEmpty();
			}
			else
				ws->GetDefaultSettings();
		}
	}
	else
	{
		SetWorkspaceCount(9);
		
		for(int32 i=0; i<9; i++)
		{
			Workspace *ws=(Workspace*)fWorkspace[i];
			if(!ws)
				continue;
			
			ws->GetDefaultSettings();
		}
	}
}

void RootLayer::SaveWorkspaceData(const char *path)
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
	BPrivate::PortLink msg(fListenPort, -1);
	msg.StartMessage(AS_ROOTLAYER_HIDE_WINBORDER);
	msg.Attach<WinBorder*>(winBorder);
	msg.Flush();
}


void
RootLayer::ShowWinBorder(WinBorder* winBorder)
{
	BPrivate::PortLink msg(fListenPort, -1);
	msg.StartMessage(AS_ROOTLAYER_SHOW_WINBORDER);
	msg.Attach<WinBorder*>(winBorder);
	msg.Flush();
}


WinBorder *
RootLayer::WinBorderAt(const BPoint& pt) const
{
	for (int32 i = 0; i < fWinBorderCount; i++)
	{
#ifndef NEW_CLIPPING
		if (fWinBorderList[i]->fFullVisible.Contains(pt))
			return fWinBorderList[i];
#endif
	}
	return NULL;
}


void
RootLayer::SetScreens(Screen *screens[], int32 rows, int32 columns)
{
	// NOTE: All screens *must* have the same resolution

	// TODO: This function is badly named. Appearently, it
	// adjusts the root layers frame rectangle, taking the information
	// from the first screen in its list. However, a Screen object
	// has actually now a different meaning. It manages access to
	// the hardware and *owns* the HWInterface instance.
	// This means there is going to be one Screen object per physical
	// screen attached (unless I misunderstood how it was intended).
	// A workspace needs to be attached to a screen
	// and then on workspace activation, the appropriate updating
	// needs to occur. The workspace tells its screen to configure
	// to the workspaces mode, and then someone takes care of
	// telling RootLayer to adjust itself and all other layers regions.

	uint16 width, height;
	uint32 colorSpace;
	float frequency;
	screens[0]->GetMode(width, height, colorSpace, frequency);

	fFrame.Set(0, 0, width * columns - 1, height * rows - 1);
	fRows = rows;
	fColumns = columns;
	fScreenWidth = width;
	fScreenHeight = height;
}


Screen **
RootLayer::Screens()
{
	return (Screen**)fScreenPtrList.Items();
}


bool
RootLayer::SetScreenMode(int32 width, int32 height, uint32 colorSpace, float frequency)
{
	if (fScreenWidth == width && fScreenHeight == height
		&& fColorSpace == colorSpace && frequency == fFrequency)
		return true;

	// NOTE: Currently, we have only one screen in that list.
	// Before I changed it, this function would only accept modes
	// that each of the (potentially multiple) screens can accept.
	// However, I didn't really know what this gives in practice.
	// We should re-think this when we really do support multiple monitors.
	// For now, this function could potentially set the first couple of
	// screens to the new mode, and fail to do so for the rest of them.

	status_t ret = B_ERROR;
	for (int i = 0; i < fScreenPtrList.CountItems(); i++) {
		Screen *screen = static_cast<Screen *>(fScreenPtrList.ItemAt(i));

		ret = screen->SetMode(width, height, colorSpace, frequency);
		if (ret < B_OK)
			break;
	}

	SetScreens(Screens(), fRows, fColumns);

	return ret >= B_OK;
}

//---------------------------------------------------------------------------
//				Workspace related methods
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//				Input related methods
//---------------------------------------------------------------------------
void
RootLayer::MouseEventHandler(int32 code, BPrivate::PortLink& msg)
{
	switch(code) {
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

			if (fLastMousePosition != evt.where) {
				// TODO: a B_MOUSE_MOVED message might have to be generated in order to
				// correctly trigger entered/exited view transits.
// Commented for now, since it is not _that_ critical and happens frequently with the Haiku
// mouse driver (which is ok, but we need to catch it here).
//				CRITICAL("mouse position changed in B_MOUSE_DOWN from last B_MOUSE_MOVED\n");
				// update on screen mouse pos
				GetHWInterface()->MoveCursorTo(evt.where.x, evt.where.y);
				fLastMousePosition	= evt.where;
			}
			
			// We'll need this so that GetMouse can query for which buttons
			// are down.
			fButtons = evt.buttons;

			if (fLastMouseMoved == NULL) {
				CRITICAL("RootLayer::MouseEventHandler(B_MOUSE_DOWN) fLastMouseMoved is null!\n");
				break;
			}

			if (fLastMouseMoved == this)
				break;

			// we are clicking a WinBorder

			WinBorder* exActive	= ActiveWinBorder();
			WinBorder* exFocus	= FocusWinBorder();
			WinBorder* target	= fLastMouseMoved->fOwner ? fLastMouseMoved->fOwner
														  : (WinBorder*)fLastMouseMoved;

			click_type action = target->MouseDown(evt);

			// TODO: only move to front if *not* in Focus Follows Mouse mode!
			bool invalidate = action == DEC_MOVETOBACK ? ActiveWorkspace()->MoveToBack(target)
													   : ActiveWorkspace()->MoveToFront(target);

			// Performance: MoveToFront() often returns true although it shouldn't do that.
			// This is because internaly it calls Workspace::ShowWinBorder() and this imposes
			// a small penalty, but we can live with that; it make Workspace code a lot more clear/clean.

			if (invalidate)
				show_final_scene(exFocus, exActive);

			// some of the actions are handled by the WinBorder itself,
			// others are handled by RootLayer
			fMouseTargetWinBorder = NULL;
			switch (action) {
				case DEC_MOVETOBACK:
					// TODO: handle here once we support FFM...
					// now it is handled above
					break;
				case DEC_NONE: {
					// TODO: bring to front if not in FFM...
					// now it is handled above
					bool sendMessage = true;
					// supress mouse down events if the window has no focus or
					// does not accept first clicks
					if (target != FocusWinBorder())
						sendMessage = false;
					else if (exFocus != FocusWinBorder()
							 && !(target->WindowFlags() & B_WILL_ACCEPT_FIRST_CLICK))
						sendMessage = false;
	
					if (sendMessage && fLastMouseMoved != target->fTopLayer) {
						BMessage msg;
						msg.what = B_MOUSE_DOWN;
						msg.AddInt64("when", evt.when);
						msg.AddPoint("where", evt.where);
						msg.AddInt32("modifiers", evt.modifiers);
						msg.AddInt32("buttons", evt.buttons);
						msg.AddInt32("clicks", evt.clicks);
	
						target->Window()->SendMessageToClient(&msg, fLastMouseMoved->fViewToken, false);
					}

					if (fLastMouseMoved->EventMask() & B_POINTER_EVENTS) {
						fEventMaskLayer = fLastMouseMoved;
					}
					break;
				}

				case DEC_DRAG:
				case DEC_RESIZE:
				default:
					// TODO: bring to front if not in FFM...
					// now it is handled above
					fMouseTargetWinBorder = target;
					break;
			}

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

			if (fLastMouseMoved == NULL) {
				CRITICAL("RootLayer::MouseEventHandler(B_MOUSE_UP) fLastMouseMoved is null!\n");
				break;
			}

			if (fLastMousePosition != evt.where) {
				// TODO: a B_MOUSE_MOVED message might have to be generated in order to
				// correctly trigger entered/exited view transits.
fprintf(stderr, "mouse position changed in B_MOUSE_UP (%.1f, %.1f) from last B_MOUSE_MOVED (%.1f, %.1f)!\n",
		evt.where.x, evt.where.y, fLastMousePosition.x, fLastMousePosition.y);
				// update on screen mouse pos
				GetHWInterface()->MoveCursorTo(evt.where.x, evt.where.y);
				fLastMousePosition	= evt.where;
			}

			// TODO: what if 'fEventMaskLayer' is deleted in the mean time.
			if (fEventMaskLayer) {
				// if this is a regular Layer, sent message to counterpart BView.
				if (fEventMaskLayer->fOwner) {
					BMessage upmsg(B_MOUSE_UP);
					upmsg.AddInt64("when",evt.when);
					upmsg.AddPoint("where",evt.where);
					upmsg.AddInt32("modifiers",evt.modifiers);

					fEventMaskLayer->Window()->SendMessageToClient(&upmsg, fEventMaskLayer->fViewToken, false);
				} else {
					// this surely is a WinBorder
// TODO: This code is too confusing. There should be a clear separation.
//					((WinBorder*)fEventMaskLayer)->MouseUp(((WinBorder*)fEventMaskLayer)->TellWhat(evt));
				}

				fEventMaskLayer	= NULL;	
			} else {
				// NOTE: focus may be NULL
				if (fLastMouseMoved->Window() && fLastMouseMoved->fOwner == FocusWinBorder()) {
					// send B_MOUSE_UP for regular Layers/BViews
					BMessage upmsg(B_MOUSE_UP);
					upmsg.AddInt64("when",evt.when);
					upmsg.AddPoint("where",evt.where);
					upmsg.AddInt32("modifiers",evt.modifiers);

					fLastMouseMoved->Window()->SendMessageToClient(&upmsg, fLastMouseMoved->fViewToken, false);
				} else {
					// WinBorders don't need _UP_ messages unless they received _DOWN_ messages,
					// and that case is threated above - WinBorders use Layer::fEventMask to
					// capture the mouse.
				}
			}

//			fMovingWindow	= false;
//			fResizingWindow	= false;
			if (fMouseTargetWinBorder) {
				fMouseTargetWinBorder->MouseUp(evt);
				fMouseTargetWinBorder = NULL;
			}

			STRACE(("MOUSE UP: at (%f, %f)\n", evt.where.x, evt.where.y));
			
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

			GetHWInterface()->MoveCursorTo(evt.where.x, evt.where.y);

			fLastMousePosition = evt.where;

			// NOTE: The mouse does NOT have to be over the fMouseTargetWinBorder for
			// it to be still the valid target of the event. In fact, it is
			// the only valid target until MOUSE_UP. So don't move this block farther
			// down.
			if (fMouseTargetWinBorder) {
				fMouseTargetWinBorder->MouseMoved(evt);
				fLastMouseMoved = fMouseTargetWinBorder;
				break;
			}

// TODO: lock ServerWindow here! Don't forget, you need to add/remove Layers
// from within RootLayer's thread!!!
			Layer* target = LayerAt(evt.where);
			if (target == NULL) {
				CRITICAL("RootLayer::MouseEventHandler(B_MOUSE_MOVED) 'target' can't be null.\n");
				break;
			}
			WinBorder* winBorderUnder = NULL;

			// TODO: I think that windows created without the B_ASYNCHRONOUS_CONTROLS flag
			// don't receive B_MOUSE_MOVED events after a B_MOUSE_DOWN. Test.
			// This will need to be cleaned up as following the code flow
			// is very hard.
			
			// fEventMaskLayer is always != this
			if (fEventMaskLayer) {
				if (fEventMaskLayer == target) {

					if (target == fLastMouseMoved)
						fViewAction = B_INSIDE_VIEW;
					else
						fViewAction = B_ENTERED_VIEW;

				} else if (fEventMaskLayer == fLastMouseMoved) {

					fViewAction = B_EXITED_VIEW;

				} else {

					fViewAction = B_OUTSIDE_VIEW;
				}

				if (fEventMaskLayer->fOwner) {
					// top layer does not have B_POINTER_EVENTS in its event mask
					BMessage movemsg(B_MOUSE_MOVED);
					movemsg.AddInt64("when", evt.when);
					movemsg.AddPoint("where", evt.where);
					movemsg.AddInt32("buttons", evt.buttons);
					movemsg.AddInt32("transit", fViewAction);

					fEventMaskLayer->Window()->SendMessageToClient(&movemsg, fEventMaskLayer->fViewToken, false);
				} else {
					winBorderUnder	= (WinBorder*)fEventMaskLayer;
				}
			} else {
				if (fLastMouseMoved != target) {
					fViewAction = B_EXITED_VIEW;
					if (fLastMouseMoved->fOwner) {
						if (fLastMouseMoved != fLastMouseMoved->fOwner->fTopLayer) {
							BMessage movemsg(B_MOUSE_MOVED);
							movemsg.AddInt64("when",evt.when);
							movemsg.AddPoint("where",evt.where);
							movemsg.AddInt32("buttons",evt.buttons);
							movemsg.AddInt32("transit",fViewAction);
							fLastMouseMoved->Window()->SendMessageToClient(&movemsg, fLastMouseMoved->fViewToken, false);
						}
					}// else if (fLastMouseMoved != this)
					//	((WinBorder*)fLastMouseMoved)->MouseMoved(DEC_NONE);

					fViewAction = B_ENTERED_VIEW;
				} else {
					fViewAction = B_INSIDE_VIEW;
				}

				if (target->fOwner) {
					if (target != target->fOwner->fTopLayer) {
						BMessage movemsg(B_MOUSE_MOVED);
						movemsg.AddInt64("when",evt.when);
						movemsg.AddPoint("where",evt.where);
						movemsg.AddInt32("buttons",evt.buttons);
						movemsg.AddInt32("transit",fViewAction);
						target->Window()->SendMessageToClient(&movemsg, target->fViewToken, false);
					}
				} else if (target != this) {
					winBorderUnder	= (WinBorder*)target;
				}
			}

/*			if (winBorderUnder) {

				BPoint delta = evt.where - fLastMousePosition;

				if (fMovingWindow) {

					winBorderUnder->MoveBy(delta.x, delta.y);

					// I know the way we get BWindow's new location it's not nice :-),
					// but I don't see another way. fTopLayer.Frame().LeftTop() does not change, ever.
					BPoint		location(winBorderUnder->fTopLayer->fFull.Frame().LeftTop());
					BMessage	msg(B_WINDOW_MOVED);
					msg.AddPoint("where", location);
					winBorderUnder->Window()->SendMessageToClient(&msg, B_NULL_TOKEN, false);

				} else if (fResizingWindow) {

					winBorderUnder->ResizeBy(delta.x, delta.y);

					BRect		frame(winBorderUnder->fTopLayer->Frame());
					BMessage	msg(B_WINDOW_RESIZED);
					msg.AddInt32("width", frame.Width());
					msg.AddInt32("height", frame.Height());
					winBorderUnder->Window()->SendMessageToClient(&msg, B_NULL_TOKEN, false);

				} else {
					winBorderUnder->MouseMoved(winBorderUnder->TellWhat(evt));
				}
			}*/

			fLastMouseMoved = target;

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

			if (fLastMouseMoved && fLastMouseMoved != this) {
				if (fLastMouseMoved->fOwner) { // is a Layer object not a WinBorder one
					if (fLastMouseMoved->fOwner->fTopLayer != fLastMouseMoved) { // must not be the top_view's counterpart
						BMessage wheelmsg(B_MOUSE_WHEEL_CHANGED);
						wheelmsg.AddInt64("when", evt.when);
						wheelmsg.AddFloat("be:wheel_delta_x",evt.wheel_delta_x);
						wheelmsg.AddFloat("be:wheel_delta_y",evt.wheel_delta_y);
						fLastMouseMoved->Window()->SendMessageToClient(&wheelmsg, fLastMouseMoved->fViewToken, false);
					}
				} else {
					// TODO: WinBorder::MouseWheel() should dissapear or get other params!
					// ((WinBorder*)fLastMouseMoved)->MouseWheel(...)
				}
			}
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

	switch(code)
	{
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
			msg.Read(keystates,sizeof(int8)*16);
			
			STRACE(("Key Down: 0x%lx\n",scancode));
			
#if !TEST_MODE
			// Check for workspace change or safe video mode
			if(scancode>0x01 && scancode<0x0e)
			{
				if(scancode==0x0d)
				{
					if(modifiers & (B_LEFT_COMMAND_KEY |
						B_LEFT_CONTROL_KEY | B_LEFT_SHIFT_KEY))
					{
						// TODO: Set to Safe Mode in KeyboardEventHandler:B_KEY_DOWN. (DisplayDriver API change)
						STRACE(("Safe Video Mode invoked - code unimplemented\n"));
						if (string)
							free(string);
						break;
					}
				}
				
				if(modifiers & B_CONTROL_KEY)
				{
					STRACE(("Set Workspace %ld\n",scancode-1));
					
					//TODO: SetWorkspace in KeyboardEventHandler
					//SetWorkspace(scancode-2);
					if (string)
						free(string);
					break;
				}	
			}
			
			// Tab key
			if(scancode==0x26 && (modifiers & B_CONTROL_KEY))
			{
				//ServerApp *deskbar=app_server->FindApp("application/x-vnd.Be-TSKB");
				//if(deskbar)
				//{
					WinBorder *exActive = ActiveWinBorder();
					WinBorder *exFocus = FocusWinBorder();
					if (ActiveWorkspace()->MoveToBack(exActive))
						show_final_scene(exFocus, exActive);
					
					//printf("Send Twitcher message key to Deskbar - unimplmemented\n");
					if (string)
						free(string);
					break;
				//}
			}

			// PrintScreen
			if(scancode==0xe)
			{
				if(GetDisplayDriver())
				{
					char filename[128];
					BEntry entry;
					
					sprintf(filename,"/boot/home/screen%ld.png",fScreenShotIndex);
					entry.SetTo(filename);
					
					while(entry.Exists())
					{
						fScreenShotIndex++;
						sprintf(filename,"/boot/home/screen%ld.png",fScreenShotIndex);
						entry.SetTo(filename);
					}
					fScreenShotIndex++;
					GetDisplayDriver()->DumpToFile(filename);
					if (string)
						free(string);
					break;
				}
			}
#else	// TEST_MODE
			// F12
			if(scancode>0x1 && scancode<0xe)
			{
				if(scancode==0xd)
				{
					if(modifiers & (B_LEFT_CONTROL_KEY | B_LEFT_SHIFT_KEY | B_LEFT_OPTION_KEY))
					{
						// TODO: Set to Safe Mode in KeyboardEventHandler:B_KEY_DOWN. (DisplayDriver API change)
						STRACE(("Safe Video Mode invoked - code unimplemented\n"));
						if (string)
							free(string);
						break;
					}
				}
				if(modifiers & (B_LEFT_SHIFT_KEY | B_LEFT_CONTROL_KEY))
				{
					STRACE(("Set Workspace %ld\n",scancode-1));
					WinBorder		*exFocus	= FocusWinBorder();
					WinBorder		*exActive	= ActiveWinBorder();

					if (SetActiveWorkspace(scancode-2))
						show_final_scene(exFocus, exActive);
					if (string)
						free(string);

					#ifdef APPSERVER_ROOTLAYER_SHOW_WORKSPACE_NUMBER
					{
						// to draw the current Workspace index on screen.
#ifndef NEW_CLIPPING
					BRegion	reg(fVisible);
					fDriver->ConstrainClippingRegion(&reg);
					Draw(reg.Frame());
					fDriver->ConstrainClippingRegion(NULL);
#endif
					}
					#endif

					break;
				}	
			}
			
			//Tab
			if(scancode==0x26 && (modifiers & B_SHIFT_KEY))
			{
				STRACE(("Twitcher\n"));
				//ServerApp *deskbar=app_server->FindApp("application/x-vnd.Be-TSKB");
				//if(deskbar)
				//{
					WinBorder *exActive = ActiveWinBorder();
					WinBorder *exFocus = FocusWinBorder();
					if (ActiveWorkspace()->MoveToBack(exActive))
						show_final_scene(exFocus, exActive);
					
					printf("Send Twitcher message key to Deskbar - unimplmemented\n");
					if (string)
						free(string);
					break;
				//}
			}

			// Pause/Break
			if(scancode==0x7f)
			{
				if(GetDisplayDriver())
				{
					char filename[128];
					BEntry entry;
					
					sprintf(filename,"/boot/home/screen%ld.png",fScreenShotIndex);
					entry.SetTo(filename);
					
					while(entry.Exists())
					{
						fScreenShotIndex++;
						sprintf(filename,"/boot/home/screen%ld.png",fScreenShotIndex);
						entry.SetTo(filename);
					}
					fScreenShotIndex++;

					GetDisplayDriver()->DumpToFile(filename);
					if (string)
						free(string);
					break;
				}
			}
#endif	// !TEST_MODE
			
			// We got this far, so apparently it's safe to pass to the active
			// window.

			if(FocusWinBorder())
			{
				ServerWindow *win = FocusWinBorder()->Window();
				if(win)
				{
					BMessage keymsg(B_KEY_DOWN);
					keymsg.AddInt64("when", time);
					keymsg.AddInt32("key", scancode);
					
					if(repeatcount > 1)
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
			
			if (string)
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
			msg.Read(keystates,sizeof(int8)*16);
	
			STRACE(("Key Up: 0x%lx\n",scancode));
			
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
			
			if(FocusWinBorder())
			{
				ServerWindow *win = FocusWinBorder()->Window();
				if(win)
				{
					BMessage keymsg(B_KEY_UP);
					keymsg.AddInt64("when", time);
					keymsg.AddInt32("key", scancode);
					keymsg.AddInt32("modifiers", modifiers);
					keymsg.AddData("states", B_UINT8_TYPE, keystates, sizeof(int8) * 16);
					
					for(uint8 i = 0; i < 3; i++) {
						keymsg.AddInt8("byte", utf[i]);
					}
					
					keymsg.AddString("bytes", string);
					keymsg.AddInt32("raw_char", raw_char);
					
					win->SendMessageToClient(&keymsg, B_NULL_TOKEN, true);
				}
			}
			
			if (string)
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
			
			if(FocusWinBorder())
			{
				ServerWindow *win = FocusWinBorder()->Window();
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
			
			if(FocusWinBorder())
			{
				ServerWindow *win = FocusWinBorder()->Window();
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

			if(FocusWinBorder())
			{
				ServerWindow *win = FocusWinBorder()->Window();
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
RootLayer::SetEventMaskLayer(Layer *lay, uint32 mask, uint32 options)
{
	if (!lay)
		return false;

	bool	returnValue = true;

	Lock();

	if (fEventMaskLayer && fEventMaskLayer != lay)
	{
		fprintf(stderr, "WARNING: fEventMaskLayer already set and different than the required one!\n");
		returnValue = false;
	}
	else
	{
		fEventMaskLayer = lay;
		// TODO: use this mask and options!
	}
	
	Unlock();

	return returnValue;
}

void
RootLayer::SetDragMessage(BMessage* msg)
{
	if (fDragMessage)
	{
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

// DEBUG methods

void
RootLayer::PrintToStream()
{
	printf("\nRootLayer '%s' internals:\n", Name());
	printf("Screen list:\n");
	for(int32 i=0; i<fScreenPtrList.CountItems(); i++)
		printf("\t%ld\n", ((Screen*)fScreenPtrList.ItemAt(i))->ScreenNumber());

	printf("Screen rows: %ld\nScreen columns: %ld\n", fRows, fColumns);
	printf("Resolution for all Screens: %ldx%ldx%ld\n", fScreenWidth, fScreenHeight, fColorSpace);
	printf("Workspace list:\n");
	for(int32 i=0; i<fWsCount; i++)
	{
		printf("\t~~~Workspace: %ld\n", ((Workspace*)fWorkspace[i])->ID());
		((Workspace*)fWorkspace[i])->PrintToStream();
		printf("~~~~~~~~\n");
	}
}

// PRIVATE methods

void
RootLayer::show_winBorder(WinBorder *winBorder)
{
	bool		invalidate = false;
	bool		invalid;
	WinBorder	*exFocus = FocusWinBorder();
	WinBorder	*exActive = ActiveWinBorder();

	winBorder->Show(false);

	for (int32 i = 0; i < fWsCount; i++)
	{
		invalid		= false;

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
		}

		if (fActiveWksIndex == i) {
			invalidate = invalid;

			if (dynamic_cast<class WorkspacesLayer *>(winBorder->TopChild()) != NULL)
				SetWorkspacesLayer(winBorder->TopChild());
		}
	}

	if (invalidate)
		show_final_scene(exFocus, exActive);
}

void RootLayer::hide_winBorder(WinBorder *winBorder)
{
	bool		invalidate = false;
	bool		invalid;
	WinBorder	*exFocus = FocusWinBorder();
	WinBorder	*exActive = ActiveWinBorder();

	winBorder->Hide(false);

	for (int32 i = 0; i < fWsCount; i++)
	{
		invalid		= false;

		if (fWorkspace[i] && fWorkspace[i]->HasWinBorder(winBorder))
			invalid = fWorkspace[i]->HideWinBorder(winBorder);

		if (fActiveWksIndex == i) {
			invalidate = invalid;

			if (dynamic_cast<class WorkspacesLayer *>(winBorder->TopChild()) != NULL)
				SetWorkspacesLayer(NULL);
		}
	}

	if (invalidate)
		show_final_scene(exFocus, exActive);
}

void RootLayer::change_winBorder_feel(WinBorder *winBorder, int32 newFeel)
{
	bool	isVisible = false;
	bool	wasVisibleInActiveWorkspace = false;

	WinBorder	*exFocus	= FocusWinBorder();
	WinBorder	*exActive	= ActiveWinBorder();

	if (!winBorder->IsHidden())
	{
		isVisible = true;
		wasVisibleInActiveWorkspace = ActiveWorkspace()->HasWinBorder(winBorder);
		winBorder->Hide(false);
	}

	gDesktop->SetWinBorderFeel(winBorder, newFeel);

	if (isVisible)
	{
		if (fEventMaskLayer)
		{
// TODO: What was this supposed to do?!?
/*			WinBorder	*wb	= fEventMaskLayer->fOwner?
								fEventMaskLayer->fOwner:
								(WinBorder*)fEventMaskLayer;
			if (wb == fEventMaskLayer)
			{
				fMovingWindow	= false;
				fResizingWindow	= false;
				wb->MouseUp(DEC_NONE);
			}*/
			fEventMaskLayer	= NULL;
		}

		winBorder->Show(false);
		if (wasVisibleInActiveWorkspace || ActiveWorkspace()->HasWinBorder(winBorder))
			show_final_scene(exFocus, exActive);
	}
}

bool RootLayer::get_workspace_windows()
{
	int32	bufferSize = fWinBorderListLength;
	int32	exCount = fWinBorderCount;
	bool	present;
	bool	newMemory = false;
	bool	aChange = false;

	memcpy(fWinBorderList2, fWinBorderList, fWinBorderCount * sizeof(WinBorder*));

	if (!(ActiveWorkspace()->GetWinBorderList((void**)fWinBorderList, &bufferSize)))
	{
		newMemory		= true;
		// grow by a factor of 8.
		fWinBorderListLength = (bufferSize / 8 + 1) * 8;
		fWinBorderList	= (WinBorder**)realloc(fWinBorderList, fWinBorderListLength);
		ActiveWorkspace()->GetWinBorderList((void**)fWinBorderList, &bufferSize);
	}

	fWinBorderCount = bufferSize;
	fWinBorderIndex	= 0;

	// activate any workspaces layer in the current list

	SetWorkspacesLayer(NULL);

	for (int32 i = 0; i < fWinBorderCount; i++) {
		if (dynamic_cast<class WorkspacesLayer *>(fWinBorderList[i]->TopChild()) != NULL)
			SetWorkspacesLayer(fWinBorderList[i]->TopChild());
	}

	// to determine if there was a change in window hierarchy 
	if (exCount != fWinBorderCount || memcmp(fWinBorderList, fWinBorderList2, fWinBorderCount) != 0)
		aChange = true;

	if (aChange)
	{
		// clear visible and full visible regions for windows not visible anymore.
		for (int32 i = 0; i < exCount; i++)
		{
			present	= false;

			for (int32 j = 0; j < fWinBorderCount; j++)
				if (fWinBorderList2[i] == fWinBorderList[j])
					present = true;

			if (!present)
#ifndef NEW_CLIPPING
				empty_visible_regions(fWinBorderList2[i]);
#else
				fWinBorderList2[i]->clear_visible_regions();
#endif
		}
	}

	// enlarge 2nd buffer also
	if (newMemory)
		fWinBorderList2	= (WinBorder**)realloc(fWinBorderList2, fWinBorderListLength);

//for (int32 i = 0; i < fWinBorderCount; i++)
//{
//	printf("Adi: %ld get_workspace_windows(%p)\n", i, fWinBorderList[i]);
//}
//printf("Adi: get_workspace_windows DONE\n");
	return aChange;
}

void
RootLayer::draw_window_tab(WinBorder *exFocus)
{
	WinBorder *focus = FocusWinBorder();
	if (exFocus || focus) {
		if (exFocus && exFocus != focus && exFocus->fDecorator)
			exFocus->fDecorator->SetFocus(false);
		if (focus && exFocus != focus && focus->fDecorator)
			focus->fDecorator->SetFocus(true);

		if (exFocus && focus != exFocus) {
			// TODO: this line is a hack, decorator is drawn twice.
#ifndef NEW_CLIPPING
			BRegion		reg(exFocus->fVisible);
			if (focus)
				reg.Include(&focus->fVisible);
			redraw_layer(this, reg);
#endif
		}
	}
}

#ifndef NEW_CLIPPING
inline void
RootLayer::empty_visible_regions(Layer *layer)
{
// TODO: optimize by avoiding recursion?
	// NOTE: first 'layer' must be a WinBorder
	Layer	*child;


	layer->fFullVisible.MakeEmpty();
	layer->fVisible.MakeEmpty();
	child	= layer->BottomChild();
	while(child) {
		empty_visible_regions(child);
		child = layer->UpperSibling();
	}
}
#endif

inline void
RootLayer::winborder_activation(WinBorder* exActive)
{
	if (exActive && (FocusWinBorder() != exActive || FrontWinBorder() != exActive)) {
		BMessage msg(B_WINDOW_ACTIVATED);
		msg.AddBool("active", false);
		exActive->Window()->SendMessageToClient(&msg, B_NULL_TOKEN, false);
	}
	if (FocusWinBorder() == FrontWinBorder()
		&& FrontWinBorder() != NULL && FrontWinBorder() != exActive) {
		BMessage msg(B_WINDOW_ACTIVATED);
		msg.AddBool("active", true);
		FrontWinBorder()->Window()->SendMessageToClient(&msg, B_NULL_TOKEN, false);
	}
}

inline void
RootLayer::show_final_scene(WinBorder *exFocus, WinBorder *exActive)
{
	if (fHaveWinBorderList || get_workspace_windows()) {
		// next call should call get_workspace_windows()
		fHaveWinBorderList = false;
		// TODO: should it be improved by calling with region of hidden windows
		//       plus the full regions of new windows???
#ifndef NEW_CLIPPING
		invalidate_layer(this, fFull);
#else
		// TODO: hmm. Bounds()? Yes, this its OK, for the moment!!! think!
		do_Invalidate(Bounds());
#endif
	}

	draw_window_tab(exFocus);

	winborder_activation(exActive);

// TODO: MoveEventHandler::B_MOUSE_DOWN may not need this. Investigate.
// TODO: B_ENTERD_VIEW, B_EXITED_VIEW, B_INSIDE_VIEW, B_OUTSIDE_VIEW are sent only
// when the mouse is moved. What about when adding or removing a window that contains
// fLastMouseMoved, shouldn't the next fLastMouseMoved Layer get a B_MOUSE_MOVE at
// the same mouse position with B_ENTERD_VIEW??? Same when changing workspaces!!!
// INVESTIGATE, INVESTIGATE, INVESTIGATE!!!
// NOTE: the following 3 lines are here for safety reasons!
	fLastMouseMoved = LayerAt(fLastMousePosition);
	if (fLastMouseMoved == NULL) {
		CRITICAL("RootLayer::show_final_scene: 'fLastMouseMoved' can't be null.\n");
		fLastMouseMoved = this;
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
