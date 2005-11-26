
#include <new>
#include <stdio.h>

#include <Message.h>
#include <MessageQueue.h>

#include "ClientLooper.h"
#include "Desktop.h"
#include "DrawingEngine.h"

#include "WindowLayer.h"

#define SLOW_DRAWING 0

// constructor
WindowLayer::WindowLayer(BRect frame, const char* name,
						 DrawingEngine* drawingEngine, Desktop* desktop)
	: BLooper(name),
	  fFrame(frame),
	  fVisibleRegion(),

	  fBorderRegion(),
	  fBorderRegionValid(false),
	  fContentRegion(),
	  fContentRegionValid(false),

	  fFocus(false),

	  fTopLayer(NULL),

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
	fTopLayer->AttachedToWindow(this, true);

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
			if (!MessageQueue()->FindMessage(MSG_REDRAW, 0)) {
				while (!fDesktop->ReadLockClippingWithTimeout()) {
//printf("%s MSG_REDRAW -> timeout\n", Name());
					if (MessageQueue()->FindMessage(MSG_REDRAW, 0)) {
//printf("%s MSG_REDRAW -> timeout - leaving because there are pending redraws\n", Name());
						return;
					}
				}
				_DrawContents(fTopLayer);
				_DrawBorder();

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
	// start from full region (as if the window was fully visible)
	GetFullRegion(&fVisibleRegion);
	// clip to region still available on screen
	fVisibleRegion.IntersectWith(stillAvailableOnScreen);
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
WindowLayer::GetBorderRegion(BRegion* region) const
{
	if (fBorderRegionValid) {
		*region = fBorderRegion;
	}

	// TODO: speed up by avoiding "Exclude()"
	// start from the frame, extend to include decorator border
	region->Set(BRect(fFrame.left - 4, fFrame.top - 4,
					  fFrame.right + 4, fFrame.bottom + 4));

	region->Exclude(fFrame);

	// add the title tab
	region->Include(BRect(fFrame.left - 4, fFrame.top - 20,
						  (fFrame.left + fFrame.right) / 2, fFrame.top - 5));

	// resize handle
	// if (B_DOCUMENT_WINDOW_LOOK)
		region->Include(BRect(fFrame.right - 10, fFrame.bottom - 10,
							  fFrame.right, fFrame.bottom));
}

// GetContentRegion
void
WindowLayer::GetContentRegion(BRegion* region) const
{
	if (fContentRegionValid) {
		*region = fContentRegion;
	}

	// TODO: speed up by avoiding "Exclude()"
	// start from the frame, extend to include decorator border
	region->Set(fFrame);

	// resize handle
	// if (B_DOCUMENT_WINDOW_LOOK)
		region->Exclude(BRect(fFrame.right - 10, fFrame.bottom - 10,
							  fFrame.right, fFrame.bottom));
}

// SetFocus
void
WindowLayer::SetFocus(bool focus)
{
	if (Lock()) {
		// executed from Desktop thread, so it's fine
		// to use the clipping without locking
		if (fDesktop->ReadLockClipping()) {
			BRegion dirty(fBorderRegion);
			dirty.IntersectWith(&fVisibleRegion);
			MarkDirty(&dirty);

			fDesktop->ReadUnlockClipping();
		}

		fFocus = focus;

		Unlock();
	}
}

// MoveBy
void
WindowLayer::MoveBy(int32 x, int32 y)
{
	if (x == 0 && y == 0)
		return;

	fFrame.OffsetBy(x, y);

	if (fBorderRegionValid)
		fBorderRegion.OffsetBy(x, y);
	if (fContentRegionValid)
		fContentRegion.OffsetBy(x, y);

	if (fCurrentUpdateSession)
		fCurrentUpdateSession->MoveBy(x, y);
	if (fPendingUpdateSession)
		fPendingUpdateSession->MoveBy(x, y);

	fTopLayer->MoveBy(x, y);

	// TODO: move a local dirty region!
}

// ResizeBy
void
WindowLayer::ResizeBy(int32 x, int32 y, BRegion* dirtyRegion)
{
	if (x == 0 && y == 0)
		return;

	fFrame.right += x;
	fFrame.bottom += y;

	fBorderRegionValid = false;
	fContentRegionValid = false;

	// the border is dirty, put it into
	// dirtyRegion for a start
	GetBorderRegion(dirtyRegion);

	// put the previous border region into the dirty region as well
	// to handle the part that was overlapping a layer
//	if B_DOCUMENT_WINDOW_LOOK
		dirtyRegion->Include(&fBorderRegion);

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

// MarkDirty
void
WindowLayer::MarkDirty(BRegion* regionOnScreen)
{
	if (fDesktop)
		fDesktop->MarkDirty(regionOnScreen);
}

// DirtyRegion
BRegion
WindowLayer::DirtyRegion()
{
	BRegion dirty;
	if (fDesktop->ReadLockClipping()) {
		dirty = *fDesktop->DirtyRegion();
		fDesktop->ReadUnlockClipping();
	}
	return dirty;
}

# pragma mark -

// _DrawContents
void
WindowLayer::_DrawContents(ViewLayer* layer)
{
//printf("%s - DrawContents()\n", Name());
#if SLOW_DRAWING
	snooze(10000);
#endif

	if (!layer)
		layer = fTopLayer;

	if (!fContentRegionValid) {
		GetContentRegion(&fContentRegion);
		fContentRegionValid = true;
	}

	BRegion effectiveWindowClipping(fContentRegion);
	// TODO: simplify
	// ideally, there would only be a local fDirtyRegion,
	// that we need to intersect with. fDirtyRegion would
	// already only include fVisibleRegion
	effectiveWindowClipping.IntersectWith(&fVisibleRegion);
	effectiveWindowClipping.IntersectWith(fDesktop->DirtyRegion());
	if (effectiveWindowClipping.Frame().IsValid()) {
		// send UPDATE message to the client here
		_MarkContentDirty(&effectiveWindowClipping);

		layer->Draw(fDrawingEngine, &effectiveWindowClipping, true);

		fDesktop->MarkClean(&fContentRegion);
	}
//else {
//printf("  nothing to do\n");
//}

}

// _DrawClient
void
WindowLayer::_DrawClient(int32 token)
{
	ViewLayer* layer = (ViewLayer*)fTokenViewMap.ItemAt(token);
	if (!layer)
		return;

	BRegion effectiveClipping(layer->ScreenClipping());
	if (fInUpdate) {
		// enforce the dirty region of the update session
		effectiveClipping.IntersectWith(&fCurrentUpdateSession->DirtyRegion());
	} else {
printf("%s - _DrawClient(token: %ld) - not in update\n", Name(), token);
	}

	if (effectiveClipping.CountRects() > 0 && fDesktop->ReadLockClipping()) {
		effectiveClipping.IntersectWith(&fVisibleRegion);
		// TODO: This step seems too much
		BRegion contentRegion;
		GetContentRegion(&contentRegion);
		effectiveClipping.IntersectWith(&contentRegion);

		layer->ClientDraw(fDrawingEngine, &effectiveClipping);

		fDesktop->ReadUnlockClipping();
	}
}

// _DrawBorder
void
WindowLayer::_DrawBorder()
{
//printf("%s - DrawBorder()\n", Name());
#if SLOW_DRAWING
	snooze(10000);
#endif

	if (!fBorderRegionValid) {
		GetBorderRegion(&fBorderRegion);
		fBorderRegionValid = true;
	}

	// construct the region of the border that needs redrawing
	BRegion dirtyBorderRegion(fBorderRegion);
	// intersect with our visible region
	dirtyBorderRegion.IntersectWith(&fVisibleRegion);
	// intersect with the Desktop's dirty region
	dirtyBorderRegion.IntersectWith(fDesktop->DirtyRegion());

	if (dirtyBorderRegion.Frame().IsValid()) {
		if (fDrawingEngine->Lock()) {
			if (fFocus)
				fDrawingEngine->SetHighColor(255, 203, 0, 255);
			else
				fDrawingEngine->SetHighColor(216, 216, 216, 0);
			fDrawingEngine->FillRegion(&dirtyBorderRegion);
			fDrawingEngine->MarkDirty(&dirtyBorderRegion);
			fDrawingEngine->Unlock();
		}
		fDesktop->MarkClean(&dirtyBorderRegion);
	}
//else {
//printf("  nothing to do\n");
//}
}

// _MarkContentDirty
void
WindowLayer::_MarkContentDirty(BRegion* localDirty)
{
	if (localDirty->CountRects() <= 0)
		return;
	if (!fPendingUpdateSession) {
		// create new pending
		fPendingUpdateSession = new UpdateSession(*localDirty);
	} else {
		// add to pending
		fPendingUpdateSession->Include(localDirty);
	}

	if (!fUpdateRequested) {
		// send this to client
		fClient->PostMessage(MSG_UPDATE);
		fUpdateRequested = true;
	}
}

// _BeginUpdate
void
WindowLayer::_BeginUpdate()
{
	if (fUpdateRequested && !fCurrentUpdateSession) {
		fCurrentUpdateSession = fPendingUpdateSession;
		fPendingUpdateSession = NULL;

		if (fCurrentUpdateSession) {
			// all drawing command from the client
			// will have the dirty region from the update
			// session enforced
			fInUpdate = true;
		}
	}
}

// _EndUpdate
void
WindowLayer::_EndUpdate()
{
	if (fInUpdate) {
		delete fCurrentUpdateSession;
		fCurrentUpdateSession = NULL;
	
		fInUpdate = false;
	}
	if (fPendingUpdateSession) {
		// send this to client
		fClient->PostMessage(MSG_UPDATE);
		fUpdateRequested = true;
	} else {
		fUpdateRequested = false;
	}
}

#pragma mark -

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

// MoveBy
void
UpdateSession::MoveBy(int32 x, int32 y)
{
	fDirtyRegion.OffsetBy(x, y);
}




