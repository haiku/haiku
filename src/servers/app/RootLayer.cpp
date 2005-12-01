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
#include "WindowLayer.h"
#include "WorkspacePrivate.h"
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

//#define TRACE_ROOT_LAYER
#ifdef TRACE_ROOT_LAYER
#	define STRACE(a) printf a
#else
#	define STRACE(a) ;
#endif


RootLayer::RootLayer(const char *name, Desktop *desktop, DrawingEngine *driver)
	: Layer(BRect(0, 0, 0, 0), name, 0, B_FOLLOW_ALL, B_WILL_DRAW, driver),
	fDesktop(desktop),
	fDragMessage(NULL),
	fMouseEventWindow(NULL),
	fAllRegionsLock("root layer region lock"),

	fDirtyForRedraw(),

	fWorkspace(0),
	fWorkspacesLayer(NULL),
	
	fFocus(NULL),
	fFront(NULL),
	fBack(NULL)
{
	//NOTE: be careful about this one.
	fRootLayer = this;

// Some stuff that will go away in the future but fills some gaps right now
#if ON_SCREEN_DEBUGGING_INFO
	  fDebugInfo.SetTo("");
#endif

	fDrawState->SetHighColor(RGBColor(255, 255, 255));

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

	_UpdateWorkspace(fDesktop->WorkspaceAt(fDesktop->CurrentWorkspace()));

	TriggerRebuild();
	TriggerRedraw();
}


