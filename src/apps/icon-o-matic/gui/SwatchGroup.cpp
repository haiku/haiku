/*
 * Copyright 2006-2012, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "SwatchGroup.h"

#include <stdio.h>

#include "support_ui.h"
#include "ui_defines.h"
#include "rgb_hsv.h"

#include "AlphaSlider.h"
#include "ColorField.h"
#include "ColorPickerPanel.h"
#include "ColorSlider.h"
#include "CurrentColor.h"
#include "Group.h"
#include "SwatchView.h"


enum {
	MSG_SET_COLOR		= 'stcl',
	MSG_COLOR_PICKER	= 'clpk',
	MSG_ALPHA_SLIDER	= 'alps',
};


#define SWATCH_VIEW_WIDTH 20
#define SWATCH_VIEW_HEIGHT 15


SwatchGroup::SwatchGroup(BRect frame)
	:
	BView(frame, "style view", B_FOLLOW_NONE, 0),

	fCurrentColor(NULL),
	fIgnoreNotifications(false),

	fColorPickerPanel(NULL),
	fColorPickerMode(H_SELECTED),
	fColorPickerFrame(100.0, 100.0, 200.0, 200.0)
{
	frame = BRect(0, 0, 100, 15);
	fTopSwatchViews = new Group(frame, "top swatch group");
	fBottomSwatchViews = new Group(frame, "bottom swatch group");

	// create swatch views with rainbow default palette
	float h = 0;
	float s = 1.0;
	float v = 1.0;
	rgb_color color;
	color.alpha = 255;
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	for (int32 i = 0; i < 20; i++) {
		if (i < 10) {
			h = ((float)i / 9.0) * 6.0;
		} else {
			h = ((float)(i - 9) / 10.0) * 6.0;
			v = 0.5;
		}

		HSV_to_RGB(h, s, v, r, g, b);
		color.red = (uint8)(255.0 * r);
		color.green = (uint8)(255.0 * g);
		color.blue = (uint8)(255.0 * b);
		fSwatchViews[i] = new SwatchView("swatch", new BMessage(MSG_SET_COLOR),
			this, color, SWATCH_VIEW_WIDTH, SWATCH_VIEW_HEIGHT);

		fSwatchViews[i]->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);

		if (i < 10)
			fTopSwatchViews->AddChild(fSwatchViews[i]);
		else
			fBottomSwatchViews->AddChild(fSwatchViews[i]);
	}

	// create current color swatch view
	fCurrentColorSV = new SwatchView("current swatch",
		new BMessage(MSG_COLOR_PICKER), this, color, 28.0, 28.0);

	// create color field and slider
	fColorField = new ColorField(BPoint(0.0, 0.0), H_SELECTED,
		1.0, B_HORIZONTAL);
	fColorSlider = new ColorSlider(BPoint(0.0, 0.0), H_SELECTED,
		1.0, 1.0, B_HORIZONTAL);
	fAlphaSlider = new AlphaSlider(B_HORIZONTAL,
		new BMessage(MSG_ALPHA_SLIDER));

	// layout gui
	fTopSwatchViews->SetSpacing(0, 0);
	fTopSwatchViews->ResizeToPreferred();
	fTopSwatchViews->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fBottomSwatchViews->SetSpacing(0, 0);
	fBottomSwatchViews->ResizeToPreferred();
	fBottomSwatchViews->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	float paletteHeight = fBottomSwatchViews->Frame().Height()
		+ fTopSwatchViews->Frame().Height() + 1;

	fTopSwatchViews->MoveTo(paletteHeight + 2, 4);
	fBottomSwatchViews->MoveTo(paletteHeight + 2,
		fTopSwatchViews->Frame().bottom + 1);

	fCurrentColorSV->MoveTo(0, fTopSwatchViews->Frame().top);
	fCurrentColorSV->ResizeTo(paletteHeight, paletteHeight);
	fCurrentColorSV->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);

	float width = fTopSwatchViews->Frame().right
		- fCurrentColorSV->Frame().left;

	fColorField->ResizeTo(width, 40);
	fColorField->FrameResized(width, 40);
	fColorSlider->ResizeTo(width, 11);
	fColorSlider->FrameResized(width, 11);
	fAlphaSlider->ResizeTo(width, 11);
	fAlphaSlider->FrameResized(width, 11);

	fColorField->MoveTo(0, fBottomSwatchViews->Frame().bottom + 3);
	fColorSlider->MoveTo(0, fColorField->Frame().bottom + 1);
	fAlphaSlider->MoveTo(0, fColorSlider->Frame().bottom + 1);
	fAlphaSlider->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	// configure self
	ResizeTo(width, fAlphaSlider->Frame().bottom + 4);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// add views
	AddChild(fCurrentColorSV);
	AddChild(fTopSwatchViews);
	AddChild(fBottomSwatchViews);
	AddChild(fColorField);
	AddChild(fColorSlider);
	AddChild(fAlphaSlider);
}


SwatchGroup::~SwatchGroup()
{
	SetCurrentColor(NULL);
}


void
SwatchGroup::ObjectChanged(const Observable* object)
{
	if (object != fCurrentColor || fIgnoreNotifications)
		return;

	rgb_color color = fCurrentColor->Color();

	float h, s, v;
	RGB_to_HSV(color.red / 255.0, color.green / 255.0, color.blue / 255.0,
		h, s, v);

	_SetColor(h, s, v, color.alpha);
}


// #pragma mark -


void
SwatchGroup::AttachedToWindow()
{
	fColorField->SetTarget(this);
	fColorSlider->SetTarget(this);
	fAlphaSlider->SetTarget(this);
}


void
SwatchGroup::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SET_COLOR:
		{
			rgb_color color;
			if (restore_color_from_message(message, color) == B_OK) {
// TODO: fix color picker panel to respect alpha
if (message->HasRect("panel frame"))
color.alpha = fAlphaSlider->Value();
//
				if (fCurrentColor != NULL)
					fCurrentColor->SetColor(color);
			}
			// if message contains these fields,
			// then it comes from the color picker panel.
			// it also means the panel has died.
			BRect frame;
			selected_color_mode mode;
			if (message->FindRect("panel frame", &frame) == B_OK
				&& message->FindInt32("panel mode", (int32*)&mode) == B_OK) {
				// message came from the color picker panel
				// we remember the settings of the panel for later
				fColorPickerFrame = frame;
				fColorPickerMode = mode;
				// color picker panel has quit
				fColorPickerPanel = NULL;
			}
			break;
		}

		case MSG_COLOR_FIELD:
		{
			// get h from color slider
			float h = ((255 - fColorSlider->Value()) / 255.0) * 6.0;
			float s, v;
			// s and v are comming from the message
			if (message->FindFloat("value", &s) == B_OK
				&& message->FindFloat("value", 1, &v) == B_OK) {
				_SetColor(h, s, v, fAlphaSlider->Value());
			}
			break;
		}

		case MSG_COLOR_SLIDER:
		{
			float h;
			float s, v;
			fColorSlider->GetOtherValues(&s, &v);
			// h is comming from the message
			if (message->FindFloat("value", &h) == B_OK)
				_SetColor(h, s, v, fAlphaSlider->Value());
			break;
		}

		case MSG_ALPHA_SLIDER:
		{
			float h = (1.0 - (float)fColorSlider->Value() / 255.0) * 6;
			float s, v;
			fColorSlider->GetOtherValues(&s, &v);
			_SetColor(h, s, v, fAlphaSlider->Value());
			break;
		}

		case MSG_COLOR_PICKER:
		{
			rgb_color color;
			if (restore_color_from_message(message, color) < B_OK)
				break;

			if (fColorPickerPanel == NULL) {
				fColorPickerPanel = new ColorPickerPanel(fColorPickerFrame,
					color, fColorPickerMode, Window(),
					new BMessage(MSG_SET_COLOR), this);
				fColorPickerPanel->Show();
			} else {
				if (fColorPickerPanel->Lock()) {
					fColorPickerPanel->SetColor(color);
					fColorPickerPanel->Activate();
					fColorPickerPanel->Unlock();
				}
			}
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


// #pragma mark -


void
SwatchGroup::SetCurrentColor(CurrentColor* color)
{
	if (fCurrentColor == color)
		return;

	if (fCurrentColor != NULL)
		fCurrentColor->RemoveObserver(this);

	fCurrentColor = color;

	if (fCurrentColor != NULL) {
		fCurrentColor->AddObserver(this);

		ObjectChanged(fCurrentColor);
	}
}


// #pragma mark -


void
SwatchGroup::_SetColor(rgb_color color)
{
	fCurrentColorSV->SetColor(color);
}


void
SwatchGroup::_SetColor(float h, float s, float v, uint8 a)
{
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	HSV_to_RGB(h, s, v, r, g, b);

	rgb_color color;
	color.red = (uint8)(r * 255.0);
	color.green = (uint8)(g * 255.0);
	color.blue = (uint8)(b * 255.0);
	color.alpha = a;

	if (!fColorField->IsTracking()) {
		fColorField->SetFixedValue(h);
		fColorField->SetMarkerToColor(color);
	}
	if (!fColorSlider->IsTracking()) {
		fColorSlider->SetOtherValues(s, v);
		fColorSlider->SetValue(255 - (int32)((h / 6.0) * 255.0 + 0.5));
	}
	if (!fAlphaSlider->IsTracking()) {
		fAlphaSlider->SetColor(color);
		fAlphaSlider->SetValue(a);
	}

	fIgnoreNotifications = true;

	if (fCurrentColor)
		fCurrentColor->SetColor(color);
	_SetColor(color);

	fIgnoreNotifications = false;
}
