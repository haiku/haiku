/* 
 * Copyright 2001 Werner Freytag - please read to the LICENSE file
 *
 * Copyright 2002-2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved.
 *		
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <Bitmap.h>
#include <Beep.h>
#include <Font.h>
#include <Message.h>
#include <RadioButton.h>
#include <Screen.h>
#include <TextControl.h>
#include <Window.h>

#include "ColorField.h"
#include "ColorPreview.h"
#include "ColorSlider.h"
#include "rgb_hsv.h"

#include "ColorPickerView.h"


#define round(x) (int)(x+.5)
#define hexdec(str, offset) (int)(((str[offset]<60?str[offset]-48:(str[offset]|32)-87)<<4)|(str[offset+1]<60?str[offset+1]-48:(str[offset+1]|32)-87))

// constructor
ColorPickerView::ColorPickerView(const char* name, rgb_color color,
								 selected_color_mode mode)
	: BView(BRect(0.0, 0.0, 400.0, 277.0), name,
			B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED),
	  h(0.0),
	  s(1.0),
	  v(1.0),
	  r((float)color.red / 255.0),
	  g((float)color.green / 255.0),
	  b((float)color.blue / 255.0),
	  fRequiresUpdate(false)
	  
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	RGB_to_HSV(r, g, b, h, s, v);

	SetColorMode(mode, false);
}

// destructor
ColorPickerView::~ColorPickerView()
{
}

#if LIB_LAYOUT
// layoutprefs
minimax
ColorPickerView::layoutprefs()
{
	mpm.mini.x = mpm.maxi.x = Bounds().Width() + 1.0;
	mpm.mini.y = mpm.maxi.y = Bounds().Height() + 1.0;
	mpm.weight = 1.0;
	return mpm;
}

// layout
BRect
ColorPickerView::layout(BRect frame)
{
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());
	return Frame();
}
#endif // LIB_LAYOUT

// AttachedToWindow
void
ColorPickerView::AttachedToWindow()
{
	rgb_color	color = { (int)(r * 255), (int)(g * 255), (int)(b * 255), 255 };

	BView::AttachedToWindow();

	fColorField = new ColorField(BPoint(10.0, 10.0), fSelectedColorMode, *p);
	fColorField->SetMarkerToColor(color);
	AddChild(fColorField);
	fColorField->SetTarget(this);
	
	fColorSlider = new ColorSlider(BPoint(278.0, 7.0), fSelectedColorMode, *p1, *p2);
	fColorSlider->SetMarkerToColor(color);
	AddChild(fColorSlider);
	fColorSlider->SetTarget(this);

	fColorPreview = new ColorPreview(BRect(0.0, 0.0, 66.0, 70.0).OffsetToCopy(321.0, 10.0), color);
	AddChild(fColorPreview);
	fColorPreview->SetTarget(this);

	BFont font(be_plain_font);
	font.SetSize(10.0);
	SetFont(&font);
	
	const char *title[] = { "H", "S", "V", "R", "G", "B" };
	
	BTextView	*textView;

	int32 selectedRadioButton = _NumForMode(fSelectedColorMode);
	for (int i=0; i<6; ++i) {
	
		fRadioButton[i] = new BRadioButton(BRect(0.0, 0.0, 30.0, 10.0).OffsetToCopy(320.0, 92.0 + 24.0 * i + (int)i/3 * 8),
			NULL, title[i], new BMessage(MSG_RADIOBUTTON + i));
		fRadioButton[i]->SetFont(&font);
		AddChild(fRadioButton[i]);

		fRadioButton[i]->SetTarget(this);

		if (i == selectedRadioButton)
			fRadioButton[i]->SetValue(1);
	}
	 
	for (int i=0; i<6; ++i) {
	
		fTextControl[i] = new BTextControl(BRect(0.0, 0.0, 32.0, 19.0).OffsetToCopy(350.0, 90.0 + 24.0 * i + (int)i/3 * 8),
			NULL, NULL, NULL, new BMessage(MSG_TEXTCONTROL + i));

		textView = fTextControl[i]->TextView();
		textView->SetMaxBytes(3);
		for (int j=32; j<255; ++j) {
			if (j<'0'||j>'9') textView->DisallowChar(j);
		}
		
		fTextControl[i]->SetFont(&font);
		fTextControl[i]->SetDivider(0.0);
		AddChild(fTextControl[i]);

		fTextControl[i]->SetTarget(this);
	}
	 
	fHexTextControl = new BTextControl(BRect(0.0, 0.0, 69.0, 19.0).OffsetToCopy(320.0, 248.0),
			NULL, "#", NULL, new BMessage(MSG_HEXTEXTCONTROL));

	textView = fHexTextControl->TextView();
	textView->SetMaxBytes(6);
	for (int j=32; j<255; ++j) {
		if (!((j>='0' && j<='9') || (j>='a' && j<='f') || (j>='A' && j<='F'))) textView->DisallowChar(j);
	}
	
	fHexTextControl->SetFont(&font);
	fHexTextControl->SetDivider(12.0);
	AddChild(fHexTextControl);

	fHexTextControl->SetTarget(this);
	
	_UpdateTextControls();
}

// MessageReceived
void
ColorPickerView::MessageReceived(BMessage *message)
{
	switch (message->what) {

		case MSG_COLOR_FIELD: {
			float	value1, value2;
			value1 = message->FindFloat("value");
			value2 = message->FindFloat("value", 1);
			_UpdateColor(-1, value1, value2);
			fRequiresUpdate = true;
		} break;
		
		case MSG_COLOR_SLIDER: {
			float	value;
			message->FindFloat("value", &value);
			_UpdateColor(value, -1, -1);
			fRequiresUpdate = true;
		} break;
		
		case MSG_COLOR_PREVIEW: {
			rgb_color	*color;
			ssize_t		numBytes;
			if (message->FindData("color", B_RGB_COLOR_TYPE, (const void **)&color, &numBytes)==B_OK) {
				color->alpha = 255;
				SetColor(*color);
			}
		} break;
		
		case MSG_RADIOBUTTON: {
			SetColorMode(H_SELECTED);
		} break;

		case MSG_RADIOBUTTON + 1: {
			SetColorMode(S_SELECTED);
		} break;

		case MSG_RADIOBUTTON + 2: {
			SetColorMode(V_SELECTED);
		} break;

		case MSG_RADIOBUTTON + 3: {
			SetColorMode(R_SELECTED);
		} break;

		case MSG_RADIOBUTTON + 4: {
			SetColorMode(G_SELECTED);
		} break;

		case MSG_RADIOBUTTON + 5: {
			SetColorMode(B_SELECTED);
		} break;
		
		case MSG_TEXTCONTROL:
		case MSG_TEXTCONTROL + 1:
		case MSG_TEXTCONTROL + 2:
		case MSG_TEXTCONTROL + 3:
		case MSG_TEXTCONTROL + 4:
		case MSG_TEXTCONTROL + 5: {

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

			rgb_color color = { round(r*255), round(g*255), round(b*255), 255 };

			SetColor(color);
			
		} break;
		
		case MSG_HEXTEXTCONTROL: {
			if (fHexTextControl->TextView()->TextLength()==6) {
				const char *string = fHexTextControl->TextView()->Text();
				rgb_color color = { hexdec(string, 0), hexdec(string, 2), hexdec(string, 4), 255 };
				SetColor(color);
			}
		} break;

		default:
			BView::MessageReceived(message);
	}
}

// Draw
void
ColorPickerView::Draw(BRect updateRect)
{
	// raised border
	BRect r(Bounds());
	if (updateRect.Intersects(r)) {
		rgb_color light = tint_color(LowColor(), B_LIGHTEN_MAX_TINT);
		rgb_color shadow = tint_color(LowColor(), B_DARKEN_2_TINT);

		BeginLineArray(4);
			AddLine(BPoint(r.left, r.bottom),
					BPoint(r.left, r.top), light);
			AddLine(BPoint(r.left + 1.0, r.top),
					BPoint(r.right, r.top), light);
			AddLine(BPoint(r.right, r.top + 1.0),
					BPoint(r.right, r.bottom), shadow);
			AddLine(BPoint(r.right - 1.0, r.bottom),
					BPoint(r.left + 1.0, r.bottom), shadow);
		EndLineArray();
		// exclude border from update rect
		r.InsetBy(1.0, 1.0);
		updateRect = r & updateRect;
	}

	// some additional labels
	font_height fh;
	GetFontHeight(&fh);
	
	const char *title[] = { "°", "%", "%" };
	for (int i = 0; i < 3; ++i) {
		DrawString(title[i], 
				   BPoint(385.0, 93.0 + 24.0 * i + (int)i / 3 * 8 + fh.ascent));
	}
}

// Pulse
void
ColorPickerView::Pulse()
{
	if (fRequiresUpdate)
		_UpdateTextControls();
}

// SetColorMode
void
ColorPickerView::SetColorMode(selected_color_mode mode, bool update)
{
	fSelectedColorMode = mode;
	switch (mode) {
		case R_SELECTED:
			p = &r; p1 = &g; p2 = &b;
		break;

		case G_SELECTED:
			p = &g; p1 = &r; p2 = &b;
		break;

		case B_SELECTED:
			p = &b; p1 = &r; p2 = &g;
		break;

		case H_SELECTED:
			p = &h; p1 = &s; p2 = &v;
		break;

		case S_SELECTED:
			p = &s; p1 = &h; p2 = &v;
		break;

		case V_SELECTED:
			p = &v; p1 = &h; p2 = &s;
		break;
	}
	
	if (!update) return;

	fColorSlider->SetModeAndValues(fSelectedColorMode, *p1, *p2);
	fColorField->SetModeAndValue(fSelectedColorMode, *p);
	
}

// SetColor
void
ColorPickerView::SetColor(rgb_color color)
{
	r = (float)color.red/255; g = (float)color.green/255; b = (float)color.blue/255;
	RGB_to_HSV(r, g, b, h, s, v);
	
	fColorSlider->SetModeAndValues(fSelectedColorMode, *p1, *p2);
	fColorSlider->SetMarkerToColor(color);

	fColorField->SetModeAndValue(fSelectedColorMode, *p);
	fColorField->SetMarkerToColor(color);

	fColorPreview->SetColor(color);
	
	fRequiresUpdate = true;
}

// Color
rgb_color
ColorPickerView::Color()
{
	if (fSelectedColorMode & (R_SELECTED|G_SELECTED|B_SELECTED))
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

// _NumForMode
int32
ColorPickerView::_NumForMode(selected_color_mode mode) const
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

// _UpdateColor
void
ColorPickerView::_UpdateColor(float value, float value1, float value2)
{
	if (value!=-1) {
		fColorField->SetFixedValue(value);
		*p = value;
	}
	else if (value1!=-1 && value2!=-1) {
		fColorSlider->SetOtherValues(value1, value2);
		*p1 = value1; *p2 = value2;
	}

	if (fSelectedColorMode & (R_SELECTED|G_SELECTED|B_SELECTED))
		RGB_to_HSV(r, g, b, h, s, v);
	else
		HSV_to_RGB(h, s, v, r, g, b);

	rgb_color color = { (int)(r*255), (int)(g*255), (int)(b*255), 255 };
	fColorPreview->SetColor(color);
}

// _UpdateTextControls
void
ColorPickerView::_UpdateTextControls()
{
	Window()->DisableUpdates();

	char	string[10];

	fRequiresUpdate = false;

	sprintf(string, "%d", round(h*60));
	_SetText(fTextControl[0], string, &fRequiresUpdate);
	
	sprintf(string, "%d", round(s*100));
	_SetText(fTextControl[1], string, &fRequiresUpdate);

	sprintf(string, "%d", round(v*100));
	_SetText(fTextControl[2], string, &fRequiresUpdate);

	sprintf(string, "%d", round(r*255));
	_SetText(fTextControl[3], string, &fRequiresUpdate);
	
	sprintf(string, "%d", round(g*255));
	_SetText(fTextControl[4], string, &fRequiresUpdate);

	sprintf(string, "%d", round(b*255));
	_SetText(fTextControl[5], string, &fRequiresUpdate);

	sprintf(string, "%.6X", (round(r*255)<<16)|(round(g*255)<<8)|round(b*255));
	_SetText(fHexTextControl, string, &fRequiresUpdate);

	Window()->EnableUpdates();
}

// _SetText
void
ColorPickerView::_SetText(BTextControl* control, const char* text,
						  bool* requiresUpdate)
{
	if (strcmp(control->Text(), text) != 0) {
		// this textview needs updating
		if (!control->TextView()->IsFocus())
			// but don't screw with user while she is typing
			control->SetText(text);
		else
			*requiresUpdate = true;
	}
}

