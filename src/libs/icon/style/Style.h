/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef STYLE_H
#define STYLE_H

#include <GraphicsDefs.h>

#include <agg_color_rgba.h>

#ifdef ICON_O_MATIC
# include "IconObject.h"
# include "Observer.h"
#endif

#include "IconRenderer.h"
	// TODO: put GammaTable into its own file

class Gradient;

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
								Style(BMessage* archive);
#endif
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

			void				SetColor(const rgb_color& color);
	inline	rgb_color			Color() const
									{ return fColor; }

			void				SetGradient(const ::Gradient* gradient);
			::Gradient*			Gradient() const
									{ return fGradient; }

			const agg::rgba8*	Colors() const
									{ return fColors; }

			const agg::rgba8*	GammaCorrectedColors(
									const GammaTable& table) const;

 private:
			rgb_color			fColor;
			::Gradient*			fGradient;

			// hold gradient color array
			agg::rgba8*			fColors;

			// for caching gamma corrected gradient color array
	mutable	agg::rgba8*			fGammaCorrectedColors;
	mutable	bool				fGammaCorrectedColorsValid;
};

#endif // STYLE_H
