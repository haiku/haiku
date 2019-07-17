/*
 * Copyright 2009-2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "NetReceiver.h"
#include "NetSender.h"
#include "RemoteMessage.h"
#include "RemoteView.h"
#include "StreamingRingBuffer.h"

#include <Application.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Message.h>
#include <NetEndpoint.h>
#include <Region.h>
#include <Shape.h>
#include <Window.h>
#include <utf8_functions.h>

#include <new>
#include <stdio.h>


static const uint8 kCursorData[] = { 16 /* size, 16x16 */,
	1 /* depth, 1 bit per pixel */, 0, 0, /* hot spot at 0, 0 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


#define TRACE(x...)				/*printf("RemoteView: " x)*/
#define TRACE_ALWAYS(x...)		printf("RemoteView: " x)
#define TRACE_ERROR(x...)		printf("RemoteView: " x)


typedef struct engine_state {
	uint32		token;
	BView *		view;
	::pattern	pattern;
	BRegion		clipping_region;
	float		pen_size;
	bool		sync_drawing;
} engine_state;


RemoteView::RemoteView(BRect frame, const char *remoteHost, uint16 remotePort)
	:
	BView(frame, "RemoteView", B_FOLLOW_NONE, B_WILL_DRAW),
	fInitStatus(B_NO_INIT),
	fIsConnected(false),
	fReceiveBuffer(NULL),
	fSendBuffer(NULL),
	fEndpoint(NULL),
	fReceiver(NULL),
	fSender(NULL),
	fStopThread(false),
	fOffscreenBitmap(NULL),
	fOffscreen(NULL),
	fViewCursor(kCursorData),
	fCursorBitmap(NULL),
	fCursorVisible(false)
{
	fReceiveBuffer = new(std::nothrow) StreamingRingBuffer(16 * 1024);
	if (fReceiveBuffer == NULL) {
		fInitStatus = B_NO_MEMORY;
		TRACE_ERROR("no memory available\n");
		return;
	}

	fInitStatus = fReceiveBuffer->InitCheck();
	if (fInitStatus != B_OK)
		return;

	fSendBuffer = new(std::nothrow) StreamingRingBuffer(16 * 1024);
	if (fSendBuffer == NULL) {
		fInitStatus = B_NO_MEMORY;
		TRACE_ERROR("no memory available\n");
		return;
	}

	fInitStatus = fSendBuffer->InitCheck();
	if (fInitStatus != B_OK)
		return;

	fEndpoint = new(std::nothrow) BNetEndpoint();
	if (fEndpoint == NULL) {
		fInitStatus = B_NO_MEMORY;
		TRACE_ERROR("no memory available\n");
		return;
	}

	fInitStatus = fEndpoint->Connect(remoteHost, remotePort);
	if (fInitStatus != B_OK) {
		TRACE_ERROR("failed to connect to %s:%" B_PRIu16 "\n",
			remoteHost, remotePort);
		return;
	}

	fSender = new(std::nothrow) NetSender(fEndpoint, fSendBuffer);
	if (fSender == NULL) {
		fInitStatus = B_NO_MEMORY;
		TRACE_ERROR("no memory available\n");
		return;
	}

	fReceiver = new(std::nothrow) NetReceiver(fEndpoint, fReceiveBuffer);
	if (fReceiver == NULL) {
		fInitStatus = B_NO_MEMORY;
		TRACE_ERROR("no memory available\n");
		return;
	}

	BRect bounds = frame.OffsetToCopy(0, 0);
	fOffscreenBitmap = new(std::nothrow) BBitmap(bounds, B_BITMAP_ACCEPTS_VIEWS,
		B_RGB32);
	if (fOffscreenBitmap == NULL) {
		fInitStatus = B_NO_MEMORY;
		TRACE_ERROR("no memory available\n");
		return;
	}

	fOffscreen = new(std::nothrow) BView(bounds, "offscreen remote view",
		B_FOLLOW_NONE, B_WILL_DRAW);
	if (fOffscreen == NULL) {
		fInitStatus = B_NO_MEMORY;
		TRACE_ERROR("no memory available\n");
		return;
	}

	fOffscreenBitmap->AddChild(fOffscreen);
	fOffscreen->SetDrawingMode(B_OP_COPY);

	fDrawThread = spawn_thread(&_DrawEntry, "draw thread", B_NORMAL_PRIORITY,
		this);
	if (fDrawThread < 0) {
		fInitStatus = fDrawThread;

		TRACE_ERROR("failed to start _DrawThread()\n");
		TRACE_ERROR("status = %" B_PRIx32 "\n", fInitStatus);

		return;
	}

	resume_thread(fDrawThread);
}


