/*
 * Copyright 2002-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Copyright 1999, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 * Written by:	Daniel Switkin
 */


#include "ConfigView.h"
#include "Common.h"
#include "PulseApp.h"
#include "PrefsWindow.h"

#include <Catalog.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <TextControl.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


RTColorControl::RTColorControl(BPoint point, BMessage *message)
	: BColorControl(point, B_CELLS_32x8, 6, "ColorControl", message, false)
{
}


/*!
	Send a message every time the color changes, not just
	when the mouse button is released
*/
void
RTColorControl::SetValue(int32 color)
{
	BColorControl::SetValue(color);
	Invoke();
}


//	#pragma mark -


/*!
	A single class for all three prefs views, needs to be
	customized below to give each control the right message
*/
ConfigView::ConfigView(BRect rect, const char *name, uint32 mode, BMessenger& target,
		Prefs *prefs)
	: BBox(rect, name, B_FOLLOW_NONE, B_WILL_DRAW),
	fMode(mode),
	fTarget(target),
	fPrefs(prefs),
	fFirstTimeAttached(true)
{
	fFadeCheckBox = NULL;
	fActiveButton = fIdleButton = fFrameButton = NULL;
	fIconWidthControl = NULL;

	SetLabel(B_TRANSLATE("Bar colors"));

	font_height fontHeight;
	be_bold_font->GetHeight(&fontHeight);

	fColorControl = new RTColorControl(BPoint(10, 5.0f + fontHeight.ascent
		+ fontHeight.descent), new BMessage(fMode));
	fColorControl->ResizeToPreferred();
	AddChild(fColorControl);

	rect = fColorControl->Frame();
	rect.top = rect.bottom + 10.0f;
	rect.bottom = rect.top + 15.0f;

	if (mode == PRV_NORMAL_CHANGE_COLOR) {
		// normal mode

		fFadeCheckBox = new BCheckBox(rect, "FadeColors", 
			B_TRANSLATE("Fade colors"), new BMessage(PRV_NORMAL_FADE_COLORS));
		fFadeCheckBox->ResizeToPreferred();
		AddChild(fFadeCheckBox);

		fColorControl->SetValue(fPrefs->normal_bar_color);
		fFadeCheckBox->SetValue(fPrefs->normal_fade_colors);
	} else if (mode == PRV_MINI_CHANGE_COLOR) {
		// mini mode

		fActiveButton = new BRadioButton(rect, "ActiveColor", 
			B_TRANSLATE("Active color"), new BMessage(PRV_MINI_ACTIVE));
		fActiveButton->ResizeToPreferred();
		fActiveButton->SetValue(B_CONTROL_ON);
		AddChild(fActiveButton);

		rect.left = fActiveButton->Frame().right + 5.0f;
		fIdleButton = new BRadioButton(rect, "IdleColor", 
			B_TRANSLATE("Idle color"), new BMessage(PRV_MINI_IDLE));
		fIdleButton->ResizeToPreferred();
		AddChild(fIdleButton);

		rect.left = fIdleButton->Frame().right + 5.0f;
		fFrameButton = new BRadioButton(rect, "FrameColor", 
			B_TRANSLATE("Frame color"),	new BMessage(PRV_MINI_FRAME));
		fFrameButton->ResizeToPreferred();
		AddChild(fFrameButton);

		fColorControl->SetValue(fPrefs->mini_active_color);
	} else {
		// deskbar mode
		fActiveButton = new BRadioButton(rect, "ActiveColor", 
			B_TRANSLATE("Active color"), new BMessage(PRV_DESKBAR_ACTIVE));
		fActiveButton->ResizeToPreferred();
		fActiveButton->SetValue(B_CONTROL_ON);
		AddChild(fActiveButton);

		rect.left = fActiveButton->Frame().right + 5.0f;
		fIdleButton = new BRadioButton(rect, "IdleColor", 
			B_TRANSLATE("Idle color"), new BMessage(PRV_DESKBAR_IDLE));
		fIdleButton->ResizeToPreferred();
		AddChild(fIdleButton);

		rect.left = fIdleButton->Frame().right + 5.0f;
		fFrameButton = new BRadioButton(rect, "FrameColor", 
			B_TRANSLATE("Frame color"),	new BMessage(PRV_DESKBAR_FRAME));
		fFrameButton->ResizeToPreferred();
		AddChild(fFrameButton);

		rect.left = fColorControl->Frame().left;
		rect.top = fActiveButton->Frame().bottom + 5.0f;

		char temp[10];
		snprintf(temp, sizeof(temp), "%d", fPrefs->deskbar_icon_width);
		fIconWidthControl = new BTextControl(rect, "Width", 
			B_TRANSLATE("Width of icon:"), temp, 
			new BMessage(PRV_DESKBAR_ICON_WIDTH));
		AddChild(fIconWidthControl);
		fIconWidthControl->SetDivider(be_plain_font->StringWidth(
			fIconWidthControl->Label()) + 5.0f);

		for (int c = 0; c < 256; c++) {
			if (!isdigit(c))
				fIconWidthControl->TextView()->DisallowChar(c);
		}
		fIconWidthControl->TextView()->SetMaxBytes(2);
		
		float width, height;
		fIconWidthControl->GetPreferredSize(&width, &height);
		fIconWidthControl->ResizeTo(fIconWidthControl->Divider() + 32.0f
			+ fIconWidthControl->StringWidth("999"), height);

		fColorControl->SetValue(fPrefs->deskbar_active_color);
	}
}


