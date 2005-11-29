
#include <new>
#include <stdio.h>

#include <Message.h>
#include <MessageQueue.h>

#include "ClientLooper.h"
#include "Desktop.h"
#include "DrawingEngine.h"

#include "WindowLayer.h"

// if the background clearing is delayed until
// the client draws the view, we have less flickering
// when contents have to be redrawn because of resizing
// a window or because the client invalidates parts.
// when redrawing something that has been exposed from underneath
// other windows, the other window will be seen longer at
// its previous position though if the exposed parts are not
// cleared right away. maybe there ought to be a flag in
// the update session, which tells us the cause of the update
#define DELAYED_BACKGROUND_CLEARING 1

// constructor
WindowLayer::WindowLayer(BRect frame, const char* name,
						 DrawingEngine* drawingEngine, Desktop* desktop)
	: BLooper(name, B_DISPLAY_PRIORITY),
	  fFrame(frame),

	  fVisibleRegion(),
	  fVisibleContentRegion(),
	  fDirtyRegion(),

	  fBorderRegion(),
	  fBorderRegionValid(false),
	  fContentRegion(),
	  fContentRegionValid(false),
	  fEffectiveDrawingRegion(),
	  fEffectiveDrawingRegionValid(false),

	  fFocus(false),

	  fTopLayer(NULL),

// TODO: windows must start hidden!
	  fHidden(false),
	  // windows start hidden
//	  fHidden(true),

	  fDrawingEngine(drawingEngine),
	  fDesktop(desktop),

	  fTokenViewMap(64),

	  fClient(new ClientLooper(name, this)),
	  fCurrentUpdateSession(NULL),
	  fPendingUpdateSession(NULL),
	  fUpdateRequested(false),
	  fInUpdate(false)
{
	// the top layer is special, it has a coordinate system
	// as if it was attached directly to the desktop, therefor,
	// the coordinate conversion through the layer tree works
	// as expected, since the top layer has no "parent" but has
	// fFrame as if it had
	fTopLayer = new(nothrow) ViewLayer(fFrame, "top view", B_FOLLOW_ALL, 0,
									   (rgb_color){ 255, 255, 255, 255 });
	fTopLayer->AttachedToWindow(this);

	fClient->Run();
}

// destructor
WindowLayer::~WindowLayer()
{
	delete fTopLayer;
}

// MessageReceived
void
WindowLayer::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_REDRAW: {
			// there is only one MSG_REDRAW in the queue at anytime
			if (fDesktop->ReadLockClipping()) {

				_DrawBorder();
				_TriggerContentRedraw();

				// reset the dirty region, since
				// we're fully clean. If the desktop
				// thread wanted to mark something
				// dirty in the mean time, it was
				// blocking on the global region lock to
				// get write access, since we held the
				// read lock for the whole time.
				fDirtyRegion.MakeEmpty();

				fDesktop->ReadUnlockClipping();
			} else {
//printf("%s MSG_REDRAW -> pending redraws\n", Name());
			}
			break;
		}
		case MSG_BEGIN_UPDATE:
			_BeginUpdate();
			break;
		case MSG_END_UPDATE:
			_EndUpdate();
			break;
		case MSG_DRAWING_COMMAND: {
			int32 token;
			if (message->FindInt32("token", &token) >= B_OK)
				_DrawClient(token);
			break;
		}
		default:
			BLooper::MessageReceived(message);
			break;
	}
}

// SetClipping
void
WindowLayer::SetClipping(BRegion* stillAvailableOnScreen)
{
	// this function is only called from the Desktop thread

	// start from full region (as if the window was fully visible)
	GetFullRegion(&fVisibleRegion);
	// clip to region still available on screen
	fVisibleRegion.IntersectWith(stillAvailableOnScreen);

	GetContentRegion(&fVisibleContentRegion);
	fVisibleContentRegion.IntersectWith(&fVisibleRegion);

	fEffectiveDrawingRegionValid = false;
	fTopLayer->InvalidateScreenClipping(true);
}

// GetFullRegion
void
WindowLayer::GetFullRegion(BRegion* region) const
{
	// start from the frame, extend to include decorator border
	region->Set(BRect(fFrame.left - 4, fFrame.top - 4,
					  fFrame.right + 4, fFrame.bottom + 4));
	// add the title tab
	region->Include(BRect(fFrame.left - 4, fFrame.top - 20,
						  (fFrame.left + fFrame.right) / 2, fFrame.top - 5));
}

