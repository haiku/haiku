
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
	  fVisibleContentRegion(),

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
				_DrawBorder();
				_DrawContents(fTopLayer);

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
	if (Lock()) {
		// executed from Desktop thread, so it's fine
		// to use the clipping without locking
		if (fDesktop->ReadLockClipping()) {
			BRegion dirty(fBorderRegion);
			dirty.IntersectWith(&fVisibleRegion);
			fDesktop->MarkDirty(&dirty);

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
	// this function is only called from the desktop thread

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

	fEffectiveDrawingRegionValid = false;

	fTopLayer->MoveBy(x, y, NULL);

	// the desktop will take care of dirty regions
}

// ResizeBy
void
WindowLayer::ResizeBy(int32 x, int32 y, BRegion* dirtyRegion)
{
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

// MarkDirty
void
WindowLayer::MarkDirty(BRegion* regionOnScreen)
{
	if (fDesktop)
		fDesktop->MarkDirty(regionOnScreen);
}

// MarkContentDirty
void
WindowLayer::MarkContentDirty(BRegion* regionOnScreen)
{
	if (fDesktop && fDesktop->LockClipping()) {

		regionOnScreen->IntersectWith(&fVisibleContentRegion);
		fDesktop->MarkDirty(regionOnScreen);

		fDesktop->UnlockClipping();
	}
}
/*
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
*/
# pragma mark -

// _DrawContents
void
WindowLayer::_DrawContents(ViewLayer* layer)
{
//printf("%s - DrawContents()\n", Name());
	if (!layer)
		layer = fTopLayer;

	if (fDesktop->ReadLockClipping()) {
	
		BRegion dirtyContentRegion(fVisibleContentRegion);
		dirtyContentRegion.IntersectWith(fDesktop->DirtyRegion());

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

			fDesktop->MarkClean(&dirtyContentRegion);

			layer->Draw(fDrawingEngine, &dirtyContentRegion,
						&fVisibleContentRegion, true);
		}

		fDesktop->ReadUnlockClipping();
	}
}

// _DrawClient
void
WindowLayer::_DrawClient(int32 token)
{
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

		BRegion effectiveClipping(fEffectiveDrawingRegion);
		effectiveClipping.IntersectWith(&layer->ScreenClipping(&fVisibleContentRegion));
		
		if (effectiveClipping.CountRects() > 0) {
			layer->ClientDraw(fDrawingEngine, &effectiveClipping);
		}

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
	if (fDesktop->ReadLockClipping()) {
	
		// construct the region of the border that needs redrawing
		BRegion dirtyBorderRegion;
		GetBorderRegion(&dirtyBorderRegion);
		// intersect with our visible region
		dirtyBorderRegion.IntersectWith(&fVisibleRegion);
		// intersect with the Desktop's dirty region
		dirtyBorderRegion.IntersectWith(fDesktop->DirtyRegion());
	
		if (dirtyBorderRegion.CountRects() > 0) {
			if (fDrawingEngine->Lock()) {

				fDrawingEngine->ConstrainClippingRegion(&dirtyBorderRegion);

				if (fFocus) {
					fDrawingEngine->SetLowColor(255, 203, 0, 255);
					fDrawingEngine->SetHighColor(0, 0, 0, 255);
				} else {
					fDrawingEngine->SetLowColor(216, 216, 216, 0);
					fDrawingEngine->SetHighColor(0, 0, 0, 255);
				}

				fDrawingEngine->FillRect(dirtyBorderRegion.Frame(), B_SOLID_LOW);
				fDrawingEngine->DrawString(Name(), BPoint(fFrame.left, fFrame.top - 5));

				fDrawingEngine->ConstrainClippingRegion(NULL);
				fDrawingEngine->MarkDirty(&dirtyBorderRegion);
				fDrawingEngine->Unlock();
			}
			fDesktop->MarkClean(&dirtyBorderRegion);
		}

		fDesktop->ReadUnlockClipping();
	}
}

// _MarkContentDirty
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

	// clip pending update session from current,
	// current update session, because the backgrounds have been
	// cleared again already 
	if (fCurrentUpdateSession) {
		fCurrentUpdateSession->Exclude(contentDirtyRegion);
		fEffectiveDrawingRegionValid = false;
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
		fEffectiveDrawingRegionValid = false;
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
		fEffectiveDrawingRegionValid = false;
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