RemoteView::~RemoteView()
{
	fStopThread = true;

	delete fReceiver;
	delete fReceiveBuffer;

	delete fSendBuffer;
	delete fSender;

	delete fEndpoint;

	delete fOffscreenBitmap;
	delete fCursorBitmap;

	int32 result;
	wait_for_thread(fDrawThread, &result);
}


status_t
RemoteView::InitCheck()
{
	return fInitStatus;
}


void
RemoteView::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetViewCursor(&fViewCursor);
}


void
RemoteView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_COPY);
	fOffscreenBitmap->Lock();
	fOffscreen->Sync();

	DrawBitmap(fOffscreenBitmap, updateRect, updateRect);

	if (fCursorVisible && fCursorBitmap != NULL
		&& fCursorFrame.Intersects(updateRect)) {
		DrawBitmap(fOffscreenBitmap, fCursorFrame, fCursorFrame);
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fCursorBitmap, fCursorFrame.LeftTop());
	}

	fOffscreenBitmap->Unlock();
}


void
RemoteView::MouseMoved(BPoint where, uint32 code, const BMessage *dragMessage)
{
	if (!fIsConnected)
		return;

	_SendMouseMessage(RP_MOUSE_MOVED, where);
}


void
RemoteView::MouseDown(BPoint where)
{
	if (!fIsConnected)
		return;

	_SendMouseMessage(RP_MOUSE_DOWN, where);
}


void
RemoteView::MouseUp(BPoint where)
{
	if (!fIsConnected)
		return;

	_SendMouseMessage(RP_MOUSE_UP, where);
}


void
RemoteView::KeyDown(const char *bytes, int32 numBytes)
{
	if (!fIsConnected)
		return;

	_SendKeyMessage(RP_KEY_DOWN, bytes, numBytes);
}


void
RemoteView::KeyUp(const char *bytes, int32 numBytes)
{
	if (!fIsConnected)
		return;

	_SendKeyMessage(RP_KEY_UP, bytes, numBytes);
}


void
RemoteView::MessageReceived(BMessage *message)
{
	if (!fIsConnected) {
		BView::MessageReceived(message);
		return;
	}

	switch (message->what) {
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
			// these are easily repeated and then cause a flood of messages
			// so we might not want them.
			break;

		case B_MODIFIERS_CHANGED:
		{
			uint32 modifiers = 0;
			message->FindInt32("modifiers", (int32 *)&modifiers);
			RemoteMessage message(NULL, fSendBuffer);
			message.Start(RP_MODIFIERS_CHANGED);
			message.Add(modifiers);
			break;
		}

		case B_MOUSE_WHEEL_CHANGED:
		{
			float xDelta, yDelta;
			if (message->FindFloat("be:wheel_delta_x", &xDelta) != B_OK)
				xDelta = 0;
			if (message->FindFloat("be:wheel_delta_y", &yDelta) != B_OK)
				yDelta = 0;

			RemoteMessage message(NULL, fSendBuffer);
			message.Start(RP_MOUSE_WHEEL_CHANGED);
			message.Add(xDelta);
			message.Add(yDelta);
			break;
		}
	}

	BView::MessageReceived(message);
}


void
RemoteView::_SendMouseMessage(uint16 code, BPoint where)
{
	RemoteMessage message(NULL, fSendBuffer);
	message.Start(code);
	message.Add(where);

	if (code == RP_MOUSE_MOVED)
		return;

	BMessage *event = Window()->CurrentMessage();

	int32 buttons = 0;
	event->FindInt32("buttons", &buttons);
	message.Add(buttons);

	if (code == RP_MOUSE_DOWN)
		return;

	int32 clicks;
	event->FindInt32("clicks", &clicks);
	message.Add(clicks);
}


void
RemoteView::_SendKeyMessage(uint16 code, const char *bytes, int32 numBytes)
{
	RemoteMessage message(NULL, fSendBuffer);
	message.Start(code);
	message.Add(numBytes);
	message.AddList(bytes, numBytes);

	BMessage *event = Window()->CurrentMessage();

	int32 rawChar, key;
	event->FindInt32("raw_char", &rawChar);
	event->FindInt32("key", &key);

	message.Add(rawChar);
	message.Add(key);
}


