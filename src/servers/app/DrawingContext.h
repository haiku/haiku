/*
 * Copyright (c) 2001-2014, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Marcus Overhagen <marcus@overhagen.de>
 *		Adrien Destugues <pulkomandy@pulkomandy.tk>
 */
#ifndef DRAWING_CONTEXT_H
#define DRAWING_CONTEXT_H


#include <Point.h>


class AlphaMask;
class BGradient;
class BRegion;
class DrawingEngine;
class DrawState;
class IntPoint;
class IntRect;
class ServerPicture;


class DrawingContext {
public:
							DrawingContext();
							DrawingContext(const DrawState& state);
	virtual					~DrawingContext();

			status_t		InitCheck() const;

	virtual	void			PushState();
	virtual	void			PopState();
			DrawState*		CurrentState() const { return fDrawState; }

			void			SetDrawingOrigin(BPoint origin);
			BPoint			DrawingOrigin() const;

			void			SetScale(float scale);
			float			Scale() const;

			void			SetUserClipping(const BRegion* region);
				// region is expected in view coordinates
	
			void			SetAlphaMask(AlphaMask* mask);
			AlphaMask*		GetAlphaMask() const;
	
			void			ConvertToScreenForDrawing(BPoint* point) const;
			void			ConvertToScreenForDrawing(BRect* rect) const;
			void			ConvertToScreenForDrawing(BRegion* region) const;
			void			ConvertToScreenForDrawing(BGradient* gradient) const;

			void			ConvertToScreenForDrawing(BPoint* dst, const BPoint* src, int32 num) const;
			void			ConvertToScreenForDrawing(BRect* dst, const BRect* src, int32 num) const;
			void			ConvertToScreenForDrawing(BRegion* dst, const BRegion* src, int32 num) const;

			void			ConvertFromScreenForDrawing(BPoint* point) const;
				// used when updating the pen position

	virtual	void			ConvertToScreen(BPoint* point) const = 0;
	virtual	void			ConvertToScreen(IntPoint* point) const = 0;
	virtual	void			ConvertToScreen(BRect* rect) const = 0;
	virtual	void			ConvertToScreen(IntRect* rect) const = 0;
	virtual	void			ConvertToScreen(BRegion* region) const = 0;

	virtual	void			ConvertFromScreen(BPoint* point) const = 0;

	virtual	DrawingEngine*	GetDrawingEngine() const = 0;
	virtual ServerPicture*	GetPicture(int32 token) const = 0;
	virtual	void			RebuildClipping(bool deep) = 0;
	virtual void			ResyncDrawState() {};
	virtual void			UpdateCurrentDrawingRegion() {};

protected:
			DrawState*		fDrawState;
};


class OffscreenContext: public DrawingContext {
public:
							OffscreenContext(DrawingEngine* engine,
								const DrawState& state);

							// Screen and View coordinates are the same for us.
							// DrawState already takes care of World<>View
							// conversions.
	virtual void			ConvertToScreen(BPoint*) const {}
	virtual void			ConvertToScreen(IntPoint*) const {}
	virtual void			ConvertToScreen(BRect*) const {}
	virtual void			ConvertToScreen(IntRect*) const {}
	virtual void			ConvertToScreen(BRegion*) const {}
	virtual void			ConvertFromScreen(BPoint*) const {}

	virtual DrawingEngine*	GetDrawingEngine() const { return fDrawingEngine; }

	virtual void			RebuildClipping(bool deep) { /* TODO */ }
	virtual void			ResyncDrawState();
	virtual ServerPicture*	GetPicture(int32 token) const
								{ /* TODO */ return NULL; }
private:
			DrawingEngine*	fDrawingEngine;
};


#endif