// GetBorderRegion
void
WindowLayer::GetBorderRegion(BRegion* region)
{
	if (!fBorderRegionValid) {
		// TODO: speed up by avoiding "Exclude()"
		// start from the frame, extend to include decorator border
		fBorderRegion.Set(BRect(fFrame.left - 4, fFrame.top - 4,
						  fFrame.right + 4, fFrame.bottom + 4));
	
		fBorderRegion.Exclude(fFrame);
	
		// add the title tab
		fBorderRegion.Include(BRect(fFrame.left - 4, fFrame.top - 20,
							  (fFrame.left + fFrame.right) / 2, fFrame.top - 5));
	
		// resize handle
		// if (B_DOCUMENT_WINDOW_LOOK)
			fBorderRegion.Include(BRect(fFrame.right - 10, fFrame.bottom - 10,
								  fFrame.right, fFrame.bottom));
		fBorderRegionValid = true;
	}

	*region = fBorderRegion;
}

// GetContentRegion
void
WindowLayer::GetContentRegion(BRegion* region)
{
	if (!fContentRegionValid) {
		// TODO: speed up by avoiding "Exclude()"
		// start from the frame, extend to include decorator border
		fContentRegion.Set(fFrame);
	
		// resize handle
		// if (B_DOCUMENT_WINDOW_LOOK)
			fContentRegion.Exclude(BRect(fFrame.right - 10, fFrame.bottom - 10,
										 fFrame.right, fFrame.bottom));

		fContentRegionValid = true;
	}

	*region = fContentRegion;
}

// SetFocus
void
WindowLayer::SetFocus(bool focus)
{
	// executed from Desktop thread
	// it holds the clipping write lock,
	// so the window thread cannot be
	// accessing fFocus

	BRegion dirty(fBorderRegion);
	dirty.IntersectWith(&fVisibleRegion);
	fDesktop->MarkDirty(&dirty);

	fFocus = focus;
}

// MoveBy
void
WindowLayer::MoveBy(int32 x, int32 y)
{
	// this function is only called from the desktop thread
	// TODO: this fact needs review maybe

	if (x == 0 && y == 0)
		return;

	fFrame.OffsetBy(x, y);

	// take along the dirty region which have not
	// processed yet
	fDirtyRegion.OffsetBy(x, y);

	if (fBorderRegionValid)
		fBorderRegion.OffsetBy(x, y);
	if (fContentRegionValid)
		fContentRegion.OffsetBy(x, y);

	if (fCurrentUpdateSession)
		fCurrentUpdateSession->MoveBy(x, y);
	if (fPendingUpdateSession)
		fPendingUpdateSession->MoveBy(x, y);

	fEffectiveDrawingRegionValid = false;

	fTopLayer->MoveBy(x, y, NULL);

	// the desktop will take care of dirty regions
}

// ResizeBy
void
WindowLayer::ResizeBy(int32 x, int32 y, BRegion* dirtyRegion)
{
	// this function is only called from the desktop thread
	// TODO: this fact needs review maybe

	if (x == 0 && y == 0)
		return;

	fFrame.right += x;
	fFrame.bottom += y;

	// put the previous border region into the dirty region as well
	// to handle the part that was overlapping a layer
	dirtyRegion->Include(&fBorderRegion);

	fBorderRegionValid = false;
	fContentRegionValid = false;
	fEffectiveDrawingRegionValid = false;

	// the border is dirty, put it into
	// dirtyRegion for a start
	BRegion newBorderRegion;
	GetBorderRegion(&newBorderRegion);
	dirtyRegion->Include(&newBorderRegion);

	// TODO: should this be clipped to the content region?
	fTopLayer->ResizeBy(x, y, dirtyRegion);
}

// AddChild
void
WindowLayer::AddChild(ViewLayer* layer)
{
	fTopLayer->AddChild(layer);

	// inform client about the view
	// (just a part of the simulation)
	fTokenViewMap.MakeEmpty();
	fTopLayer->CollectTokensForChildren(&fTokenViewMap);
	BMessage message(MSG_VIEWS_ADDED);
	message.AddInt32("count", fTokenViewMap.CountItems());
	fClient->PostMessage(&message);

	// TODO: trigger redraw for dirty regions
}

void
WindowLayer::Hide()
{
	if (fHidden)
		return;

	fHidden = true;

	// TODO: notify window manager
	// TODO: call RevealNewWMState
}

void
WindowLayer::Show()
{
	if (!fHidden)
		return;

	fHidden = false;

	// TODO: notify window manager
	// TODO: call RevealNewWMState

}

