
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

// IMPORTANT: nested ReadLockClipping()s are not supported (by MultiLocker)


// constructor
WindowLayer::WindowLayer(BRect frame, const char* name,
						 DrawingEngine* drawingEngine, Desktop* desktop)
	: BLooper(name, B_DISPLAY_PRIORITY),
	  fFrame(frame),

	  fVisibleRegion(),
	  fVisibleContentRegion(),
	  fVisibleContentRegionValid(false),
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
	  fCurrentUpdateSession(),
	  fPendingUpdateSession(),
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
		case MSG_DRAW_POLYGON: {
			int32 token;
			BPoint polygon[4];
			if (message->FindInt32("token", &token) >= B_OK &&
				message->FindPoint("point", 0, &polygon[0]) >= B_OK &&
				message->FindPoint("point", 1, &polygon[1]) >= B_OK &&
				message->FindPoint("point", 2, &polygon[2]) >= B_OK &&
				message->FindPoint("point", 3, &polygon[3]) >= B_OK) {

				_DrawClientPolygon(token, polygon);
			}
			break;
			
		}

		case MSG_INVALIDATE_VIEW: {
			int32 token;
			if (message->FindInt32("token", &token) >= B_OK)
				InvalidateView(token);
			break;
		}

		case MSG_SHOW:
			if (IsHidden()) {
				fDesktop->ShowWindow(this);
			}
			break;
		default:
			BLooper::MessageReceived(message);
			break;
	}
}

// QuitRequested
bool
WindowLayer::QuitRequested()
{
	if (fDesktop && fDesktop->LockClipping()) {
		fDesktop->WindowDied(this);

		fClient->Lock();
		fClient->Quit();

		fDesktop->UnlockClipping();
	}
	return true;
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

	fVisibleContentRegionValid = false;
	fEffectiveDrawingRegionValid = false;
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
						  ceilf((fFrame.left + fFrame.right) / 2), fFrame.top - 5));
}

// GetBorderRegion
void
WindowLayer::GetBorderRegion(BRegion* region)
{
	if (!fBorderRegionValid) {
		fBorderRegion.Set(BRect(fFrame.left - 4, fFrame.top - 20,
							  	ceilf((fFrame.left + fFrame.right) / 2), fFrame.top - 5));
		fBorderRegion.Include(BRect(fFrame.left - 4, fFrame.top - 4,
									fFrame.right + 4, fFrame.top - 1));
		fBorderRegion.Include(BRect(fFrame.left - 4, fFrame.top,
									fFrame.left - 1, fFrame.bottom));
		fBorderRegion.Include(BRect(fFrame.right + 1, fFrame.top,
									fFrame.right + 4, fFrame.bottom - 11));
		fBorderRegion.Include(BRect(fFrame.left - 4, fFrame.bottom + 1,
									fFrame.right - 11, fFrame.bottom + 4));
		fBorderRegion.Include(BRect(fFrame.right - 10, fFrame.bottom - 10,
									fFrame.right + 4, fFrame.bottom + 4));
		fBorderRegionValid = true;
	}

	*region = fBorderRegion;
}

// GetContentRegion
void
WindowLayer::GetContentRegion(BRegion* region)
{
	if (!fContentRegionValid) {
		_UpdateContentRegion();
	}

	*region = fContentRegion;
}

// VisibleContentRegion
BRegion&
WindowLayer::VisibleContentRegion()
{
	// regions expected to be locked
	if (!fVisibleContentRegionValid) {
		GetContentRegion(&fVisibleContentRegion);
		fVisibleContentRegion.IntersectWith(&fVisibleRegion);
	}
	return fVisibleContentRegion;
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

	if (fCurrentUpdateSession.IsUsed())
		fCurrentUpdateSession.MoveBy(x, y);
	if (fPendingUpdateSession.IsUsed())
		fPendingUpdateSession.MoveBy(x, y);

	fEffectiveDrawingRegionValid = false;

	fTopLayer->MoveBy(x, y, NULL);

	// the desktop will take care of dirty regions
}

// ResizeBy
void
WindowLayer::ResizeBy(int32 x, int32 y, BRegion* dirtyRegion)
{
	// this function is only called from the desktop thread

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

	fTopLayer->ResizeBy(x, y, dirtyRegion);
}

