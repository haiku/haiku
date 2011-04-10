/*
 * Copyright 2006, 2011, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef CURRENT_COLOR_H
#define CURRENT_COLOR_H


#include <GraphicsDefs.h>

#include "Observable.h"


class CurrentColor : public Observable {
public:
								CurrentColor();
	virtual						~CurrentColor();

			void				SetColor(rgb_color color);
	inline	rgb_color			Color() const
									{ return fColor; }

private:
			rgb_color			fColor;
};


#endif // CURRENT_COLOR_H
