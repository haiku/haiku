//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		LayerData.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Adi Oanca <adioanca@mymail.ro>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Data classes for working with BView states and draw parameters
//  
//------------------------------------------------------------------------------
#ifndef LAYER_DATA_H_
#define LAYER_DATA_H_

#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <Point.h>
#include <View.h> // for B_FONT_ALL

#include "RGBColor.h"
#include "FontServer.h"
#include "ServerFont.h"
#include "PatternHandler.h"

class BRegion;
namespace BPrivate {
	class LinkReceiver;
	class LinkSender;
};

class DrawData {
 public:
								DrawData();
								DrawData(const DrawData& from);
	virtual						~DrawData();

			DrawData&			operator=(const DrawData& from);

								// coordinate transformation
			void				SetOrigin(const BPoint& origin);
			void				OffsetOrigin(const BPoint& offset);
	inline	const BPoint&		Origin() const
									{ return fOrigin; }

			void				SetScale(float scale);
	inline	float				Scale() const
									{ return fScale; }

								// additional clipping as requested by client
			void				SetClippingRegion(const BRegion& region);
	inline	const BRegion*		ClippingRegion() const
									{ return fClippingRegion; }
/*	inline	int32				CountClippingRects() const
									{ return fClippingRegion ? fClippingRegion.CountRects() : 0; }
	inline	BRect				ClippingRectAt(int32 index) const;*/

								// color
			void				SetHighColor(const RGBColor& color);
	inline	const RGBColor&		HighColor() const
									{ return fHighColor; }

			void				SetLowColor(const RGBColor& color);
	inline	const RGBColor&		LowColor() const
									{ return fLowColor; }

			void				SetPattern(const Pattern& pattern);
	inline	const Pattern&		GetPattern() const
									{ return fPattern; }

								// drawing/blending mode
			void				SetDrawingMode(drawing_mode mode);
	inline	drawing_mode		GetDrawingMode() const
									{ return fDrawingMode; }

			void				SetBlendingMode(source_alpha srcMode,
												alpha_function fncMode);
	inline	source_alpha		AlphaSrcMode() const
									{ return fAlphaSrcMode; }
	inline	alpha_function		AlphaFncMode() const
									{ return fAlphaFncMode; }

								// pen
			void				SetPenLocation(const BPoint& location);
			const BPoint&		PenLocation() const;

			void				SetPenSize(float size);
			float				PenSize() const;

								// font
			void				SetFont(const ServerFont& font,
										uint32 flags = B_FONT_ALL);
	inline	const ServerFont&	Font() const
									{ return fFont; }

// TODO: remove (is contained in SeverFont::Flags())
			void				SetFontAntiAliasing(bool antiAliasing);
	inline	bool				FontAntiAliasing() const
									{ return fFontAntiAliasing; }

// TODO: remove (should be part of DisplayDriver::DrawString() as in BView)
			void				SetEscapementDelta(escapement_delta delta);
	inline	escapement_delta	EscapementDelta() const
									{ return fEscapementDelta; }

								// postscript style settings
			void				SetLineCapMode(cap_mode mode);
	inline	cap_mode			LineCapMode() const
									{ return fLineCapMode; }

			void				SetLineJoinMode(join_mode mode);
	inline	join_mode			LineJoinMode() const
									{ return fLineJoinMode; }

			void				SetMiterLimit(float limit);
	inline	float				MiterLimit() const
									{ return fMiterLimit; }

								// convenience functions
	inline	BPoint				Transform(const BPoint& point) const;
	inline	BRect				Transform(const BRect& rect) const;


 protected:
			BPoint				fOrigin;
			float				fScale;

			BRegion*			fClippingRegion;

			RGBColor			fHighColor;
			RGBColor			fLowColor;
			Pattern				fPattern;

			drawing_mode		fDrawingMode;
			source_alpha		fAlphaSrcMode;
			alpha_function		fAlphaFncMode;

			BPoint				fPenLocation;
			float				fPenSize;

			ServerFont			fFont;
			bool				fFontAntiAliasing;
			escapement_delta	fEscapementDelta;

			cap_mode			fLineCapMode;
			join_mode			fLineJoinMode;
			float				fMiterLimit;

			float				fUnscaledFontSize;
};

class LayerData : public DrawData {
 public:
								LayerData();
								LayerData(const LayerData &data);
	virtual						~LayerData();

			LayerData&			operator=(const LayerData &from);

								// convenience functions
	virtual	void				PrintToStream() const;

			void				ReadFontFromLink(BPrivate::LinkReceiver& link);
								// NOTE: ReadFromLink() does not read Font state!!
								// It was separate in ServerWindow, and I didn't
								// want to change it without knowing implications.
			void				ReadFromLink(BPrivate::LinkReceiver& link);
			void				WriteToLink(BPrivate::LinkSender& link) const;

 public:
			// used for the state stack
			LayerData*			prevState;
};

// inline implementations

// Transform
BPoint
DrawData::Transform(const BPoint& point) const
{
	BPoint p(point);
	p += fOrigin;
	p.x *= fScale;
	p.y *= fScale;
	return p;
}

// Transform
BRect
DrawData::Transform(const BRect& rect) const
{
	return BRect(Transform(rect.LeftTop()),
				 Transform(rect.LeftBottom()));
}
/*
// ClippingRectAt
BRect
DrawData::ClippingRectAt(int32 index) const
{
	BRect r;
	if (fClippingRegion) {
		r = fClippingRegion.RectAt(index);
	}
	return r;
}*/

#endif // LAYER_DATA_H_