int
RemoteView::_StateCompareByKey(const uint32 *key, const engine_state *state)
{
	if (state->token == *key)
		return 0;

	if (state->token < *key)
		return -1;

	return 1;
}


engine_state *
RemoteView::_CreateState(uint32 token)
{
	int32 index = fStates.BinarySearchIndexByKey(token, &_StateCompareByKey);
	if (index >= 0) {
		TRACE_ERROR("state for token %" B_PRIu32 " already in list\n", token);
		return NULL;
	}

	engine_state *state = new(std::nothrow) engine_state;
	if (state == NULL) {
		TRACE_ERROR("failed to allocate engine state\n");
		return NULL;
	}

	fOffscreenBitmap->Lock();
	BView *offscreen = new(std::nothrow) BView(fOffscreenBitmap->Bounds(),
		"offscreen remote view", B_FOLLOW_NONE, B_WILL_DRAW);
	if (offscreen == NULL) {
		TRACE_ERROR("failed to allocate offscreen view\n");
		fOffscreenBitmap->Unlock();
		delete state;
		return NULL;
	}

	fOffscreenBitmap->AddChild(offscreen);
	fOffscreenBitmap->Unlock();

	state->token = token;
	state->view = offscreen;
	state->pattern = B_SOLID_HIGH;
	state->clipping_region.MakeEmpty();
	state->pen_size = 0;
	state->sync_drawing = true;

	fStates.AddItem(state, -index - 1);
	return state;
}


void
RemoteView::_DeleteState(uint32 token)
{
	int32 index = fStates.BinarySearchIndexByKey(token, &_StateCompareByKey);
	if (index < 0)
		return;

	engine_state *state = fStates.RemoveItemAt(index);

	fOffscreenBitmap->RemoveChild(state->view);
	delete state->view;
	delete state;
}


engine_state *
RemoteView::_FindState(uint32 token)
{
	return fStates.BinarySearchByKey(token, &_StateCompareByKey);
}


int32
RemoteView::_DrawEntry(void *data)
{
	((RemoteView *)data)->_DrawThread();
	return 0;
}