// ScrollViewBy
void
WindowLayer::ScrollViewBy(ViewLayer* view, int32 dx, int32 dy)
{
	// this can be executed from any thread, but if the
	// desktop thread is executing this, it should have
	// the write lock, otherwise it is not prevented
	// from executing this at the same time as the window
	// is doing something else here!

	if (!view || view == fTopLayer || (dx == 0 && dy == 0))
		return;

	if (fDesktop && fDesktop->ReadLockClipping()) {

		BRegion dirty;
		view->ScrollBy(dx, dy, &dirty);

		_MarkContentDirty(&dirty);

		fDesktop->ReadUnlockClipping();
	}
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

// ViewAt
ViewLayer*
WindowLayer::ViewAt(const BPoint& where)
{
	if (!fContentRegionValid)
		_UpdateContentRegion();

	return fTopLayer->ViewAt(where, &fContentRegion);
}

// SetHidden
void
WindowLayer::SetHidden(bool hidden)
{
	// the desktop takes care of dirty regions
	if (fHidden != hidden) {
		fHidden = hidden;

		fTopLayer->SetHidden(hidden);

		// this is only for simulation purposes:
		if (fHidden)
			fClient->PostMessage(MSG_WINDOW_HIDDEN);

		// TODO: anything else?
	}
}

// ProcessDirtyRegion
void
WindowLayer::ProcessDirtyRegion(BRegion* region)
{
	// if this is exectuted in the desktop thread,
	// it means that the window thread currently
	// blocks to get the read lock, if it is
	// executed from the window thread, it should
	// have the read lock and the desktop thread
	// is blocking to get the write lock. IAW, this
	// is only executed in one thread.
	if (fDirtyRegion.CountRects() == 0) {
		// the window needs to be informed
		// when the dirty region was empty.
		// NOTE: when the window thread has processed
		// the dirty region in MessageReceived(),
		// it will make the region empty again, 
		// when it is empty here, we need to send
		// the message to initiate the next update round.
		// Until the message is processed in the window
		// thread, the desktop thread can add parts to
		// the region as it likes.
		PostMessage(MSG_REDRAW, this);
	}
	// this is executed from the desktop thread
	fDirtyRegion.Include(region);
}

// MarkDirty
void
WindowLayer::MarkDirty(BRegion* regionOnScreen)
{
	// for marking any part of the desktop dirty
	// this will get write access to the global
	// region lock, and result in ProcessDirtyRegion()
	// to be called for any windows affected
	if (fDesktop)
		fDesktop->MarkDirty(regionOnScreen);
}

// MarkContentDirty
void
WindowLayer::MarkContentDirty(BRegion* regionOnScreen)
{
	// for triggering MSG_REDRAW
	// since this won't affect other windows, read locking
	// is sufficient. If there was no dirty region before,
	// an update message is triggered
	if (fDesktop && fDesktop->ReadLockClipping()) {

		regionOnScreen->IntersectWith(&VisibleContentRegion());
		ProcessDirtyRegion(regionOnScreen);

		fDesktop->ReadUnlockClipping();
	}
}

// InvalidateView
void
WindowLayer::InvalidateView(int32 token)
{
	if (fDesktop && fDesktop->ReadLockClipping()) {

		ViewLayer* layer = (ViewLayer*)fTokenViewMap.ItemAt(token);
		if (!layer || !layer->IsVisible()) {
			fDesktop->ReadUnlockClipping();
			return;
		}
		if (!fContentRegionValid)
			_UpdateContentRegion();

		_MarkContentDirty(&layer->ScreenClipping(&fContentRegion));

		fDesktop->ReadUnlockClipping();
	}
}

//# pragma mark -

// CopyContents
void
WindowLayer::CopyContents(BRegion* region, int32 xOffset, int32 yOffset)
{
	// this function takes care of invalidating parts that could not be copied

	if (fDesktop->ReadLockClipping()) {

		BRegion newDirty(*region);

		// clip the region to the visible contents at the
		// source and destination location (not that VisibleContentRegion()
		// is used once to make sure it is valid, then fVisibleContentRegion
		// is used directly)
		region->IntersectWith(&VisibleContentRegion());
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
				if (fCurrentUpdateSession.IsUsed())
					_ShiftPartOfRegion(&fCurrentUpdateSession.DirtyRegion(), region, xOffset, yOffset);
				if (fPendingUpdateSession.IsUsed())
					_ShiftPartOfRegion(&fPendingUpdateSession.DirtyRegion(), region, xOffset, yOffset);
		
			}
		}
		// what is left visible from the original region
		// at the destination after the region which could be
		// copied has been excluded, is considered dirty
		// NOTE: it may look like dirty regions are not moved
		// if no region could be copied, but that's alright,
		// since these parts will now be in newDirty anyways
		// (with the right offset)
		newDirty.OffsetBy(xOffset, yOffset);
		newDirty.IntersectWith(&fVisibleContentRegion);
		if (newDirty.CountRects() > 0)
			ProcessDirtyRegion(&newDirty);

		fDesktop->ReadUnlockClipping();
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
	BRegion dirtyContentRegion(VisibleContentRegion());
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

		if (!fContentRegionValid)
			_UpdateContentRegion();

#if DELAYED_BACKGROUND_CLEARING
		fTopLayer->Draw(fDrawingEngine, &dirtyContentRegion,
						&fContentRegion, false);
#else
		fTopLayer->Draw(fDrawingEngine, &dirtyContentRegion,
						&fContentRegion, true);
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
	if (fDesktop->ReadLockClipping()) {

		ViewLayer* layer = (ViewLayer*)fTokenViewMap.ItemAt(token);
		if (!layer || !layer->IsVisible()) {
			fDesktop->ReadUnlockClipping();
			return;
		}

		if (!fEffectiveDrawingRegionValid) {
			fEffectiveDrawingRegion = VisibleContentRegion();
			if (fInUpdate) {
				// enforce the dirty region of the update session
				fEffectiveDrawingRegion.IntersectWith(&fCurrentUpdateSession.DirtyRegion());
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
		if (!fContentRegionValid)
			_UpdateContentRegion();
		effectiveClipping.IntersectWith(&layer->ScreenClipping(&fContentRegion));

		if (effectiveClipping.CountRects() > 0) {
#if DELAYED_BACKGROUND_CLEARING
			// clear the back ground
			// TODO: only if this is the first drawing command for
			// this layer of course! in the simulation, all client
			// drawing is done from a single command yet
			layer->Draw(fDrawingEngine, &effectiveClipping,
						&fContentRegion, false);
#endif

			layer->ClientDraw(fDrawingEngine, &effectiveClipping);
		}

		fDesktop->ReadUnlockClipping();
	}
}

// _DrawClientPolygon
void
WindowLayer::_DrawClientPolygon(int32 token, BPoint polygon[4])
{
	if (fDesktop->ReadLockClipping()) {

		ViewLayer* layer = (ViewLayer*)fTokenViewMap.ItemAt(token);
		if (!layer || !layer->IsVisible()) {
			fDesktop->ReadUnlockClipping();
			return;
		}

		if (!fEffectiveDrawingRegionValid) {
			fEffectiveDrawingRegion = VisibleContentRegion();
			if (fInUpdate) {
				// enforce the dirty region of the update session
				fEffectiveDrawingRegion.IntersectWith(&fCurrentUpdateSession.DirtyRegion());
			} else {
				printf("%s - _DrawClientPolygon(token: %ld) - not in update\n", Name(), token);
			}
			fEffectiveDrawingRegionValid = true;
		}

		BRegion effectiveClipping(fEffectiveDrawingRegion);
		if (!fContentRegionValid)
			_UpdateContentRegion();
		effectiveClipping.IntersectWith(&layer->ScreenClipping(&fContentRegion));

		if (effectiveClipping.CountRects() > 0) {
#if DELAYED_BACKGROUND_CLEARING
			layer->Draw(fDrawingEngine, &effectiveClipping,
						&fContentRegion, false);
#endif

			layer->ConvertToTop(&polygon[0]);
			layer->ConvertToTop(&polygon[1]);
			layer->ConvertToTop(&polygon[2]);
			layer->ConvertToTop(&polygon[3]);

			if (fDrawingEngine->Lock()) {

				fDrawingEngine->ConstrainClipping(&effectiveClipping);

//				fDrawingEngine->SetPenSize(3);
//				fDrawingEngine->SetDrawingMode(B_OP_BLEND);
				fDrawingEngine->StrokeLine(polygon[0], polygon[1], layer->ViewColor());
				fDrawingEngine->StrokeLine(polygon[1], polygon[2], layer->ViewColor());
				fDrawingEngine->StrokeLine(polygon[2], polygon[3], layer->ViewColor());
				fDrawingEngine->StrokeLine(polygon[3], polygon[0], layer->ViewColor());

				fDrawingEngine->Unlock();
			}
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

		rgb_color lowColor;
		rgb_color highColor;
		if (fFocus) {
			lowColor = (rgb_color){ 255, 203, 0, 255 };
			highColor = (rgb_color){ 0, 0, 0, 255 };
		} else {
			lowColor = (rgb_color){ 216, 216, 216, 0 };
			highColor = (rgb_color){ 30, 30, 30, 255 };
		}

		fDrawingEngine->FillRegion(&dirtyBorderRegion, lowColor);

		rgb_color light = tint_color(lowColor, B_LIGHTEN_2_TINT);
		rgb_color shadow = tint_color(lowColor, B_DARKEN_2_TINT);

		if (fDrawingEngine->Lock()) {

			fDrawingEngine->ConstrainClipping(&dirtyBorderRegion);

			fDrawingEngine->DrawString(Name(), BPoint(fFrame.left, fFrame.top - 5), highColor);

			BRect frame(fFrame);
			frame.InsetBy(-1, -1);
			fDrawingEngine->StrokeLine(BPoint(frame.left, frame.bottom),
									   BPoint(frame.left, frame.top), shadow);
			fDrawingEngine->StrokeLine(BPoint(frame.left + 1, frame.top),
									   BPoint(frame.right, frame.top), shadow);
			fDrawingEngine->StrokeLine(BPoint(frame.right, frame.top + 1),
									   BPoint(frame.right, frame.bottom - 11), light);
			fDrawingEngine->StrokeLine(BPoint(frame.right - 1, frame.bottom - 11),
									   BPoint(frame.right - 11, frame.bottom - 11), light);
			fDrawingEngine->StrokeLine(BPoint(frame.right - 11, frame.bottom - 10),
									   BPoint(frame.right - 11, frame.bottom), light);
			fDrawingEngine->StrokeLine(BPoint(frame.right - 12, frame.bottom),
									   BPoint(frame.left + 1, frame.bottom), light);

			frame.InsetBy(-3, -3);
			int32 tabRight = ceilf((fFrame.left + fFrame.right) / 2);
			fDrawingEngine->StrokeLine(BPoint(frame.left, frame.bottom),
									   BPoint(frame.left, frame.top - 16), light);
			fDrawingEngine->StrokeLine(BPoint(frame.left + 1, frame.top - 16),
									   BPoint(tabRight, frame.top - 16), light);
			fDrawingEngine->StrokeLine(BPoint(tabRight, frame.top - 15),
									   BPoint(tabRight, frame.top), shadow);
			fDrawingEngine->StrokeLine(BPoint(tabRight + 1, frame.top),
									   BPoint(frame.right, frame.top), light);
			fDrawingEngine->StrokeLine(BPoint(frame.right, frame.top + 1),
									   BPoint(frame.right, frame.bottom), shadow);
			fDrawingEngine->StrokeLine(BPoint(frame.right, frame.bottom),
									   BPoint(frame.left + 1, frame.bottom), shadow);

			fDrawingEngine->ConstrainClipping(NULL);
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

	// add to pending
	fPendingUpdateSession.SetUsed(true);
	fPendingUpdateSession.Include(contentDirtyRegion);

	// clip pending update session from current
	// update session, it makes no sense to draw stuff
	// already needing a redraw anyways. Theoretically,
	// this could be done smarter (clip layers from pending
	// that have not yet been redrawn in the current update
	// session)
	if (fCurrentUpdateSession.IsUsed()) {
		fCurrentUpdateSession.Exclude(contentDirtyRegion);
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

		if (fUpdateRequested && !fCurrentUpdateSession.IsUsed()) {
			if (fPendingUpdateSession.IsUsed()) {

				// TODO: the toggling between the update sessions is too
				// expensive, optimize with some pointer tricks
				fCurrentUpdateSession = fPendingUpdateSession;
				fPendingUpdateSession.SetUsed(false);
		
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
			fCurrentUpdateSession.SetUsed(false);
		
			fInUpdate = false;
			fEffectiveDrawingRegionValid = false;
		}
		if (fPendingUpdateSession.IsUsed()) {
			// send this to client
			fClient->PostMessage(MSG_UPDATE);
			fUpdateRequested = true;
		} else {
			fUpdateRequested = false;
		}
	
	fDesktop->ReadUnlockClipping();
	}
}

// _UpdateContentRegion
void
WindowLayer::_UpdateContentRegion()
{
	// TODO: speed up by avoiding "Exclude()"
	// start from the frame, extend to include decorator border
	fContentRegion.Set(fFrame);

	// resize handle
	// if (B_DOCUMENT_WINDOW_LOOK)
		fContentRegion.Exclude(BRect(fFrame.right - 10, fFrame.bottom - 10,
									 fFrame.right, fFrame.bottom));

	fContentRegionValid = true;
}

// #pragma mark -

// constructor
UpdateSession::UpdateSession()
	: fDirtyRegion(),
	  fInUse(false)
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

// SetUsed
void
UpdateSession::SetUsed(bool used)
{
	fInUse = used;
	if (!fInUse) {
		fDirtyRegion.MakeEmpty();
	}
}

// operator=
UpdateSession&
UpdateSession::operator=(const UpdateSession& other)
{
	fDirtyRegion = other.fDirtyRegion;
	fInUse = other.fInUse;
	return *this;
}



