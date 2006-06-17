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

#include "Referenceable.h"

class Gradient;

class Style : public Referenceable {
 public:
								Style();
	virtual						~Style();

			void				SetColor(const rgb_color& color);
	inline	rgb_color			Color() const
									{ return fColor; }

			void				SetGradient(::Gradient* gradient);
			::Gradient*			Gradient() const
									{ return fGradient; }

 private:
			rgb_color			fColor;
			::Gradient*			fGradient;
};

#endif // STYLE_H