void
RemoteView::_DrawThread()
{
	RemoteMessage reply(NULL, fSendBuffer);
	RemoteMessage message(fReceiveBuffer, NULL);

	// cursor
	BPoint cursorHotSpot(0, 0);

	reply.Start(RP_INIT_CONNECTION);
	reply.Flush();

	while (!fStopThread) {
		uint16 code;
		status_t status = message.NextMessage(code);

		if (status != B_OK) {
			if (status == B_TIMED_OUT || status == -1) {
				TRACE_ERROR("could not connect to device\n");
			} else {
				TRACE_ERROR("failed to read message from receiver\n");
				break;
			}
		}

		TRACE("code %u with %ld bytes data\n", code, message.DataLeft());

		BAutolock locker(this->Looper());
		if (!locker.IsLocked())
			break;

		// handle stuff that doesn't go to a specific engine
		switch (code) {
			case RP_INIT_CONNECTION:
			{
				BRect bounds = fOffscreenBitmap->Bounds();
				reply.Start(RP_UPDATE_DISPLAY_MODE);
				reply.Add(bounds.IntegerWidth() + 1);
				reply.Add(bounds.IntegerHeight() + 1);
				if (reply.Flush() == B_OK)
					fIsConnected = true;

				continue;
			}

			case RP_CLOSE_CONNECTION:
			{
				be_app->PostMessage(B_QUIT_REQUESTED);
				continue;
			}

			case RP_CREATE_STATE:
			case RP_DELETE_STATE:
			{
				uint32 token;
				message.Read(token);

				if (code == RP_CREATE_STATE)
					_CreateState(token);
				else
					_DeleteState(token);

				continue;
			}

			case RP_SET_CURSOR:
			{
				BBitmap *bitmap;
				BPoint oldHotSpot = cursorHotSpot;
				message.Read(cursorHotSpot);
				if (message.ReadBitmap(&bitmap) != B_OK)
					continue;

				delete fCursorBitmap;
				fCursorBitmap = bitmap;

				Invalidate(fCursorFrame);

				BRect bounds = fCursorBitmap->Bounds();
				fCursorFrame.right = fCursorFrame.left
					+ bounds.IntegerWidth() + 1;
				fCursorFrame.bottom = fCursorFrame.bottom
					+ bounds.IntegerHeight() + 1;

				fCursorFrame.OffsetBy(oldHotSpot - cursorHotSpot);

				Invalidate(fCursorFrame);
				continue;
			}

			case RP_SET_CURSOR_VISIBLE:
			{
				bool wasVisible = fCursorVisible;
				message.Read(fCursorVisible);
				if (wasVisible != fCursorVisible)
					Invalidate(fCursorFrame);
				continue;
			}

			case RP_MOVE_CURSOR_TO:
			{
				BPoint position;
				message.Read(position);

				if (fCursorVisible)
					Invalidate(fCursorFrame);

				fCursorFrame.OffsetTo(position - cursorHotSpot);

				Invalidate(fCursorFrame);
				continue;
			}

			case RP_INVALIDATE_RECT:
			{
				BRect rect;
				if (message.Read(rect) != B_OK)
					continue;

				Invalidate(rect);
				continue;
			}

			case RP_INVALIDATE_REGION:
			{
				BRegion region;
				if (message.ReadRegion(region) != B_OK)
					continue;

				Invalidate(&region);
				continue;
			}

			case RP_FILL_REGION_COLOR_NO_CLIPPING:
			{
				BRegion region;
				rgb_color color;

				message.ReadRegion(region);
				if (message.Read(color) != B_OK)
					continue;

				fOffscreen->LockLooper();
				fOffscreen->SetHighColor(color);
				fOffscreen->FillRegion(&region);
				fOffscreen->UnlockLooper();
				Invalidate(&region);
				continue;
			}

			case RP_COPY_RECT_NO_CLIPPING:
			{
				int32 xOffset, yOffset;
				BRect rect;

				message.Read(xOffset);
				message.Read(yOffset);
				if (message.Read(rect) != B_OK)
					continue;

				BRect dest = rect.OffsetByCopy(xOffset, yOffset);
				fOffscreen->LockLooper();
				fOffscreen->CopyBits(rect, dest);
				fOffscreen->UnlockLooper();
				continue;
			}
		}

		uint32 token;
		message.Read(token);

		engine_state *state = _FindState(token);
		if (state == NULL) {
			TRACE_ERROR("didn't find state for token %" B_PRIu32 "\n", token);
			state = _CreateState(token);
			if (state == NULL) {
				TRACE_ERROR("failed to create state for unknown token\n");
				continue;
			}
		}

		BView *offscreen = state->view;
		::pattern &pattern = state->pattern;
		BRegion &clippingRegion = state->clipping_region;
		float &penSize = state->pen_size;
		bool &syncDrawing = state->sync_drawing;
		BRegion invalidRegion;

		BAutolock offscreenLocker(offscreen->Looper());
		if (!offscreenLocker.IsLocked())
			break;

		switch (code) {
			case RP_ENABLE_SYNC_DRAWING:
				syncDrawing = true;
				continue;

			case RP_DISABLE_SYNC_DRAWING:
				syncDrawing = false;
				continue;

			case RP_SET_OFFSETS:
			{
				int32 xOffset, yOffset;
				message.Read(xOffset);
				if (message.Read(yOffset) != B_OK)
					continue;

				offscreen->MovePenTo(xOffset, yOffset);
				break;
			}

			case RP_SET_HIGH_COLOR:
			case RP_SET_LOW_COLOR:
			{
				rgb_color color;
				if (message.Read(color) != B_OK)
					continue;

				if (code == RP_SET_HIGH_COLOR)
					offscreen->SetHighColor(color);
				else
					offscreen->SetLowColor(color);

				break;
			}

			case RP_SET_PEN_SIZE:
			{
				float newPenSize;
				if (message.Read(newPenSize) != B_OK)
					continue;

				offscreen->SetPenSize(newPenSize);
				penSize = newPenSize / 2;
				break;
			}

			case RP_SET_STROKE_MODE:
			{
				cap_mode capMode;
				join_mode joinMode;
				float miterLimit;

				message.Read(capMode);
				message.Read(joinMode);
				if (message.Read(miterLimit) != B_OK)
					continue;

				offscreen->SetLineMode(capMode, joinMode, miterLimit);
				break;
			}

			case RP_SET_BLENDING_MODE:
			{
				source_alpha sourceAlpha;
				alpha_function alphaFunction;

				message.Read(sourceAlpha);
				if (message.Read(alphaFunction) != B_OK)
					continue;

				offscreen->SetBlendingMode(sourceAlpha, alphaFunction);
				break;
			}

			case RP_SET_TRANSFORM:
			{
				BAffineTransform transform;
				if (message.ReadTransform(transform) != B_OK)
					continue;

				offscreen->SetTransform(transform);
				break;
			}

			case RP_SET_PATTERN:
			{
				if (message.Read(pattern) != B_OK)
					continue;
				break;
			}

			case RP_SET_DRAWING_MODE:
			{
				drawing_mode drawingMode;
				if (message.Read(drawingMode) != B_OK)
					continue;

				offscreen->SetDrawingMode(drawingMode);
				break;
			}

			case RP_SET_FONT:
			{
				BFont font;
				if (message.ReadFontState(font) != B_OK)
					continue;

				offscreen->SetFont(&font);
				break;
			}

			case RP_CONSTRAIN_CLIPPING_REGION:
			{
				if (message.ReadRegion(clippingRegion) != B_OK)
					continue;

				offscreen->ConstrainClippingRegion(&clippingRegion);
				break;
			}

			case RP_INVERT_RECT:
			{
				BRect rect;
				if (message.Read(rect) != B_OK)
					continue;

				offscreen->InvertRect(rect);
				invalidRegion.Include(rect);
				break;
			}

			case RP_DRAW_BITMAP:
			{
				BBitmap *bitmap;
				BRect bitmapRect, viewRect;
				uint32 options;

				message.Read(bitmapRect);
				message.Read(viewRect);
				message.Read(options);
				if (message.ReadBitmap(&bitmap) != B_OK || bitmap == NULL)
					continue;

				offscreen->DrawBitmap(bitmap, bitmapRect, viewRect, options);
				invalidRegion.Include(viewRect);
				delete bitmap;
				break;
			}

			case RP_DRAW_BITMAP_RECTS:
			{
				color_space colorSpace;
				int32 rectCount;
				uint32 flags, options;

				message.Read(options);
				message.Read(colorSpace);
				message.Read(flags);
				message.Read(rectCount);
				for (int32 i = 0; i < rectCount; i++) {
					BBitmap *bitmap;
					BRect viewRect;

					message.Read(viewRect);
					if (message.ReadBitmap(&bitmap, true, colorSpace,
							flags) != B_OK || bitmap == NULL) {
						continue;
					}

					offscreen->DrawBitmap(bitmap, bitmap->Bounds(), viewRect,
						options);
					invalidRegion.Include(viewRect);
					delete bitmap;
				}

				break;
			}

			case RP_STROKE_ARC:
			case RP_FILL_ARC:
			case RP_FILL_ARC_GRADIENT:
			{
				BRect rect;
				float angle, span;

				message.Read(rect);
				message.Read(angle);
				if (message.Read(span) != B_OK)
					continue;

				if (code == RP_STROKE_ARC) {
					offscreen->StrokeArc(rect, angle, span, pattern);
					rect.InsetBy(-penSize, -penSize);
				} else if (code == RP_FILL_ARC)
					offscreen->FillArc(rect, angle, span, pattern);
				else {
					BGradient *gradient;
					if (message.ReadGradient(&gradient) != B_OK)
						continue;

					offscreen->FillArc(rect, angle, span, *gradient);
					delete gradient;
				}

				invalidRegion.Include(rect);
				break;
			}

			case RP_STROKE_BEZIER:
			case RP_FILL_BEZIER:
			case RP_FILL_BEZIER_GRADIENT:
			{
				BPoint points[4];
				if (message.ReadList(points, 4) != B_OK)
					continue;

				BRect bounds = _BuildInvalidateRect(points, 4);
				if (code == RP_STROKE_BEZIER) {
					offscreen->StrokeBezier(points, pattern);
					bounds.InsetBy(-penSize, -penSize);
				} else if (code == RP_FILL_BEZIER)
					offscreen->FillBezier(points, pattern);
				else {
					BGradient *gradient;
					if (message.ReadGradient(&gradient) != B_OK)
						continue;

					offscreen->FillBezier(points, *gradient);
					delete gradient;
				}

				invalidRegion.Include(bounds);
				break;
			}

			case RP_STROKE_ELLIPSE:
			case RP_FILL_ELLIPSE:
			case RP_FILL_ELLIPSE_GRADIENT:
			{
				BRect rect;
				if (message.Read(rect) != B_OK)
					continue;

				if (code == RP_STROKE_ELLIPSE) {
					offscreen->StrokeEllipse(rect, pattern);
					rect.InsetBy(-penSize, -penSize);
				} else if (code == RP_FILL_ELLIPSE)
					offscreen->FillEllipse(rect, pattern);
				else {
					BGradient *gradient;
					if (message.ReadGradient(&gradient) != B_OK)
						continue;

					offscreen->FillEllipse(rect, *gradient);
					delete gradient;
				}

				invalidRegion.Include(rect);
				break;
			}

			case RP_STROKE_POLYGON:
			case RP_FILL_POLYGON:
			case RP_FILL_POLYGON_GRADIENT:
			{
				BRect bounds;
				bool closed;
				int32 numPoints;

				message.Read(bounds);
				message.Read(closed);
				if (message.Read(numPoints) != B_OK)
					continue;

				BPoint points[numPoints];
				for (int32 i = 0; i < numPoints; i++)
					message.Read(points[i]);

				if (code == RP_STROKE_POLYGON) {
					offscreen->StrokePolygon(points, numPoints, bounds, closed,
						pattern);
					bounds.InsetBy(-penSize, -penSize);
				} else if (code == RP_FILL_POLYGON)
					offscreen->FillPolygon(points, numPoints, bounds, pattern);
				else {
					BGradient *gradient;
					if (message.ReadGradient(&gradient) != B_OK)
						continue;

					offscreen->FillPolygon(points, numPoints, bounds,
						*gradient);
					delete gradient;
				}

				invalidRegion.Include(bounds);
				break;
			}

			case RP_STROKE_RECT:
			case RP_FILL_RECT:
			case RP_FILL_RECT_GRADIENT:
			{
				BRect rect;
				if (message.Read(rect) != B_OK)
					continue;

				if (code == RP_STROKE_RECT) {
					offscreen->StrokeRect(rect, pattern);
					rect.InsetBy(-penSize, -penSize);
				} else if (code == RP_FILL_RECT)
					offscreen->FillRect(rect, pattern);
				else {
					BGradient *gradient;
					if (message.ReadGradient(&gradient) != B_OK)
						continue;

					offscreen->FillRect(rect, *gradient);
					delete gradient;
				}

				invalidRegion.Include(rect);
				break;
			}

			case RP_STROKE_ROUND_RECT:
			case RP_FILL_ROUND_RECT:
			case RP_FILL_ROUND_RECT_GRADIENT:
			{
				BRect rect;
				float xRadius, yRadius;

				message.Read(rect);
				message.Read(xRadius);
				if (message.Read(yRadius) != B_OK)
					continue;

				if (code == RP_STROKE_ROUND_RECT) {
					offscreen->StrokeRoundRect(rect, xRadius, yRadius,
						pattern);
					rect.InsetBy(-penSize, -penSize);
				} else if (code == RP_FILL_ROUND_RECT)
					offscreen->FillRoundRect(rect, xRadius, yRadius, pattern);
				else {
					BGradient *gradient;
					if (message.ReadGradient(&gradient) != B_OK)
						continue;

					offscreen->FillRoundRect(rect, xRadius, yRadius,
						*gradient);
					delete gradient;
				}

				invalidRegion.Include(rect);
				break;
			}

			case RP_STROKE_SHAPE:
			case RP_FILL_SHAPE:
			case RP_FILL_SHAPE_GRADIENT:
			{
				BRect bounds;
				int32 opCount, pointCount;

				message.Read(bounds);
				if (message.Read(opCount) != B_OK)
					continue;

				BMessage archive;
				for (int32 i = 0; i < opCount; i++) {
					int32 op;
					message.Read(op);
					archive.AddInt32("ops", op);
				}

				if (message.Read(pointCount) != B_OK)
					continue;

				for (int32 i = 0; i < pointCount; i++) {
					BPoint point;
					message.Read(point);
					archive.AddPoint("pts", point);
				}

				BPoint offset;
				message.Read(offset);

				float scale;
				if (message.Read(scale) != B_OK)
					continue;

				offscreen->PushState();
				offscreen->MovePenTo(offset);
				offscreen->SetScale(scale);

				BShape shape(&archive);
				if (code == RP_STROKE_SHAPE) {
					offscreen->StrokeShape(&shape, pattern);
					bounds.InsetBy(-penSize, -penSize);
				} else if (code == RP_FILL_SHAPE)
					offscreen->FillShape(&shape, pattern);
				else {
					BGradient *gradient;
					if (message.ReadGradient(&gradient) != B_OK) {
						offscreen->PopState();
						continue;
					}

					offscreen->FillShape(&shape, *gradient);
					delete gradient;
				}

				offscreen->PopState();
				invalidRegion.Include(bounds);
				break;
			}

			case RP_STROKE_TRIANGLE:
			case RP_FILL_TRIANGLE:
			case RP_FILL_TRIANGLE_GRADIENT:
			{
				BRect bounds;
				BPoint points[3];

				message.ReadList(points, 3);
				if (message.Read(bounds) != B_OK)
					continue;

				if (code == RP_STROKE_TRIANGLE) {
					offscreen->StrokeTriangle(points[0], points[1], points[2],
						bounds, pattern);
					bounds.InsetBy(-penSize, -penSize);
				} else if (code == RP_FILL_TRIANGLE) {
					offscreen->FillTriangle(points[0], points[1], points[2],
						bounds, pattern);
				} else {
					BGradient *gradient;
					if (message.ReadGradient(&gradient) != B_OK)
						continue;

					offscreen->FillTriangle(points[0], points[1], points[2],
						bounds, *gradient);
					delete gradient;
				}

				invalidRegion.Include(bounds);
				break;
			}

			case RP_STROKE_LINE:
			{
				BPoint points[2];
				if (message.ReadList(points, 2) != B_OK)
					continue;

				offscreen->StrokeLine(points[0], points[1], pattern);

				BRect bounds = _BuildInvalidateRect(points, 2);
				invalidRegion.Include(bounds.InsetBySelf(-penSize, -penSize));
				break;
			}

			case RP_STROKE_LINE_ARRAY:
			{
				int32 numLines;
				if (message.Read(numLines) != B_OK)
					continue;

				BRect bounds;
				offscreen->BeginLineArray(numLines);
				for (int32 i = 0; i < numLines; i++) {
					rgb_color color;
					BPoint start, end;
					message.ReadArrayLine(start, end, color);
					offscreen->AddLine(start, end, color);

					bounds.left = min_c(bounds.left, min_c(start.x, end.x));
					bounds.top = min_c(bounds.top, min_c(start.y, end.y));
					bounds.right = max_c(bounds.right, max_c(start.x, end.x));
					bounds.bottom = max_c(bounds.bottom, max_c(start.y, end.y));
				}

				offscreen->EndLineArray();
				invalidRegion.Include(bounds);
				break;
			}

			case RP_FILL_REGION:
			case RP_FILL_REGION_GRADIENT:
			{
				BRegion region;
				if (message.ReadRegion(region) != B_OK)
					continue;

				if (code == RP_FILL_REGION)
					offscreen->FillRegion(&region, pattern);
				else {
					BGradient *gradient;
					if (message.ReadGradient(&gradient) != B_OK)
						continue;

					offscreen->FillRegion(&region, *gradient);
					delete gradient;
				}

				invalidRegion.Include(&region);
				break;
			}

			case RP_STROKE_POINT_COLOR:
			{
				BPoint point;
				rgb_color color;

				message.Read(point);
				if (message.Read(color) != B_OK)
					continue;

				rgb_color oldColor = offscreen->HighColor();
				offscreen->SetHighColor(color);
				offscreen->StrokeLine(point, point);
				offscreen->SetHighColor(oldColor);

				invalidRegion.Include(
					BRect(point, point).InsetBySelf(-penSize, -penSize));
				break;
			}

			case RP_STROKE_LINE_1PX_COLOR:
			{
				BPoint points[2];
				rgb_color color;

				message.ReadList(points, 2);
				if (message.Read(color) != B_OK)
					continue;

				float oldSize = offscreen->PenSize();
				rgb_color oldColor = offscreen->HighColor();
				drawing_mode oldMode = offscreen->DrawingMode();
				offscreen->SetPenSize(1);
				offscreen->SetHighColor(color);
				offscreen->SetDrawingMode(B_OP_OVER);

				offscreen->StrokeLine(points[0], points[1]);

				offscreen->SetDrawingMode(oldMode);
				offscreen->SetHighColor(oldColor);
				offscreen->SetPenSize(oldSize);

				invalidRegion.Include(_BuildInvalidateRect(points, 2));
				break;
			}

			case RP_STROKE_RECT_1PX_COLOR:
			case RP_FILL_RECT_COLOR:
			{
				BRect rect;
				rgb_color color;

				message.Read(rect);
				if (message.Read(color) != B_OK)
					continue;

				rgb_color oldColor = offscreen->HighColor();
				offscreen->SetHighColor(color);

				if (code == RP_STROKE_RECT_1PX_COLOR) {
					float oldSize = PenSize();
					offscreen->SetPenSize(1);
					offscreen->StrokeRect(rect);
					offscreen->SetPenSize(oldSize);
				} else
					offscreen->FillRect(rect);

				offscreen->SetHighColor(oldColor);
				invalidRegion.Include(rect);
				break;
			}

			case RP_DRAW_STRING:
			{
				BPoint point;
				size_t length;
				char *string;
				bool hasDelta;

				message.Read(point);
				message.ReadString(&string, length);
				if (message.Read(hasDelta) != B_OK) {
					free(string);
					continue;
				}

				if (hasDelta) {
					escapement_delta delta[length];
					message.ReadList(delta, length);
					offscreen->DrawString(string, point, delta);
				} else
					offscreen->DrawString(string, point);

				free(string);
				reply.Start(RP_DRAW_STRING_RESULT);
				reply.Add(token);
				reply.Add(offscreen->PenLocation());
				reply.Flush();

				font_height height;
				offscreen->GetFontHeight(&height);

				BRect bounds(point, offscreen->PenLocation());
				bounds.top -= height.ascent;
				bounds.bottom += height.descent;
				invalidRegion.Include(bounds);
				break;
			}

			case RP_DRAW_STRING_WITH_OFFSETS:
			{
				size_t length;
				char *string;
				message.ReadString(&string, length);
				int32 count = UTF8CountChars(string, length);

				BPoint offsets[count];
				if (message.ReadList(offsets, count) != B_OK) {
					free(string);
					continue;
				}

				offscreen->DrawString(string, offsets, count);

				reply.Start(RP_DRAW_STRING_RESULT);
				reply.Add(token);
				reply.Add(offscreen->PenLocation());
				reply.Flush();

				BFont font;
				offscreen->GetFont(&font);

				BRect boxes[count];
				font.GetBoundingBoxesAsGlyphs(string, count, B_SCREEN_METRIC,
					boxes);
				free(string);

				font_height height;
				offscreen->GetFontHeight(&height);

				for (int32 i = 0; i < count; i++) {
					// TODO: validate
					boxes[i].OffsetBy(offsets[i] + BPoint(0, -height.ascent));
					invalidRegion.Include(boxes[i]);
				}

				break;
			}

			case RP_READ_BITMAP:
			{
				BRect bounds;
				bool drawCursor;

				message.Read(bounds);
				if (message.Read(drawCursor) != B_OK)
					continue;

				// TODO: support the drawCursor flag
				BBitmap bitmap(bounds, B_BITMAP_NO_SERVER_LINK, B_RGB32);
				bitmap.ImportBits(fOffscreenBitmap, bounds.LeftTop(),
					BPoint(0, 0), bounds.IntegerWidth() + 1,
					bounds.IntegerHeight() + 1);

				reply.Start(RP_READ_BITMAP_RESULT);
				reply.Add(token);
				reply.AddBitmap(&bitmap);
				reply.Flush();
				break;
			}

			default:
				TRACE_ERROR("unknown protocol code: %u\n", code);
				break;
		}

		if (syncDrawing) {
			offscreen->Sync();
			Invalidate(&invalidRegion);
		}
	}
}


BRect
RemoteView::_BuildInvalidateRect(BPoint *points, int32 pointCount)
{
	BRect bounds(1000000, 1000000, 0, 0);
	for (int32 i = 0; i < pointCount; i++) {
		bounds.left = min_c(bounds.left, points[i].x);
		bounds.top = min_c(bounds.top, points[i].y);
		bounds.right = max_c(bounds.right, points[i].x);
		bounds.bottom = max_c(bounds.bottom, points[i].y);
	}

	return bounds;
}
