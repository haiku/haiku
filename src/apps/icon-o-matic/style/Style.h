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

#include "IconObject.h"
#include "Observer.h"

class Gradient;

class Style : public IconObject,
			  public Observer {
 public:
								Style();
	virtual						~Style();


	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// Style
			void				SetColor(const rgb_color& color);
	inline	rgb_color			Color() const
									{ return fColor; }

			void				SetGradient(::Gradient* gradient);
			::Gradient*			Gradient() const
									{ return fGradient; }

			const agg::rgba8*	Colors() const
									{ return fColors; }

 private:
			rgb_color			fColor;
			::Gradient*			fGradient;

			// hold gradient color array
			agg::rgba8*			fColors;
};

#endif // STYLE_H