// ProcessDirtyRegion
void
WindowLayer::ProcessDirtyRegion(BRegion* region)
{
	// this is exectuted in the desktop thread,
	// and it means that the window thread currently
	// blocks on the global region lock
	if (fDirtyRegion.CountRects() == 0) {
		// the window needs to be informed
		// when the dirty region was empty
		PostMessage(MSG_REDRAW, this);
	}
	// this is executed from the desktop thread
	fDirtyRegion.Include(region);
}

// MarkDirty
void
WindowLayer::MarkDirty(BRegion* regionOnScreen)
{
	// for triggering MSG_REDRAW
	if (fDesktop)
		fDesktop->MarkDirty(regionOnScreen);
}

// MarkContentDirty
void
WindowLayer::MarkContentDirty(BRegion* regionOnScreen)
{
/*	// for triggering MSG_REDRAW
	if (fDesktop && fDesktop->LockClipping()) {

		regionOnScreen->IntersectWith(&fVisibleContentRegion);
		fDesktop->MarkDirty(regionOnScreen);

		fDesktop->UnlockClipping();
	}*/
}

//# pragma mark -

// CopyContents
void
WindowLayer::CopyContents(BRegion* region, int32 xOffset, int32 yOffset)
{
	// this function takes care of invalidating parts that could not be copied

	if (fDesktop->LockClipping()) {

		BRegion newDirty(*region);

		// clip the region to the visible contents at the
		// source and destination location
		region->IntersectWith(&fVisibleContentRegion);
		if (region->CountRects() > 0) {
			region->OffsetBy(xOffset, yOffset);
			region->IntersectWith(&fVisibleContentRegion);
			if (region->CountRects() > 0) {
				// if the region still contains any rects
				// offset to source location again
				region->OffsetBy(-xOffset, -yOffset);
				// the part which we can copy is not dirty
				newDirty.Exclude(region);
		
				if (fDrawingEngine->Lock()) {
					fDrawingEngine->CopyRegion(region, xOffset, yOffset);
					fDrawingEngine->Unlock();
				}

				// move along the already dirty regions that are common
				// with the region that we could copy
				_ShiftPartOfRegion(&fDirtyRegion, region, xOffset, yOffset);
				if (fCurrentUpdateSession)
					_ShiftPartOfRegion(&fCurrentUpdateSession->DirtyRegion(), region, xOffset, yOffset);
				if (fPendingUpdateSession)
					_ShiftPartOfRegion(&fPendingUpdateSession->DirtyRegion(), region, xOffset, yOffset);
		
			}
		}
		// what is left visible from the original region
		// at the destination after the region which could be
		// copied has been excluded, is considered dirty
		newDirty.OffsetBy(xOffset, yOffset);
		newDirty.IntersectWith(&fVisibleContentRegion);
		if (newDirty.CountRects() > 0)
			fDesktop->MarkDirty(&newDirty);

		fDesktop->UnlockClipping();
	}
}

// #pragma mark -

// _ShiftPartOfRegion
void
WindowLayer::_ShiftPartOfRegion(BRegion* region, BRegion* regionToShift,
								int32 xOffset, int32 yOffset)
{
	BRegion common(*regionToShift);
	// see if there is a common part at all
	common.IntersectWith(region);
	if (common.CountRects() > 0) {
		// cut the common part from the region,
		// offset that to destination and include again
		region->Exclude(&common);
		common.OffsetBy(xOffset, yOffset);
		region->Include(&common);
	}
}

// _TriggerContentRedraw
void
WindowLayer::_TriggerContentRedraw()
{
//printf("%s - DrawContents()\n", Name());
	BRegion dirtyContentRegion(fVisibleContentRegion);
	dirtyContentRegion.IntersectWith(&fDirtyRegion);

	if (dirtyContentRegion.CountRects() > 0) {

#if SHOW_WINDOW_CONTENT_DIRTY_REGION
if (fDrawingEngine->Lock()) {
fDrawingEngine->SetHighColor(0, 0, 255);
fDrawingEngine->FillRegion(&dirtyContentRegion);
fDrawingEngine->MarkDirty(&dirtyContentRegion);
fDrawingEngine->Unlock();
snooze(100000);
}
#endif
		// send UPDATE message to the client
		_MarkContentDirty(&dirtyContentRegion);

#if DELAYED_BACKGROUND_CLEARING
		fTopLayer->Draw(fDrawingEngine, &dirtyContentRegion,
						&fVisibleContentRegion, false);
#else
		fTopLayer->Draw(fDrawingEngine, &dirtyContentRegion,
						&fVisibleContentRegion, true);
#endif
	}
}

