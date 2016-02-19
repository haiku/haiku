/*
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 * Copyright 2002-2015, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <Bitmap.h>
#include <Beep.h>
#include <ControlLook.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <MessageRunner.h>
#include <RadioButton.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

#include "ColorField.h"
#include "ColorPreview.h"
#include "ColorSlider.h"
#include "rgb_hsv.h"

#include "ColorPickerView.h"


#define round(x) (int)(x+.5)
#define hexdec(str, offset) (int)(((str[offset]<60?str[offset]-48:(str[offset]|32)-87)<<4)|(str[offset+1]<60?str[offset+1]-48:(str[offset+1]|32)-87))


ColorPickerView::ColorPickerView(const char* name, rgb_color color,
		SelectedColorMode mode)
	:
	BView(name, 0),
	h(0.0),
	s(1.0),
	v(1.0),
	r((float)color.red / 255.0),
	g((float)color.green / 255.0),
	b((float)color.blue / 255.0),
	fRequiresUpdate(false)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	RGB_to_HSV(r, g, b, h, s, v);

	SetColorMode(mode, false);
}


ColorPickerView::~ColorPickerView()
{
}


void
ColorPickerView::AttachedToWindow()
{
	rgb_color color = { (uint8)(r * 255), (uint8)(g * 255), (uint8)(b * 255),
		255 };
	BView::AttachedToWindow();

	fColorField = new ColorField(fSelectedColorMode, *p);
	fColorField->SetMarkerToColor(color);
	fColorField->SetTarget(this);

	fColorSlider = new ColorSlider(fSelectedColorMode, *p1, *p2);
	fColorSlider->SetMarkerToColor(color);
	fColorSlider->SetTarget(this);

	fColorPreview = new ColorPreview(color);
	fColorPreview->SetTarget(this);

	fColorField->SetExplicitMinSize(BSize(256, 256));
	fColorField->SetExplicitMaxSize(
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	fColorSlider->SetExplicitMaxSize(
		BSize(B_SIZE_UNSET, B_SIZE_UNLIMITED));
	fColorPreview->SetExplicitMinSize(BSize(B_SIZE_UNSET, 70));
	
	const char* title[] = { "H", "S", "V", "R", "G", "B" };

	int32 selectedRadioButton = _NumForMode(fSelectedColorMode);

	for (int i = 0; i < 6; i++) {
		fRadioButton[i] = new BRadioButton(NULL, title[i],
			new BMessage(MSG_RADIOBUTTON + i));
		fRadioButton[i]->SetTarget(this);

		if (i == selectedRadioButton)
			fRadioButton[i]->SetValue(1);

		fTextControl[i] = new BTextControl(NULL, NULL, NULL,
			new BMessage(MSG_TEXTCONTROL + i));

		fTextControl[i]->TextView()->SetMaxBytes(3);
		for (int j = 32; j < 255; ++j) {
			if (j < '0' || j > '9')
				fTextControl[i]->TextView()->DisallowChar(j);
		}
	}

	fHexTextControl = new BTextControl(NULL, "#", NULL,
		new BMessage(MSG_HEXTEXTCONTROL));

	fHexTextControl->TextView()->SetMaxBytes(6);
	for (int j = 32; j < 255; ++j) {
		if (!((j >= '0' && j <= '9') || (j >= 'a' && j <= 'f')
			|| (j >= 'A' && j <= 'F'))) {
			fHexTextControl->TextView()->DisallowChar(j);
		}
	}

	const float inset = be_control_look->DefaultLabelSpacing();
	BSize separatorSize(B_SIZE_UNSET, inset / 2);
	BAlignment separatorAlignment(B_ALIGN_LEFT, B_ALIGN_TOP);

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	BLayoutBuilder::Group<>(this, B_HORIZONTAL)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(fColorField)
		.SetInsets(3, 3, 0, 3)
		.End()
		.Add(fColorSlider)
		.AddGroup(B_VERTICAL)
			.Add(fColorPreview)
			.AddGrid(inset / 2, inset / 2)
				.Add(fRadioButton[0], 0, 0)
				.Add(fTextControl[0], 1, 0)
				.Add(new BStringView(NULL, "°"), 2, 0)

				.Add(fRadioButton[1], 0, 1)
				.Add(fTextControl[1], 1, 1)
				.Add(new BStringView(NULL, "%"), 2, 1)

				.Add(fRadioButton[2], 0, 2)
				.Add(fTextControl[2], 1, 2)
				.Add(new BStringView(NULL, "%"), 2, 2)

				.Add(new BSpaceLayoutItem(separatorSize, separatorSize,
					separatorSize, separatorAlignment),
					0, 3, 3)

				.Add(fRadioButton[3], 0, 4)
				.Add(fTextControl[3], 1, 4)

				.Add(fRadioButton[4], 0, 5)
				.Add(fTextControl[4], 1, 5)

				.Add(fRadioButton[5], 0, 6)
				.Add(fTextControl[5], 1, 6)

				.Add(new BSpaceLayoutItem(separatorSize, separatorSize,
					separatorSize, separatorAlignment),
					0, 7, 3)
				
				.AddGroup(B_HORIZONTAL, 0.0f, 0, 8, 2)
					.Add(fHexTextControl->CreateLabelLayoutItem())
					.Add(fHexTextControl->CreateTextViewLayoutItem())
				.End()
			.End()
		.SetInsets(0, 3, 3, 3)
		.End()
		.SetInsets(inset, inset, inset, inset)
	;

	// After the views are attached, configure their target
	for (int i = 0; i < 6; i++) {
		fRadioButton[i]->SetTarget(this);
		fTextControl[i]->SetTarget(this);
	}
	fHexTextControl->SetTarget(this);

	_UpdateTextControls();
}


void
ColorPickerView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MSG_UPDATE_COLOR_PICKER_VIEW:
			if (fRequiresUpdate)
				_UpdateTextControls();
			break;

		case MSG_COLOR_FIELD:
		{
			float value1, value2;
			value1 = message->FindFloat("value");
			value2 = message->FindFloat("value", 1);
			_UpdateColor(-1, value1, value2);
			fRequiresUpdate = true;
			break;
		}

		case MSG_COLOR_SLIDER:
		{
			float value;
			message->FindFloat("value", &value);
			_UpdateColor(value, -1, -1);
			fRequiresUpdate = true;
			break;
		}

		case MSG_COLOR_PREVIEW:
		{
			rgb_color* color;
			ssize_t numBytes;
			if (message->FindData("color", B_RGB_COLOR_TYPE,
					(const void**)&color, &numBytes) == B_OK) {
				color->alpha = 255;
				SetColor(*color);
			}
			break;
		}

		case MSG_RADIOBUTTON:
			SetColorMode(H_SELECTED);
			break;

		case MSG_RADIOBUTTON + 1:
			SetColorMode(S_SELECTED);
			break;

		case MSG_RADIOBUTTON + 2:
			SetColorMode(V_SELECTED);
			break;

		case MSG_RADIOBUTTON + 3:
			SetColorMode(R_SELECTED);
			break;

		case MSG_RADIOBUTTON + 4:
			SetColorMode(G_SELECTED);
			break;

		case MSG_RADIOBUTTON + 5:
			SetColorMode(B_SELECTED);
			break;

		case MSG_TEXTCONTROL:
		case MSG_TEXTCONTROL + 1:
		case MSG_TEXTCONTROL + 2:
		case MSG_TEXTCONTROL + 3:
		case MSG_TEXTCONTROL + 4:
		case MSG_TEXTCONTROL + 5:
		{
			int nr = message->what - MSG_TEXTCONTROL;
			int value = atoi(fTextControl[nr]->Text());

			char string[4];

			switch (nr) {
				case 0: {
					value %= 360;
					sprintf(string, "%d", value);
					h = (float)value / 60;
				} break;

				case 1: {
					value = min_c(value, 100);
					sprintf(string, "%d", value);
					s = (float)value / 100;
				} break;

				case 2: {
					value = min_c(value, 100);
					sprintf(string, "%d", value);
					v = (float)value / 100;
				} break;

				case 3: {
					value = min_c(value, 255);
					sprintf(string, "%d", value);
					r = (float)value / 255;
				} break;

				case 4: {
					value = min_c(value, 255);
					sprintf(string, "%d", value);
					g = (float)value / 255;
				} break;

				case 5: {
					value = min_c(value, 255);
					sprintf(string, "%d", value);
					b = (float)value / 255;
				} break;
			}

			if (nr<3) { // hsv-mode
				HSV_to_RGB(h, s, v, r, g, b);
			}

			rgb_color color = { (uint8)round(r * 255), (uint8)round(g * 255),
								(uint8)round(b * 255), 255 };

			SetColor(color);
			break;
		}

		case MSG_HEXTEXTCONTROL:
		{
			BString string = _HexTextControlString();
			if (string.Length() == 6) {
				rgb_color color = {
					(uint8)hexdec(string, 0),
					(uint8)hexdec(string, 2),
					(uint8)hexdec(string, 4), 255 };
				SetColor(color);
			}
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}


void
ColorPickerView::SetColorMode(SelectedColorMode mode, bool update)
{
	fSelectedColorMode = mode;
	switch (mode) {
		case R_SELECTED:
			p = &r;
			p1 = &g;
			p2 = &b;
			break;

		case G_SELECTED:
			p = &g;
			p1 = &r;
			p2 = &b;
			break;

		case B_SELECTED:
			p = &b;
			p1 = &r;
			p2 = &g;
			break;

		case H_SELECTED:
			p = &h;
			p1 = &s;
			p2 = &v;
			break;

		case S_SELECTED:
			p = &s;
			p1 = &h;
			p2 = &v;
			break;

		case V_SELECTED:
			p = &v;
			p1 = &h;
			p2 = &s;
			break;
	}

	if (!update)
		return;

	fColorSlider->SetModeAndValues(fSelectedColorMode, *p1, *p2);
	fColorField->SetModeAndValue(fSelectedColorMode, *p);

}

// SetColor
void
ColorPickerView::SetColor(rgb_color color)
{
	r = (float)color.red / 255;
	g = (float)color.green / 255;
	b = (float)color.blue / 255;
	RGB_to_HSV(r, g, b, h, s, v);

	fColorSlider->SetModeAndValues(fSelectedColorMode, *p1, *p2);
	fColorSlider->SetMarkerToColor(color);

	fColorField->SetModeAndValue(fSelectedColorMode, *p);
	fColorField->SetMarkerToColor(color);

	fColorPreview->SetColor(color);

	_UpdateTextControls();
}


rgb_color
ColorPickerView::Color()
{
	if (fSelectedColorMode & (R_SELECTED | G_SELECTED | B_SELECTED))
		RGB_to_HSV(r, g, b, h, s, v);
	else
		HSV_to_RGB(h, s, v, r, g, b);

	rgb_color color;
	color.red = (uint8)round(r * 255.0);
	color.green = (uint8)round(g * 255.0);
	color.blue = (uint8)round(b * 255.0);
	color.alpha = 255;

	return color;
}


int32
ColorPickerView::_NumForMode(SelectedColorMode mode) const
{
	int32 num = -1;
	switch (mode) {
		case H_SELECTED:
			num = 0;
			break;
		case S_SELECTED:
			num = 1;
			break;
		case V_SELECTED:
			num = 2;
			break;
		case R_SELECTED:
			num = 3;
			break;
		case G_SELECTED:
			num = 4;
			break;
		case B_SELECTED:
			num = 5;
			break;
	}
	return num;
}


void
ColorPickerView::_UpdateColor(float value, float value1, float value2)
{
	if (value != -1) {
		fColorField->SetFixedValue(value);
		*p = value;
	} else if (value1 != -1 && value2 != -1) {
		fColorSlider->SetOtherValues(value1, value2);
		*p1 = value1; *p2 = value2;
	}

	if (fSelectedColorMode & (R_SELECTED|G_SELECTED|B_SELECTED))
		RGB_to_HSV(r, g, b, h, s, v);
	else
		HSV_to_RGB(h, s, v, r, g, b);

	rgb_color color = { (uint8)(r * 255), (uint8)(g * 255),
		(uint8)(b * 255), 255 };
	fColorPreview->SetColor(color);
}


void
ColorPickerView::_UpdateTextControls()
{
	bool updateRequired = false;
	updateRequired |= _SetTextControlValue(0, round(h * 60));
	updateRequired |= _SetTextControlValue(1, round(s * 100));
	updateRequired |= _SetTextControlValue(2, round(v * 100));
	updateRequired |= _SetTextControlValue(3, round(r * 255));
	updateRequired |= _SetTextControlValue(4, round(g * 255));
	updateRequired |= _SetTextControlValue(5, round(b * 255));

	BString hexString;
	hexString.SetToFormat("%.6X",
		(round(r * 255) << 16) | (round(g * 255) << 8) | round(b * 255));
	updateRequired |= _SetHexTextControlString(hexString);

	fRequiresUpdate = updateRequired;
	if (fRequiresUpdate) {
		// Couldn't set all values. Try again later.
		BMessage message(MSG_UPDATE_COLOR_PICKER_VIEW);
		BMessageRunner::StartSending(this, &message, 500000, 1);
	}
}


int
ColorPickerView::_TextControlValue(int32 index)
{
	return atoi(fTextControl[index]->Text());
}


// Returns whether the value needs to be set later, since it is currently
// being edited by the user.
bool
ColorPickerView::_SetTextControlValue(int32 index, int value)
{
	BString text;
	text << value;
	return _SetText(fTextControl[index], text);
}


BString
ColorPickerView::_HexTextControlString() const
{
	return fHexTextControl->TextView()->Text();
}


// Returns whether the value needs to be set later, since it is currently
// being edited by the user.
bool
ColorPickerView::_SetHexTextControlString(const BString& text)
{
	return _SetText(fHexTextControl, text);
}


// Returns whether the value needs to be set later, since it is currently
// being edited by the user.
bool
ColorPickerView::_SetText(BTextControl* control, const BString& text)
{
	if (text == control->Text())
		return false;

	// This textview needs updating, but don't screw with user while she is
	// typing.
	if (control->TextView()->IsFocus())
		return true;

	control->SetText(text.String());
	return false;
}

