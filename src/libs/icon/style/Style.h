/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef STYLE_H
#define STYLE_H


#ifdef ICON_O_MATIC
#	include "IconObject.h"
#	include "Observer.h"
#endif

#include "IconBuild.h"
#include "IconRenderer.h"
	// TODO: put GammaTable into its own file

#include <GraphicsDefs.h>

#include <agg_color_rgba.h>


class BMessage;


_BEGIN_ICON_NAMESPACE


class Gradient;
class Shape;

// TODO: This class can represent solid colors, gradients, and bitmaps. It
// should probably be split into subclasses.
#ifdef ICON_O_MATIC
class Style : public IconObject,
			  public Observer {
#else
class Style {
#endif
 public:
								Style();
								Style(const Style& other);
								Style(const rgb_color& color);
#ifdef ICON_O_MATIC
								Style(BBitmap* image);
									// transfers ownership of the image
#endif
								Style(BMessage* archive);

	virtual						~Style();

#ifdef ICON_O_MATIC
	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// Style
			status_t			Archive(BMessage* into,
										bool deep = true) const;

			bool				operator==(const Style& other) const;
#else
	inline	void				Notify() {}
#endif // ICON_O_MATIC

			bool				HasTransparency() const;

			void				SetColor(const rgb_color& color);
	inline	rgb_color			Color() const
									{ return fColor; }

			void				SetGradient(const _ICON_NAMESPACE Gradient*
											gradient);
			_ICON_NAMESPACE Gradient* Gradient() const
									{ return fGradient; }

#ifdef ICON_O_MATIC
			void				SetBitmap(BBitmap* image);
									// transfers ownership of the image
			BBitmap*			Bitmap() const
									{ return fImage; }

		// alpha only applies to bitmaps
			void				SetAlpha(uint8 alpha)
									{ fAlpha = alpha; Notify(); }
			uint8				Alpha() const
									{ return fAlpha; }
#endif // ICON_O_MATIC

			const agg::rgba8*	Colors() const
									{ return fColors; }

			const agg::rgba8*	GammaCorrectedColors(
									const GammaTable& table) const;

 private:
			rgb_color			fColor;

			_ICON_NAMESPACE Gradient* fGradient;

			// hold gradient color array
			agg::rgba8*			fColors;

#ifdef ICON_O_MATIC
			BBitmap*			fImage;
			uint8				fAlpha;
#endif

			// for caching gamma corrected gradient color array
	mutable	agg::rgba8*			fGammaCorrectedColors;
	mutable	bool				fGammaCorrectedColorsValid;
};


_END_ICON_NAMESPACE


_USING_ICON_NAMESPACE


#endif	// STYLE_H
