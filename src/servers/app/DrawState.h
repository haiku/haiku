/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@mymail.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _DRAW_STATE_H_
#define _DRAW_STATE_H_


#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <Point.h>
#include <View.h> // for B_FONT_ALL

#include "ServerFont.h"
#include "PatternHandler.h"

class BRegion;

namespace BPrivate {
	class LinkReceiver;
	class LinkSender;
};


class DrawState {
public:
							DrawState();
private:
							DrawState(DrawState* from);
public:
		virtual				~DrawState();

		DrawState*			PushState();
		DrawState*			PopState();
		DrawState*			PreviousState() const { return fPreviousState; }

		void				ReadFontFromLink(BPrivate::LinkReceiver& link);
								// NOTE: ReadFromLink() does not read Font state!!
								// It was separate in ServerWindow, and I didn't
								// want to change it without knowing implications.
		void				ReadFromLink(BPrivate::LinkReceiver& link);
		void				WriteToLink(BPrivate::LinkSender& link) const;

							// coordinate transformation
		void				SetOrigin(const BPoint& origin);
		const BPoint&		Origin() const
								{ return fOrigin; }
		const BPoint&		CombinedOrigin() const
								{ return fCombinedOrigin; }

		void				SetScale(float scale);
		float				Scale() const
								{ return fScale; }
		float				CombinedScale() const
								{ return fCombinedScale; }

							// additional clipping as requested by client
		void				SetClippingRegion(const BRegion* region);

		bool				HasClipping() const;
		bool				HasAdditionalClipping() const;
		bool				GetCombinedClippingRegion(BRegion* region) const;

							// coordinate transformations
				void		Transform(float* x, float* y) const;
				void		InverseTransform(float* x, float* y) const;

				void		Transform(BPoint* point) const;
				void		Transform(BRect* rect) const;
				void		Transform(BRegion* region) const;

				void		InverseTransform(BPoint* point) const;

							// color
		void				SetHighColor(const rgb_color& color);
		const rgb_color&	HighColor() const
								{ return fHighColor; }

		void				SetLowColor(const rgb_color& color);
		const rgb_color&		LowColor() const
								{ return fLowColor; }

		void				SetPattern(const Pattern& pattern);
		const Pattern&		GetPattern() const
								{ return fPattern; }

							// drawing/blending mode
		void				SetDrawingMode(drawing_mode mode);
		drawing_mode		GetDrawingMode() const
								{ return fDrawingMode; }

		void				SetBlendingMode(source_alpha srcMode,
								alpha_function fncMode);
		source_alpha		AlphaSrcMode() const
								{ return fAlphaSrcMode; }
		alpha_function		AlphaFncMode() const
								{ return fAlphaFncMode; }

							// pen
		void				SetPenLocation(const BPoint& location);
		const BPoint&		PenLocation() const;

		void				SetPenSize(float size);
		float				PenSize() const;
		float				UnscaledPenSize() const;

							// font
		void				SetFont(const ServerFont& font,
								uint32 flags = B_FONT_ALL);
		const ServerFont&	Font() const
								{ return fFont; }

		// overrides aliasing flag contained in SeverFont::Flags())
		void				SetForceFontAliasing(bool aliasing);
		bool				ForceFontAliasing() const
								{ return fFontAliasing; }

							// postscript style settings
		void				SetLineCapMode(cap_mode mode);
		cap_mode			LineCapMode() const
								{ return fLineCapMode; }

		void				SetLineJoinMode(join_mode mode);
		join_mode			LineJoinMode() const
								{ return fLineJoinMode; }

		void				SetMiterLimit(float limit);
		float				MiterLimit() const
								{ return fMiterLimit; }

							// convenience functions
		void				PrintToStream() const;

		void				SetSubPixelPrecise(bool precise);
		bool				SubPixelPrecise() const
								{ return fSubPixelPrecise; }

protected:
		BPoint				fOrigin;
		BPoint				fCombinedOrigin;
		float				fScale;
		float				fCombinedScale;

		BRegion*			fClippingRegion;

		rgb_color			fHighColor;
		rgb_color			fLowColor;
		Pattern				fPattern;

		drawing_mode		fDrawingMode;
		source_alpha		fAlphaSrcMode;
		alpha_function		fAlphaFncMode;

		BPoint				fPenLocation;
		float				fPenSize;

		ServerFont			fFont;
		// overrides font aliasing flag
		bool				fFontAliasing;

		// This is not part of the normal state stack.
		// The view will update it in PushState/PopState.
		// A BView can have a flag "B_SUBPIXEL_PRECISE",
		// I never knew what it does on R5, but I can use
		// it in Painter to actually draw stuff with
		// sub-pixel coordinates. It means
		// StrokeLine(BPoint(10, 5), BPoint(20, 9));
		// will look different from
		// StrokeLine(BPoint(10.3, 5.8), BPoint(20.6, 9.5));
		bool				fSubPixelPrecise;

		cap_mode			fLineCapMode;
		join_mode			fLineJoinMode;
		float				fMiterLimit;
		// "internal", used to calculate the size
		// of the font (again) when the scale changes
		float				fUnscaledFontSize;

		DrawState*			fPreviousState;
};

#endif	// _DRAW_STATE_H_
