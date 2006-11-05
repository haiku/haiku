/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef CANVAS_H
#define CANVAS_H

#include <List.h>
#include <Rect.h>
#include <String.h>

struct entry_ref;

class BBitmap;
class BMessage;
class Layer;

class Canvas : private BList {
 public:
								Canvas(BRect frame);
								~Canvas();

			bool				IsValid() const;
			void				MakeEmpty();

	// list functionality
			bool				AddLayer(Layer* layer);
			bool				AddLayer(Layer* layer, int32 index);

			Layer*				RemoveLayer(int32 index);
			bool				RemoveLayer(Layer* layer);

			Layer*				LayerAt(int32 index) const;
			Layer*				LayerAtFast(int32 index) const;
			int32				IndexOf(Layer* layer) const;
			int32				CountLayers() const;
			bool				HasLayer(Layer* layer) const;

			void				SetBounds(BRect bounds);
			BRect				Bounds() const;

								// composes layers on top of passed bitmap
			void				Compose(BBitmap* into, BRect area) const;
								// returns entire composition in new bitmap
			BBitmap*			Bitmap() const;

	// loading
			status_t			Unarchive(const BMessage* archive);

 private:
	BRect						fBounds;
};

#endif // CANVAS_H
