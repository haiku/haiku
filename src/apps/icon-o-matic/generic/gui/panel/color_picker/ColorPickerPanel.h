/* 
 * Copyright 2002-2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 *		
 */

#ifndef COLOR_PICKER_PANEL_H
#define COLOR_PICKER_PANEL_H

#include "Panel.h"

#include "selected_color_mode.h"

class ColorPickerView;

class ColorPickerPanel : public Panel {
 public:
								ColorPickerPanel(BRect frame,
												 rgb_color color,
												 selected_color_mode mode
												 	= H_SELECTED,
												 BMessage* message = NULL,
												 BWindow* target = NULL);
	virtual						~ColorPickerPanel();

								// Panel
	virtual	void				Cancel();

	virtual	void				MessageReceived(BMessage* message);

			void				SetColor(rgb_color color);

 private:

	ColorPickerView*			fColorPickerView;
	BMessage*					fMessage;
	BWindow*					fTarget;
};

#endif // COLOR_PICKER_PANEL_H
