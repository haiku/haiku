
#include <new>
#include <stdio.h>

#include <Message.h>
#include <MessageQueue.h>

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

	  fBorderColor((rgb_color){ 255, 203, 0, 255 }),

	  fTopLayer(NULL),

	  fDrawingEngine(drawingEngine),
	  fDesktop(desktop)
{
	// the top layer is special, it has a coordinate system
	// as if it was attached directly to the desktop, therefor,
	// the coordinate conversion through the layer tree works
	// as expected, since the top layer has no "parent" but has
	// fFrame as if it had
	fTopLayer = new(nothrow) ViewLayer(fFrame, "top view", B_FOLLOW_ALL, 0,
									   (rgb_color){ 255, 255, 255, 255 });
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
				while (!fDesktop->ReadLockClipping()) {
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

// MoveBy
void
WindowLayer::MoveBy(int32 x, int32 y)
{
	if (x == 0 && y == 0)
		return;

	fFrame.OffsetBy(x, y);

	fTopLayer->MoveBy(x, y);

	// TODO: move a local dirty region!
}

// ResizeBy
void
WindowLayer::ResizeBy(int32 x, int32 y, BRegion* dirtyRegion)
{
	if (x == 0 && y == 0)
		return;

//	BRegion previousBorder();
//
	fFrame.right += x;
	fFrame.bottom += y;

	// the border is dirty, put it into
	// dirtyRegion for a start
	GetBorderRegion(dirtyRegion);

	fTopLayer->ResizeBy(x, y, dirtyRegion);
}

// AddChild
void
WindowLayer::AddChild(ViewLayer* layer)
{
	fTopLayer->AddChild(layer);

	// TODO: trigger redraw for dirty regions
}

// MarkDirty
void
WindowLayer::MarkDirty(BRegion* regionOnScreen)
{
	fDesktop->MarkDirty(regionOnScreen);
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

	BRegion effectiveLayerClipping(layer->ScreenClipping());
	effectiveLayerClipping.IntersectWith(&fVisibleRegion);
	effectiveLayerClipping.IntersectWith(fDesktop->DirtyRegion());
	if (effectiveLayerClipping.Frame().IsValid()) {
		layer->Draw(fDrawingEngine, &effectiveLayerClipping, true);
		fDesktop->MarkClean(&effectiveLayerClipping);
	}
//else {
//printf("  nothing to do\n");
//}

}

// _DrawBorder
void
WindowLayer::_DrawBorder()
{
//printf("%s - DrawBorder()\n", Name());
#if SLOW_DRAWING
	snooze(10000);
#endif

	// construct the region containing just the border
	BRegion borderRegion(fVisibleRegion);
	borderRegion.Exclude(fFrame);
	// intersect with the Desktop's dirty region
	borderRegion.IntersectWith(fDesktop->DirtyRegion());

	if (borderRegion.Frame().IsValid()) {
		if (fDrawingEngine->Lock()) {
			fDrawingEngine->SetHighColor(fBorderColor);
			fDrawingEngine->FillRegion(&borderRegion);
			fDrawingEngine->MarkDirty(&borderRegion);
			fDrawingEngine->Unlock();
		}
		fDesktop->MarkClean(&borderRegion);
	}
//else {
//printf("  nothing to do\n");
//}
}

