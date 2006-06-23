/* 
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 * Copyright 2002-2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved.
 *		
 */

#ifndef COLOR_PICKER_VIEW_H
#define COLOR_PICKER_VIEW_H

#include <View.h>

#if LIB_LAYOUT
#  include <layout.h>
#endif

#include "selected_color_mode.h"

#define	MSG_RADIOBUTTON		'Rad0'
#define	MSG_TEXTCONTROL		'Txt0'
#define MSG_HEXTEXTCONTROL	'HTxt'

class ColorField;
class ColorSlider;
class ColorPreview;

class BRadioButton;
class BTextControl;

class ColorPickerView :
						#if LIB_LAYOUT
						public MView,
						#endif
						public BView {
 public:
								ColorPickerView(const char* name,
												rgb_color color,
												selected_color_mode mode);
	virtual						~ColorPickerView();

	#if LIB_LAYOUT
								// MView
	virtual	minimax				layoutprefs();
	virtual	BRect				layout(BRect frame);
	#endif

								// BView
	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage *message);

	virtual	void				Draw(BRect updateRect);
	virtual	void				Pulse();

								// ColorPickerView
			void				SetColorMode(selected_color_mode mode,
											 bool update = true);
			void				SetColor(rgb_color color);
			rgb_color			Color();
			selected_color_mode	Mode() const
									{ return fSelectedColorMode; }

 private:
			int32				_NumForMode(selected_color_mode mode) const;

			void				_UpdateColor(float value, float value1,
											 float value2);
			void				_UpdateTextControls();
			void				_SetText(BTextControl* control,
										 const char* text,
										 bool* requiresUpdate);


	selected_color_mode			fSelectedColorMode;

	float						h, s, v, r, g, b;
	float						*p, *p1, *p2;

	bool						fRequiresUpdate;

	ColorField*					fColorField;
	ColorSlider*				fColorSlider;
	ColorPreview*				fColorPreview;

	BRadioButton*				fRadioButton[6];
	BTextControl*				fTextControl[6];
	BTextControl*				fHexTextControl;
};

#endif // COLOR_PICKER_VIEW_H


