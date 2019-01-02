/*
 * Copyright 2001-2018, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@mymail.ro>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 *		Joseph Groover <looncraz@looncraz.net>
 */
#ifndef _DRAW_STATE_H_
#define _DRAW_STATE_H_


#include <AffineTransform.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <Point.h>
#include <Referenceable.h>
#include <View.h>

#include "ServerFont.h"
#include "PatternHandler.h"
#include "SimpleTransform.h"

class AlphaMask;
class BRegion;
class shape_data;

namespace BPrivate {
	class LinkReceiver;
	class LinkSender;
};


class DrawState {
public:
							DrawState();
							DrawState(const DrawState& other);
public:
		virtual				~DrawState();

		DrawState*			PushState();
		DrawState*			PopState();
		DrawState*			PreviousState() const { return fPreviousState; }

		uint16				ReadFontFromLink(BPrivate::LinkReceiver& link);
								// NOTE: ReadFromLink() does not read Font state!!
								// It was separate in ServerWindow, and I didn't
								// want to change it without knowing implications.
		void				ReadFromLink(BPrivate::LinkReceiver& link);
		void				WriteToLink(BPrivate::LinkSender& link) const;

							// coordinate transformation
		void				SetOrigin(BPoint origin);
		BPoint				Origin() const
								{ return fOrigin; }
		BPoint				CombinedOrigin() const
								{ return fCombinedOrigin; }

		void				SetScale(float scale);
		float				Scale() const
								{ return fScale; }
		float				CombinedScale() const
								{ return fCombinedScale; }

		void				SetTransform(BAffineTransform transform);
		BAffineTransform	Transform() const
								{ return fTransform; }
		BAffineTransform	CombinedTransform() const
								{ return fCombinedTransform; }
		void				SetTransformEnabled(bool enabled);

		DrawState*			Squash() const;

							// additional clipping as requested by client
		void				SetClippingRegion(const BRegion* region);

		bool				HasClipping() const;
		bool				HasAdditionalClipping() const;
		bool				GetCombinedClippingRegion(BRegion* region) const;

		bool				ClipToRect(BRect rect, bool inverse);
		void				ClipToShape(shape_data* shape, bool inverse);

			void			SetAlphaMask(AlphaMask* mask);
			AlphaMask*		GetAlphaMask() const;

							// coordinate transformations
				void		Transform(SimpleTransform& transform) const;
				void		InverseTransform(SimpleTransform& transform) const;

							// color
		void				SetHighColor(rgb_color color);
		rgb_color			HighColor() const
								{ return fHighColor; }

		void				SetLowColor(rgb_color color);
		rgb_color			LowColor() const
								{ return fLowColor; }

		void				SetHighUIColor(color_which which, float tint);
		color_which			HighUIColor(float* tint) const;

		void				SetLowUIColor(color_which which, float tint);
		color_which			LowUIColor(float* tint) const;

		void				SetPattern(const Pattern& pattern);
		const Pattern&		GetPattern() const
								{ return fPattern; }

							// drawing/blending mode
		bool				SetDrawingMode(drawing_mode mode);
		drawing_mode		GetDrawingMode() const
								{ return fDrawingMode; }

		bool				SetBlendingMode(source_alpha srcMode,
								alpha_function fncMode);
		source_alpha		AlphaSrcMode() const
								{ return fAlphaSrcMode; }
		alpha_function		AlphaFncMode() const
								{ return fAlphaFncMode; }

		void				SetDrawingModeLocked(bool locked);

							// pen
		void				SetPenLocation(BPoint location);
		BPoint				PenLocation() const;

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

		void				SetFillRule(int32 fillRule);
		int32				FillRule() const
								{ return fFillRule; }

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
		BAffineTransform	fTransform;
		BAffineTransform	fCombinedTransform;

		BRegion*			fClippingRegion;

		BReference<AlphaMask> fAlphaMask;

		rgb_color			fHighColor;
		rgb_color			fLowColor;

		color_which			fWhichHighColor;
		color_which			fWhichLowColor;
		float				fWhichHighColorTint;
		float				fWhichLowColorTint;
		Pattern				fPattern;

		drawing_mode		fDrawingMode;
		source_alpha		fAlphaSrcMode;
		alpha_function		fAlphaFncMode;
		bool				fDrawingModeLocked;

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
		int32				fFillRule;
		// "internal", used to calculate the size
		// of the font (again) when the scale changes
		float				fUnscaledFontSize;

		DrawState*			fPreviousState;
};

#endif	// _DRAW_STATE_H_