// _DrawClient
void
WindowLayer::_DrawClient(int32 token)
{
	// This function is only executed in the window thread.
	// It still needs to block on the clipping lock, since
	// We have to be sure that the clipping is up to date.
	// If true readlocking would work correctly, this would
	// not be an issue
	ViewLayer* layer = (ViewLayer*)fTokenViewMap.ItemAt(token);
	if (!layer)
		return;

	if (fDesktop->ReadLockClipping()) {

		if (!fEffectiveDrawingRegionValid) {
			fEffectiveDrawingRegion = fVisibleContentRegion;
			if (fInUpdate) {
				// enforce the dirty region of the update session
				fEffectiveDrawingRegion.IntersectWith(&fCurrentUpdateSession->DirtyRegion());
			} else {
				printf("%s - _DrawClient(token: %ld) - not in update\n", Name(), token);
			}
			fEffectiveDrawingRegionValid = true;
		}

		// TODO: this is a region that needs to be cached later in the server
		// when the current layer in ServerWindow is set, and we are currently
		// in an update (fInUpdate), than we can set this region and remember
		// it for the comming drawing commands until the current layer changes
		// again or fEffectiveDrawingRegionValid is suddenly false.
		BRegion effectiveClipping(fEffectiveDrawingRegion);
		effectiveClipping.IntersectWith(&layer->ScreenClipping(&fVisibleContentRegion));

		if (effectiveClipping.CountRects() > 0) {
#if DELAYED_BACKGROUND_CLEARING
			// clear the back ground
			// TODO: only if this is the first drawing command for
			// this layer of course! in the simulation, all client
			// drawing is done from a single command yet
			layer->Draw(fDrawingEngine, &effectiveClipping,
						&fVisibleContentRegion, false);
#endif

			layer->ClientDraw(fDrawingEngine, &effectiveClipping);
		}

		fDesktop->ReadUnlockClipping();
	}
}

// _DrawBorder
void
WindowLayer::_DrawBorder()
{
	// this is executed in the window thread, but only
	// in respond to MSG_REDRAW having been received, the
	// clipping lock is held for reading

	// construct the region of the border that needs redrawing
	BRegion dirtyBorderRegion;
	GetBorderRegion(&dirtyBorderRegion);
// TODO: why is it not enough to only intersect with the dirty region?
// is it faster to intersect the dirty region with the visible when it
// is set in ProcessDirtyRegion()?
	// intersect with our visible region
	dirtyBorderRegion.IntersectWith(&fVisibleRegion);
	// intersect with the dirty region
	dirtyBorderRegion.IntersectWith(&fDirtyRegion);

	if (dirtyBorderRegion.CountRects() > 0) {
		if (fDrawingEngine->Lock()) {

			fDrawingEngine->ConstrainClippingRegion(&dirtyBorderRegion);

			if (fFocus) {
				fDrawingEngine->SetLowColor(255, 203, 0, 255);
				fDrawingEngine->SetHighColor(0, 0, 0, 255);
			} else {
				fDrawingEngine->SetLowColor(216, 216, 216, 0);
				fDrawingEngine->SetHighColor(30, 30, 30, 255);
			}
			rgb_color light = tint_color(fDrawingEngine->LowColor(), B_LIGHTEN_2_TINT);
			rgb_color shadow = tint_color(fDrawingEngine->LowColor(), B_DARKEN_2_TINT);

			fDrawingEngine->FillRect(dirtyBorderRegion.Frame(), B_SOLID_LOW);

			fDrawingEngine->DrawString(Name(), BPoint(fFrame.left, fFrame.top - 5));

			BRect frame(fFrame);
			fDrawingEngine->BeginLineArray(12);
			frame.InsetBy(-1, -1);
			fDrawingEngine->AddLine(BPoint(frame.left, frame.bottom),
									BPoint(frame.left, frame.top), shadow);
			fDrawingEngine->AddLine(BPoint(frame.left + 1, frame.top),
									BPoint(frame.right, frame.top), shadow);
			fDrawingEngine->AddLine(BPoint(frame.right, frame.top + 1),
									BPoint(frame.right, frame.bottom - 11), light);
			fDrawingEngine->AddLine(BPoint(frame.right - 1, frame.bottom - 11),
									BPoint(frame.right - 11, frame.bottom - 11), light);
			fDrawingEngine->AddLine(BPoint(frame.right - 11, frame.bottom - 10),
									BPoint(frame.right - 11, frame.bottom), light);
			fDrawingEngine->AddLine(BPoint(frame.right - 12, frame.bottom),
									BPoint(frame.left + 1, frame.bottom), light);

			frame.InsetBy(-3, -3);
			fDrawingEngine->AddLine(BPoint(frame.left, frame.bottom),
									BPoint(frame.left, frame.top - 16), light);
			fDrawingEngine->AddLine(BPoint(frame.left + 1, frame.top - 16),
									BPoint((frame.left + frame.right) / 2, frame.top - 16), light);
			fDrawingEngine->AddLine(BPoint((frame.left + frame.right) / 2, frame.top - 15),
									BPoint((frame.left + frame.right) / 2, frame.top), shadow);
			fDrawingEngine->AddLine(BPoint((frame.left + frame.right) / 2 + 1, frame.top),
									BPoint(frame.left + frame.right, frame.top), light);
			fDrawingEngine->AddLine(BPoint(frame.right, frame.top + 1),
									BPoint(frame.right, frame.bottom), shadow);
			fDrawingEngine->AddLine(BPoint(frame.right, frame.bottom),
									BPoint(frame.left + 1, frame.bottom), shadow);
			fDrawingEngine->EndLineArray();

			fDrawingEngine->ConstrainClippingRegion(NULL);
			fDrawingEngine->MarkDirty(&dirtyBorderRegion);
			fDrawingEngine->Unlock();
		}
	}
}

