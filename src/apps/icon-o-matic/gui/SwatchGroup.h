/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef SWATCH_GROUP_H
#define SWATCH_GROUP_H

#include <View.h>

class ColorField;
class ColorSlider;
class Group;
class SwatchView;

class SwatchGroup : public BView {
 public:
								SwatchGroup(BRect frame);
	virtual						~SwatchGroup();

 private:

			SwatchView*			fCurrentColorSV;
			SwatchView*			fSwatchViews[20];
			ColorField*			fColorField;
			ColorSlider*		fColorSlider;

			Group*				fTopSwatchViews;
			Group*				fBottomSwatchViews;
};

#endif // SWATCH_GROUP_H
