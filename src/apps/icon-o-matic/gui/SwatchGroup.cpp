/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "SwatchGroup.h"

#include "ui_defines.h"
#include "rgb_hsv.h"

#include "ColorField.h"
#include "ColorSlider.h"
#include "Group.h"
#include "SwatchView.h"

enum {
	MSG_SET_COLOR	= 'stcl',
};

// constructor
SwatchGroup::SwatchGroup(BRect frame)
	: BView(frame, "style view", B_FOLLOW_NONE, 0)
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
	float r, g, b;
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
										 this, color, 17.0, 14.0);
		
		if (i < 10)
			fTopSwatchViews->AddChild(fSwatchViews[i]);
		else
			fBottomSwatchViews->AddChild(fSwatchViews[i]);
	}
	// create color field and slider
	color = kBlack;

	fColorField = new ColorField(BPoint(0.0, 0.0), H_SELECTED,
								 1.0, B_HORIZONTAL);
	fColorSlider = new ColorSlider(BPoint(0.0, 0.0), H_SELECTED,
								   1.0, 1.0, B_HORIZONTAL);
	fColorField->SetMarkerToColor(color);
	fColorSlider->SetMarkerToColor(color);

	// layout gui
	fTopSwatchViews->SetSpacing(0, 0);
	fTopSwatchViews->ResizeToPreferred();
	fBottomSwatchViews->SetSpacing(0, 0);
	fBottomSwatchViews->ResizeToPreferred();
	float width = fTopSwatchViews->Frame().Width();
	fColorField->ResizeTo(width, 40);
	fColorField->FrameResized(width, 40);
	fColorSlider->ResizeTo(width, 15);
	fColorSlider->FrameResized(width, 40);

	fTopSwatchViews->MoveTo(0, 4);
	fBottomSwatchViews->MoveTo(0, fTopSwatchViews->Frame().bottom + 1);
	fColorField->MoveTo(0, fBottomSwatchViews->Frame().bottom + 3);
	fColorSlider->MoveTo(0, fColorField->Frame().bottom + 1);

	// configure self
	ResizeTo(width, fColorSlider->Frame().bottom + 4);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// add views
	AddChild(fTopSwatchViews);
	AddChild(fBottomSwatchViews);
	AddChild(fColorField);
	AddChild(fColorSlider);

}

// destructor
SwatchGroup::~SwatchGroup()
{
}