// _MarkContentDirty
//
// pre: the clipping is readlocked (this function is
// only called from _TriggerContentRedraw()), which
// in turn is only called from MessageReceived() with
// the clipping lock held
void
WindowLayer::_MarkContentDirty(BRegion* contentDirtyRegion)
{
	if (contentDirtyRegion->CountRects() <= 0)
		return;

	if (!fPendingUpdateSession) {
		// create new pending
		fPendingUpdateSession = new UpdateSession(*contentDirtyRegion);
	} else {
		// add to pending
		fPendingUpdateSession->Include(contentDirtyRegion);
	}

	// clip pending update session from current
	// update session, it makes no sense to draw stuff
	// already needing a redraw anyways. Theoretically,
	// this could be done smarter (clip layers from pending
	// that have not yet been redrawn in the current update
	// session)
	if (fCurrentUpdateSession) {
		fCurrentUpdateSession->Exclude(contentDirtyRegion);
		fEffectiveDrawingRegionValid = false;
	}

	if (!fUpdateRequested) {
		// send this to client
		fClient->PostMessage(MSG_UPDATE);
		fUpdateRequested = true;
		// as long as we have not received
		// the "begin update" command, the
		// pending session does not become the
		// current
	}
}

// _BeginUpdate
void
WindowLayer::_BeginUpdate()
{
	// TODO: since we might "shift" parts of the
	// internal dirty regions from the desktop thread
	// in respond to WindowLayer::ResizeBy(), which
	// might move arround views, this function needs to block
	// on the global clipping lock so that the internal
	// dirty regions are not messed with from both threads
	// at the same time.
	if (fDesktop->ReadLockClipping()) {

		if (fUpdateRequested && !fCurrentUpdateSession) {
			fCurrentUpdateSession = fPendingUpdateSession;
			fPendingUpdateSession = NULL;
	
			if (fCurrentUpdateSession) {
				// all drawing command from the client
				// will have the dirty region from the update
				// session enforced
				fInUpdate = true;
			}
			fEffectiveDrawingRegionValid = false;
		}

	fDesktop->ReadUnlockClipping();
	}
}

// _EndUpdate
void
WindowLayer::_EndUpdate()
{
	// TODO: see comment in _BeginUpdate()
	if (fDesktop->ReadLockClipping()) {
	
		if (fInUpdate) {
			delete fCurrentUpdateSession;
			fCurrentUpdateSession = NULL;
		
			fInUpdate = false;
			fEffectiveDrawingRegionValid = false;
		}
		if (fPendingUpdateSession) {
			// send this to client
			fClient->PostMessage(MSG_UPDATE);
			fUpdateRequested = true;
		} else {
			fUpdateRequested = false;
		}
	
	fDesktop->ReadUnlockClipping();
	}
}

// #pragma mark -

// constructor
UpdateSession::UpdateSession(const BRegion& dirtyRegion)
	: fDirtyRegion(dirtyRegion)
{
}

// destructor
UpdateSession::~UpdateSession()
{
}

// Include
void
UpdateSession::Include(BRegion* additionalDirty)
{
	fDirtyRegion.Include(additionalDirty);
}

// Exclude
void
UpdateSession::Exclude(BRegion* dirtyInNextSession)
{
	fDirtyRegion.Exclude(dirtyInNextSession);
}

// MoveBy
void
UpdateSession::MoveBy(int32 x, int32 y)
{
	fDirtyRegion.OffsetBy(x, y);
}