void
ConfigView::GetPreferredSize(float* _width, float* _height)
{
	float right, bottom;

	if (fMode == PRV_NORMAL_CHANGE_COLOR) {
		// normal mode
		bottom = fFadeCheckBox->Frame().bottom;
		right = fFadeCheckBox->Frame().right;
	} else if (fMode == PRV_MINI_CHANGE_COLOR) {
		// mini mode
		bottom = fIdleButton->Frame().bottom;
		right = fFrameButton->Frame().right;
	} else {
		// deskbar mode
		bottom = fIconWidthControl->Frame().bottom;
		right = fFrameButton->Frame().right;
	}

	if (right < fColorControl->Frame().right)
		right = fColorControl->Frame().right;
	if (right < 300)
		right = 300;

	if (_width)
		*_width = right + 10.0f;
	if (_height)
		*_height = bottom + 8.0f;
}


void
ConfigView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	// AttachedToWindow() gets called every time this tab is brought
	// to the front, but we only want this initialization to happen once
	if (fFirstTimeAttached) {
		if (Parent() != NULL)
			SetViewColor(Parent()->ViewColor());
		else
			SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		BMessenger messenger(this);
		fColorControl->SetTarget(messenger);
		if (fFadeCheckBox != NULL)
			fFadeCheckBox->SetTarget(messenger);
		if (fActiveButton != NULL)
			fActiveButton->SetTarget(messenger);
		if (fIdleButton != NULL)
			fIdleButton->SetTarget(messenger);
		if (fFrameButton != NULL)	
			fFrameButton->SetTarget(messenger);
		if (fIconWidthControl != NULL)
			fIconWidthControl->SetTarget(messenger);

		fFirstTimeAttached = false;
	}
}


void
ConfigView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		// These two send the color and the status of the fade checkbox together
		case PRV_NORMAL_FADE_COLORS:
		case PRV_NORMAL_CHANGE_COLOR:
		{
			bool fade_colors = (bool)fFadeCheckBox->Value();
			int32 bar_color = fColorControl->Value();
			message->AddInt32("color", bar_color);
			message->AddBool("fade", fade_colors);
			fPrefs->normal_fade_colors = fade_colors;
			fPrefs->normal_bar_color = bar_color;

			fTarget.SendMessage(message);
			break;
		}
		// Share the single color control among three values
		case PRV_MINI_ACTIVE:
			fColorControl->SetValue(fPrefs->mini_active_color);
			break;
		case PRV_MINI_IDLE:
			fColorControl->SetValue(fPrefs->mini_idle_color);
			break;
		case PRV_MINI_FRAME:
			fColorControl->SetValue(fPrefs->mini_frame_color);
			break;
		case PRV_MINI_CHANGE_COLOR: {
			int32 color = fColorControl->Value();
			if (fActiveButton->Value())
				fPrefs->mini_active_color = color;
			else if (fIdleButton->Value())
				fPrefs->mini_idle_color = color;
			else
				fPrefs->mini_frame_color = color;

			message->AddInt32("active_color", fPrefs->mini_active_color);
			message->AddInt32("idle_color", fPrefs->mini_idle_color);
			message->AddInt32("frame_color", fPrefs->mini_frame_color);
			fTarget.SendMessage(message);
			break;
		}
		case PRV_DESKBAR_ACTIVE:
			fColorControl->SetValue(fPrefs->deskbar_active_color);
			break;
		case PRV_DESKBAR_IDLE:
			fColorControl->SetValue(fPrefs->deskbar_idle_color);
			break;
		case PRV_DESKBAR_FRAME:
			fColorControl->SetValue(fPrefs->deskbar_frame_color);
			break;
		case PRV_DESKBAR_ICON_WIDTH:
			UpdateDeskbarIconWidth();
			break;
		case PRV_DESKBAR_CHANGE_COLOR: {
			int32 color = fColorControl->Value();
			if (fActiveButton->Value())
				fPrefs->deskbar_active_color = color;
			else if (fIdleButton->Value())
				fPrefs->deskbar_idle_color = color;
			else
				fPrefs->deskbar_frame_color = color;

			message->AddInt32("active_color", fPrefs->deskbar_active_color);
			message->AddInt32("idle_color", fPrefs->deskbar_idle_color);
			message->AddInt32("frame_color", fPrefs->deskbar_frame_color);
			fTarget.SendMessage(message);
			break;
		}
		case PRV_BOTTOM_DEFAULTS:
			_ResetDefaults();
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
ConfigView::UpdateDeskbarIconWidth()
{
	// Make sure the width shows at least one pixel per CPU and
	// that it will fit in the tray in any Deskbar orientation
	int width = atoi(fIconWidthControl->Text());
	int min_width = GetMinimumViewWidth();
	if (width < min_width || width > 50) {
		char temp[10];
		if (width < min_width) {
			sprintf(temp, "%d", min_width);
			width = min_width;
		} else {
			strcpy(temp, "50");
			width = 50;
		}
		fIconWidthControl->SetText(temp);
	}

	fPrefs->deskbar_icon_width = width;

	BMessage message(PRV_DESKBAR_ICON_WIDTH);
	message.AddInt32("width", width);
	fTarget.SendMessage(&message);
}