RootLayer::~RootLayer()
{
	delete fDragMessage;

	fFirstChild = NULL;
		// this prevents the Layer destructor from freeing our children again
		// (the Desktop destructor already took care of that)

	// RootLayer object just uses Screen objects, it is not allowed to delete them.

#if DISPLAY_HAIKU_LOGO
	delete fLogoBitmap;
#endif
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


bool
RootLayer::_SetFocus(WindowLayer* focus, BRegion& update)
{
	// TODO: test for FFM and B_LOCK_WINDOW_FOCUS

	if (focus == fFocus && (focus->WindowFlags() & B_AVOID_FOCUS) == 0)
		return true;

	// make sure no window is chosen that doesn't want focus
	while (focus != NULL && (focus->WindowFlags() & B_AVOID_FOCUS) != 0) {
		focus = (WindowLayer*)focus->PreviousLayer();
	}

	if (fFocus != NULL) {
		update.Include(&fFocus->VisibleRegion());
		fFocus->SetFocus(false);
	}

	fFocus = focus;

	if (focus != NULL) {
		update.Include(&focus->VisibleRegion());
		focus->SetFocus(true);
	}

	return true;
}


bool
RootLayer::SetFocus(WindowLayer* focus)
{
	BAutolock _(fAllRegionsLock);

	BRegion update;
	bool success = _SetFocus(focus, update);
	if (!success)
		return false;

	MarkForRedraw(update);
	TriggerRedraw();
	return true;
}


void
RootLayer::_SetFront(WindowLayer* windowLayer, BRegion& update)
{
	if (windowLayer == NULL) {
		fBack = NULL;
		fFront = NULL;
		return;
	}

	if (!windowLayer->SupportsFront() && fFront != NULL)
		return;

	BRegion previous = windowLayer->FullVisible();

	_RemoveChildFromList(windowLayer);
	_AddChildToList(windowLayer);

	windowLayer->GetOnScreenRegion(update);
	update.Exclude(&previous);

	fFront = windowLayer;

	if (windowLayer == fBack || fBack == NULL)
		_UpdateBack();
}


/*! search the visible windows for a valid back window
	(only normal windows can be back windows)
*/
void
RootLayer::_UpdateBack()
{
	fBack = NULL;

	for (WindowLayer* window = (WindowLayer*)FirstChild();
			window != NULL; window = (WindowLayer*)window->NextLayer()) {
		if (window->IsHidden() || !window->SupportsFront())
			continue;

		fBack = window;
		break;
	}
}


/*! search the visible windows for a valid front window
	(only normal windows can be front windows)
*/
void
RootLayer::_UpdateFront()
{
	// TODO: for now, just choose the top window
	fFront = NULL;

	for (WindowLayer* window = (WindowLayer*)LastChild();
			window != NULL; window = (WindowLayer*)window->PreviousLayer()) {
		if (window->IsHidden() || !window->SupportsFront())
			continue;

		fFront = window;
		break;
	}
}


void
RootLayer::_UpdateFronts()
{
	_UpdateBack();
	_UpdateFront();
}


void
RootLayer::ActivateWindow(WindowLayer* windowLayer)
{
	BAutolock _(fAllRegionsLock);

	BRegion changed;
	_SetFront(windowLayer, changed);

	// TODO: if this window has any floating windows, add them here

	MarkForRebuild(changed);
	TriggerRebuild();

	_SetFocus(windowLayer, changed);
	_WindowsChanged(changed);
	MarkForRedraw(changed);
	TriggerRedraw();
}


void
RootLayer::SendBehindWindow(WindowLayer* windowLayer, WindowLayer* behindOf)
{
	BAutolock _(fAllRegionsLock);

	if (windowLayer == Back())
		return;

	bool wasFocus = windowLayer == Focus();

	BRegion changed = windowLayer->FullVisible();

	_RemoveChildFromList(windowLayer);
	if (behindOf == NULL)
		behindOf = Back();

	_AddChildToList(windowLayer, behindOf);

	// TODO: if this window has any floating windows, remove them here

	MarkForRebuild(changed);
	TriggerRebuild();

	_UpdateFronts();
	_SetFocus(Front(), changed);

	changed.Exclude(&windowLayer->FullVisible());
	if (wasFocus != (Focus() == windowLayer))
		changed.Include(&windowLayer->VisibleRegion());

	_WindowsChanged(changed);
	MarkForRedraw(changed);
	TriggerRedraw();
}


void
RootLayer::AddWindowLayer(WindowLayer* windowLayer)
{
	STRACE(("AddWindowLayer(%p \"%s\")\n", windowLayer, windowLayer->Name()));

	if (windowLayer->SupportsFront())
		_AddChildToList(windowLayer);
	else
		_AddChildToList(windowLayer, Back());

	windowLayer->SetRootLayer(this);

	if (!windowLayer->IsHidden())
		ActivateWindow(windowLayer);
}


void
RootLayer::RemoveWindowLayer(WindowLayer* windowLayer)
{
	if (!windowLayer->IsHidden())
		HideWindowLayer(windowLayer);

	_RemoveChildFromList(windowLayer);

	LayerRemoved(windowLayer);
	windowLayer->SetRootLayer(NULL);
}


void
RootLayer::_UpdateWorkspace(Workspace::Private& workspace)
{
	fColor = workspace.Color();

#if ON_SCREEN_DEBUGGING_INFO
	fDrawState->SetLowColor(fColor);
#endif
}


void
RootLayer::SetWorkspace(int32 index, Workspace::Private& previousWorkspace,
	Workspace::Private& workspace)
{
	BAutolock _(fAllRegionsLock);

	int32 previousIndex = fWorkspace;

	// build region of windows that are no longer visible in the new workspace
	// and move windows to the previous workspace's window list

	BRegion changed;
	while (WindowLayer* window = (WindowLayer*)FirstChild()) {
		// move the window into the workspace's window list
		_RemoveChildFromList(window);

		BPoint position = window->Frame().LeftTop();
		previousWorkspace.AddWindow(window, &position);

		if (window->IsHidden())
			continue;

		if (!window->OnWorkspace(index)) {
			// this window will no longer be visible
			changed.Include(&window->FullVisible());
		}
	}

	fWorkspace = index;
	_UpdateWorkspace(workspace);

	// add new windows, and include them in the changed region - but only
	// those that were not visible before (or whose position changed)

	while (workspace.CountWindows() != 0) {
		window_layer_info* info = workspace.WindowAt(0);
		WindowLayer* window = info->window;
		BPoint position = info->position;

		// move window into the RootLayer's list
		workspace.RemoveWindowAt(0);
		_AddChildToList(window);

		if (window->IsHidden())
			continue;

		if (position == kInvalidWindowPosition) {
			// if you enter a workspace for the first time, the position
			// of the window in the previous workspace is adopted
			position = window->Frame().LeftTop();
				// TODO: make sure the window is still on-screen if it
				//	was before!
		}

		if (!window->OnWorkspace(previousIndex)) {
			// this window was not visible before

			// TODO: what we really want here is the visible region of the window
			BRegion region;
			window->GetOnScreenRegion(region);

			changed.Include(&region);
		} else if (window->Frame().LeftTop() != position) {
			// the window was visible before, but its on-screen location changed
			BPoint offset = position - window->Frame().LeftTop();
			window->MoveBy(offset.x, offset.y);

			// TODO: we're playing dumb here - what we need is a MoveBy() that
			//	gives us a dirty region back, and since the new clipping code
			//	will give us just that, we don't take any special measurement
			//	here
			GetOnScreenRegion(changed);
		} else {
			// the window is still visible and on the same location
			continue;
		}
	}

	_UpdateFronts();
	_SetFocus(Front(), changed);

	MarkForRebuild(changed);
	TriggerRebuild();

	_WindowsChanged(changed);
	MarkForRedraw(changed);
	TriggerRedraw();
}


void
RootLayer::HideWindowLayer(WindowLayer* windowLayer)
{
	BAutolock _(fAllRegionsLock);

	BRegion changed = windowLayer->FullVisible();
	windowLayer->Hide();
	_UpdateFronts();

	if (dynamic_cast<WorkspacesLayer*>(windowLayer->TopLayer()) != NULL)
		fWorkspacesLayer = NULL;

	// TODO: if this window has any floating windows, remove them here

	MarkForRebuild(changed);
	TriggerRebuild();

	if (windowLayer == Focus())
		_SetFocus(Front(), changed);
	_WindowsChanged(changed);

	MarkForRedraw(changed);
	TriggerRedraw();
}


void
RootLayer::ShowWindowLayer(WindowLayer* windowLayer, bool toFront)
{
	STRACE(("ShowWindowLayer(%p)\n", windowLayer));

	if (!windowLayer->IsHidden())
		return;

	BAutolock _(fAllRegionsLock);

	windowLayer->Show();
	BRegion changed = windowLayer->FullVisible();

	if (toFront)
		_SetFront(windowLayer, changed);
	else
		_UpdateFront();

	// TODO: if this window has any floating windows, remove them here

	// TODO: support FFM
	if (Front() == windowLayer || Focus() == NULL)
		_SetFocus(windowLayer, changed);
	_WindowsChanged(changed);
	MarkForRedraw(changed);
	TriggerRedraw();

	if (dynamic_cast<WorkspacesLayer*>(windowLayer->TopLayer()) != NULL)
		fWorkspacesLayer = windowLayer->TopLayer();
}


void
RootLayer::MoveWindowBy(WindowLayer* window, float x, float y)
{
	BAutolock _(fAllRegionsLock);

	// TODO: the MoveBy() should just return a dirty region
	window->MoveBy(x, y);
	if (window->Parent() == NULL)
		return;

	BRegion changed;
	_WindowsChanged(changed);

	if (changed.CountRects() > 0) {
		MarkForRedraw(changed);
		TriggerRedraw();
	}
}


void
RootLayer::ResizeWindowBy(WindowLayer* window, float x, float y)
{
	BAutolock _(fAllRegionsLock);

	// TODO: the MoveBy() should just return a dirty region
	window->ResizeBy(x, y);
	if (window->Parent() == NULL)
		return;

	BRegion changed;
	_WindowsChanged(changed);

	if (changed.CountRects() > 0) {
		MarkForRedraw(changed);
		TriggerRedraw();
	}
}


void
RootLayer::SetWindowLayerFeel(WindowLayer *windowLayer, int32 newFeel)
{
	BAutolock _(fAllRegionsLock);

	// TODO
}


void
RootLayer::SetWindowLayerLook(WindowLayer *windowLayer, int32 newLook)
{
	BAutolock _(fAllRegionsLock);

	// TODO
}


void
RootLayer::_WindowsChanged(BRegion& region)
{
	if (fWorkspacesLayer == NULL)
		return;

	region.Include(&fWorkspacesLayer->VisibleRegion());
}


void
RootLayer::UpdateWorkspaces()
{
	BAutolock _(fAllRegionsLock);

	if (fWorkspacesLayer == NULL)
		return;

	BRegion changed;
	_WindowsChanged(changed);

	MarkForRedraw(changed);
	TriggerRedraw();
}


WindowLayer*
RootLayer::WindowAt(BPoint where)
{
	if (VisibleRegion().Contains(where))
		return NULL;

	for (Layer* child = LastChild(); child; child = child->PreviousLayer()) {
		if (child->FullVisible().Contains(where))
			return dynamic_cast<WindowLayer*>(child);
	}

	return NULL;
}


void
RootLayer::SetMouseEventWindow(WindowLayer* window)
{
	fMouseEventWindow = window;
}


void
RootLayer::LayerRemoved(Layer* layer)
{
	if (fMouseEventWindow == layer)
		fMouseEventWindow = NULL;
	if (fWorkspacesLayer == layer)
		fWorkspacesLayer = NULL;
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
RootLayer::DragMessage() const
{
	return fDragMessage;
}


void
RootLayer::Draw(const BRect& rect)
{
	fDriver->FillRect(rect, fColor);

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

#if 0
	// for debugging
	GetDrawingEngine()->ConstrainClippingRegion(&fDirtyForRedraw);
	RGBColor color(rand()%255, rand()%255, rand()%255);
	GetDrawingEngine()->FillRect(Bounds(), color);
	snooze(200000);
	GetDrawingEngine()->ConstrainClippingRegion(NULL);
#endif
	_AllRedraw(fDirtyForRedraw);

	fDirtyForRedraw.MakeEmpty();
}

