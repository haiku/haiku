/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef CURRENT_COLOR_H
#define CURRENT_COLOR_H

#include <GraphicsDefs.h>

#include "Observable.h"

class CurrentColor : public Observable {
 public:
								CurrentColor();
	virtual						~CurrentColor();

	static	CurrentColor*		Default();

			void				SetColor(rgb_color color);
	inline	rgb_color			Color() const
									{ return fColor; }

 private:
			rgb_color			fColor;

	static	CurrentColor		fDefaultInstance;
};

#endif // CURRENT_COLOR_H
