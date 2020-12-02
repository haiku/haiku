/*
 * Copyright (c) 2001-2015, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Marcus Overhagen <marcus@overhagen.de>
 *		Adrien Destugues <pulkomandy@pulkomandy.tk>
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 */
#ifndef CANVAS_H
#define CANVAS_H


#include <AutoDeleter.h>
#include <Point.h>

#include "SimpleTransform.h"


class AlphaMask;
class BGradient;
class BRegion;
class DrawingEngine;
class DrawState;
class IntPoint;
class IntRect;
class Layer;
class ServerPicture;
class shape_data;


class Canvas {
public:
							Canvas();
							Canvas(const DrawState& state);
	virtual					~Canvas();

			status_t		InitCheck() const;

	virtual	void			PushState();
	virtual	void			PopState();
			DrawState*		CurrentState() const { return fDrawState.Get(); }
			void			SetDrawState(DrawState* newState);
			DrawState*		DetachDrawState() { return fDrawState.Detach(); }

			void			SetDrawingOrigin(BPoint origin);
			BPoint			DrawingOrigin() const;

			void			SetScale(float scale);
			float			Scale() const;

			void			SetUserClipping(const BRegion* region);
				// region is expected in view coordinates

			bool			ClipToRect(BRect rect, bool inverse);
			void			ClipToShape(shape_data* shape, bool inverse);

			void			SetAlphaMask(AlphaMask* mask);
			AlphaMask*		GetAlphaMask() const;

	virtual	IntRect			Bounds() const = 0;

			SimpleTransform LocalToScreenTransform() const;
			SimpleTransform ScreenToLocalTransform() const;
			SimpleTransform PenToScreenTransform() const;
			SimpleTransform PenToLocalTransform() const;
			SimpleTransform ScreenToPenTransform() const;

			void			BlendLayer(Layer* layer);

	virtual	DrawingEngine*	GetDrawingEngine() const = 0;
	virtual ServerPicture*	GetPicture(int32 token) const = 0;
	virtual	void			RebuildClipping(bool deep) = 0;
	virtual void			ResyncDrawState() {};
	virtual void			UpdateCurrentDrawingRegion() {};

protected:
	virtual	void			_LocalToScreenTransform(
								SimpleTransform& transform) const = 0;
	virtual	void			_ScreenToLocalTransform(
								SimpleTransform& transform) const = 0;

protected:
			ObjectDeleter<DrawState>
							fDrawState;
};


class OffscreenCanvas : public Canvas {
public:
							OffscreenCanvas(DrawingEngine* engine,
								const DrawState& state, const IntRect& bounds);
	virtual					~OffscreenCanvas();

	virtual DrawingEngine*	GetDrawingEngine() const { return fDrawingEngine; }

	virtual void			RebuildClipping(bool deep) { /* TODO */ }
	virtual void			ResyncDrawState();
	virtual void			UpdateCurrentDrawingRegion();
	virtual ServerPicture*	GetPicture(int32 token) const
								{ /* TODO */ return NULL; }
	virtual	IntRect			Bounds() const;

protected:
	virtual	void			_LocalToScreenTransform(SimpleTransform&) const {}
	virtual	void			_ScreenToLocalTransform(SimpleTransform&) const {}

private:
			DrawingEngine*	fDrawingEngine;
			BRegion			fCurrentDrawingRegion;
			IntRect			fBounds;
};


#endif // CANVAS_H
