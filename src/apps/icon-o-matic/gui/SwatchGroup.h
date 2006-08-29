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

#include "selected_color_mode.h"

#include "Observer.h"

class AlphaSlider;
class ColorField;
class ColorPickerPanel;
class ColorSlider;
class CurrentColor;
class Group;
class SwatchView;

class SwatchGroup : public BView,
					public Observer {
 public:
								SwatchGroup(BRect frame);
	virtual						~SwatchGroup();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

	// SwatchGroup
			void				SetCurrentColor(CurrentColor* color);

 private:
			void				_SetColor(rgb_color color);
			void				_SetColor(float h, float s, float v,
										  uint8 a);

			SwatchView*			fCurrentColorSV;
			SwatchView*			fSwatchViews[20];
			ColorField*			fColorField;
			ColorSlider*		fColorSlider;
			AlphaSlider*		fAlphaSlider;

			Group*				fTopSwatchViews;
			Group*				fBottomSwatchViews;

			CurrentColor*		fCurrentColor;
			bool				fIgnoreNotifications;

			ColorPickerPanel*	fColorPickerPanel;
			selected_color_mode fColorPickerMode;
			BRect				fColorPickerFrame;
};

#endif // SWATCH_GROUP_H
