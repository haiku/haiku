/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Canvas.h"

#include <new>
#include <stdio.h>

#include <Bitmap.h>
#include <Entry.h>
#include <Message.h>

#include "Layer.h"

using std::nothrow;

// constructor
Canvas::Canvas(BRect frame)
	: BList(10),
	  fBounds(frame)
{
}

// destructor
Canvas::~Canvas()
{
	MakeEmpty();
}

// IsValid
bool
Canvas::IsValid() const
{
	return fBounds.IsValid();
}

// MakeEmpty
void
Canvas::MakeEmpty()
{
	int32 count = CountLayers();
	for (int32 i = 0; i < count; i++)
		delete LayerAtFast(i);
	BList::MakeEmpty();
}

// AddLayer
bool
Canvas::AddLayer(Layer* layer)
{
	return AddLayer(layer, CountLayers());
}

// AddLayer
bool
Canvas::AddLayer(Layer* layer, int32 index)
{
	return layer && AddItem((void*)layer, index);
}

// RemoveLayer
Layer*
Canvas::RemoveLayer(int32 index)
{
	return (Layer*)RemoveItem(index);
}

// RemoveLayer
bool
Canvas::RemoveLayer(Layer* layer)
{
	return RemoveItem((void*)layer);
}

// LayerAt
Layer*
Canvas::LayerAt(int32 index) const
{
	return (Layer*)ItemAt(index);
}

// LayerAtFast
Layer*
Canvas::LayerAtFast(int32 index) const
{
	return (Layer*)ItemAtFast(index);
}

// IndexOf
int32
Canvas::IndexOf(Layer* layer) const
{
	return BList::IndexOf((void*)layer);
}

// CountLayers
int32
Canvas::CountLayers() const
{
	return CountItems();
}

// HasLayer
bool
Canvas::HasLayer(Layer* layer) const
{
	return HasItem((void*)layer);
}

// SetBounds
void
Canvas::SetBounds(BRect bounds)
{
	if (bounds.IsValid())
		fBounds = bounds;
}

// Bounds
BRect
Canvas::Bounds() const
{
	return fBounds;
}

// Compose
void
Canvas::Compose(BBitmap* into, BRect area) const
{
	if (into && into->IsValid()
		&& area.IsValid() && area.Intersects(into->Bounds())) {
		area = area & into->Bounds();
		int32 count = CountLayers();
		for (int32 i = count - 1; Layer* layer = LayerAt(i); i--) {
			layer->Compose(into, area);
		}
	}
}

// Bitmap
BBitmap*
Canvas::Bitmap() const
{
	BBitmap* bitmap = new BBitmap(fBounds, 0, B_RGBA32);
	if (!bitmap->IsValid()) {
		delete bitmap;
		return NULL;
	}

	// this bitmap is uninitialized, clear to black/fully transparent
	memset(bitmap->Bits(), 0, bitmap->BitsLength());
	Compose(bitmap, fBounds);
	// remove image data where alpha = 0 to improve compression later on
	uint8* bits = (uint8*)bitmap->Bits();
	uint32 bpr = bitmap->BytesPerRow();
	uint32 width = bitmap->Bounds().IntegerWidth() + 1;
	uint32 height = bitmap->Bounds().IntegerHeight() + 1;
	while (height > 0) {
		uint8* bitsHandle = bits;
		for (uint32 x = 0; x < width; x++) {
			if (!bitsHandle[3]) {
				bitsHandle[0] = 0;
				bitsHandle[1] = 0;
				bitsHandle[2] = 0;
			}
			bitsHandle += 4;
		}
		bits += bpr;
		height--;
	}

	return bitmap;
}



static const char*	LAYER_KEY			= "layer";
static const char*	BOUNDS_KEY			= "bounds";

// Unarchive
status_t
Canvas::Unarchive(const BMessage* archive)
{
	if (!archive)
		return B_BAD_VALUE;

	// restore bounds
	BRect bounds;
	if (archive->FindRect(BOUNDS_KEY, &bounds) < B_OK)
		return B_ERROR;

	fBounds = bounds;
	// restore each layer
	BMessage layerMessage;
	for (int32 i = 0;
		 archive->FindMessage(LAYER_KEY, i, &layerMessage) == B_OK;
		 i++) {

		Layer* layer = new (nothrow) Layer();
		if (!layer || layer->Unarchive(&layerMessage) < B_OK
			|| !AddLayer(layer)) {
			delete layer;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}

