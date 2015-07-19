/*
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 * Copyright 2002-2015, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved.
 *
 */

#ifndef COLOR_PICKER_VIEW_H
#define COLOR_PICKER_VIEW_H

#include <String.h>
#include <View.h>

#include "SelectedColorMode.h"

#define	MSG_RADIOBUTTON					'Rad0'
#define	MSG_TEXTCONTROL					'Txt0'
#define MSG_HEXTEXTCONTROL				'HTxt'
#define MSG_UPDATE_COLOR_PICKER_VIEW	'UpCp'

class ColorField;
class ColorSlider;
class ColorPreview;

class BRadioButton;
class BTextControl;

class ColorPickerView : public BView {
public:
								ColorPickerView(const char* name,
									rgb_color color,
									SelectedColorMode mode);
	virtual						~ColorPickerView();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage *message);

	// ColorPickerView
			void				SetColorMode(SelectedColorMode mode,
									bool update = true);
			void				SetColor(rgb_color color);
			rgb_color			Color();
			SelectedColorMode	Mode() const
									{ return fSelectedColorMode; }

private:
			int32				_NumForMode(SelectedColorMode mode) const;

			void				_UpdateColor(float value, float value1,
									float value2);
			void				_UpdateTextControls();

			int					_TextControlValue(int32 index);
			bool				_SetTextControlValue(int32 index, int value);
			BString				_HexTextControlString() const;
			bool				_SetHexTextControlString(const BString& text);
			bool				_SetText(BTextControl* control,
									const BString& text);


private:
	SelectedColorMode			fSelectedColorMode;

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