void
ConfigView::_ResetDefaults()
{
	if (fMode == PRV_NORMAL_CHANGE_COLOR) {
		fColorControl->SetValue(DEFAULT_NORMAL_BAR_COLOR);
		fFadeCheckBox->SetValue(DEFAULT_NORMAL_FADE_COLORS);
	} else if (fMode == PRV_MINI_CHANGE_COLOR) {
		fPrefs->mini_active_color = DEFAULT_MINI_ACTIVE_COLOR;
		fPrefs->mini_idle_color = DEFAULT_MINI_IDLE_COLOR;
		fPrefs->mini_frame_color = DEFAULT_MINI_FRAME_COLOR;
		if (fActiveButton->Value())
			fColorControl->SetValue(DEFAULT_MINI_ACTIVE_COLOR);
		else if (fIdleButton->Value())
			fColorControl->SetValue(DEFAULT_MINI_IDLE_COLOR);
		else
			fColorControl->SetValue(DEFAULT_MINI_FRAME_COLOR);

		BMessage *message = new BMessage(PRV_MINI_CHANGE_COLOR);
		message->AddInt32("active_color", DEFAULT_MINI_ACTIVE_COLOR);
		message->AddInt32("idle_color", DEFAULT_MINI_IDLE_COLOR);
		message->AddInt32("frame_color", DEFAULT_MINI_FRAME_COLOR);
		fTarget.SendMessage(message);
	} else {	
		fPrefs->deskbar_active_color = DEFAULT_DESKBAR_ACTIVE_COLOR;
		fPrefs->deskbar_idle_color = DEFAULT_DESKBAR_IDLE_COLOR;
		fPrefs->deskbar_frame_color = DEFAULT_DESKBAR_FRAME_COLOR;
		if (fActiveButton->Value())
			fColorControl->SetValue(DEFAULT_DESKBAR_ACTIVE_COLOR);
		else if (fIdleButton->Value())
			fColorControl->SetValue(DEFAULT_DESKBAR_IDLE_COLOR);
		else
			fColorControl->SetValue(DEFAULT_DESKBAR_FRAME_COLOR);

		BMessage *message = new BMessage(PRV_DESKBAR_CHANGE_COLOR);
		message->AddInt32("active_color", DEFAULT_DESKBAR_ACTIVE_COLOR);
		message->AddInt32("idle_color", DEFAULT_DESKBAR_IDLE_COLOR);
		message->AddInt32("frame_color", DEFAULT_DESKBAR_FRAME_COLOR);
		fTarget.SendMessage(message);

		char temp[10];
		sprintf(temp, "%d", DEFAULT_DESKBAR_ICON_WIDTH);
		fIconWidthControl->SetText(temp);
		// Need to force the model message to be sent
		fIconWidthControl->Invoke();
	}
}
