//****************************************************************************************
//
//	File:		ConfigView.cpp
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "ConfigView.h"
#include "Common.h"
#include "PulseApp.h"
#include "PrefsWindow.h"
#include <interface/Box.h>
#include <stdlib.h>
#include <stdio.h>

RTColorControl::RTColorControl(BPoint point, BMessage *message)
	: BColorControl(point, B_CELLS_32x8, 6, "ColorControl", message, false) {

}

// Send a message every time the color changes, not just
// when the mouse button is released
void RTColorControl::SetValue(int32 color) {
	BColorControl::SetValue(color);
	Invoke();
}

// A single class for all three prefs views, needs to be
// customized below to give each control the right message
ConfigView::ConfigView(BRect rect, const char *name, int mode, Prefs *prefs) :
	BView(rect, name, B_FOLLOW_NONE, B_WILL_DRAW) {

	this->mode = mode;
	first_time_attached = true;
	fadecolors = NULL;
	active = idle = frame = NULL;
	iconwidth = NULL;

	BRect r(6, 5, 296, 115);
	BBox *bbox = new BBox(r, "BBox");
	bbox->SetLabel("Bar Colors");
	AddChild(bbox);
	
	if (mode == NORMAL_WINDOW_MODE) {
		colorcontrol = new RTColorControl(BPoint(10, 20),
			new BMessage(PRV_NORMAL_CHANGE_COLOR));
	} else if (mode == MINI_WINDOW_MODE) {
		colorcontrol = new RTColorControl(BPoint(10, 20),
			new BMessage(PRV_MINI_CHANGE_COLOR));
	} else {
		colorcontrol = new RTColorControl(BPoint(10, 20),
			new BMessage(PRV_DESKBAR_CHANGE_COLOR));
	}

	bbox->AddChild(colorcontrol);
	r = colorcontrol->Frame();
	r.top = r.bottom + 10;
	r.bottom = r.top + 15;

	if (mode == NORMAL_WINDOW_MODE) {
		r.right = r.left + be_plain_font->StringWidth("Fade colors") + 20;
		fadecolors = new BCheckBox(r, "FadeColors", "Fade colors",
			new BMessage(PRV_NORMAL_FADE_COLORS));
		bbox->AddChild(fadecolors);
		
		colorcontrol->SetValue(prefs->normal_bar_color);
		fadecolors->SetValue(prefs->normal_fade_colors);
	} else if (mode == MINI_WINDOW_MODE) {
		r.right = r.left + be_plain_font->StringWidth("Active color") + 20;
		active = new BRadioButton(r, "ActiveColor", "Active color",
			new BMessage(PRV_MINI_ACTIVE));
		bbox->AddChild(active);
		active->SetValue(B_CONTROL_ON);
		
		r.left = r.right + 5;
		r.right = r.left + be_plain_font->StringWidth("Idle color") + 20;
		idle = new BRadioButton(r, "IdleColor", "Idle color",
			new BMessage(PRV_MINI_IDLE));
		bbox->AddChild(idle);
		
		r.left = r.right + 5;
		r.right = r.left + be_plain_font->StringWidth("Frame color") + 20;
		frame = new BRadioButton(r, "FrameColor", "Frame color",
			new BMessage(PRV_MINI_FRAME));
		bbox->AddChild(frame);
		
		colorcontrol->SetValue(prefs->mini_active_color);
	} else {
		bbox->ResizeBy(0, 20);
	
		r.right = r.left + be_plain_font->StringWidth("Active color") + 20;
		active = new BRadioButton(r, "ActiveColor", "Active color",
			new BMessage(PRV_DESKBAR_ACTIVE));
		bbox->AddChild(active);
		active->SetValue(B_CONTROL_ON);
		
		r.left = r.right + 5;
		r.right = r.left + be_plain_font->StringWidth("Idle color") + 20;
		idle = new BRadioButton(r, "IdleColor", "Idle color",
			new BMessage(PRV_DESKBAR_IDLE));
		bbox->AddChild(idle);
		
		r.left = r.right + 5;
		r.right = r.left + be_plain_font->StringWidth("Frame color") + 20;
		frame = new BRadioButton(r, "FrameColor", "Frame color",
			new BMessage(PRV_DESKBAR_FRAME));
		bbox->AddChild(frame);
		
		r.top = active->Frame().bottom + 1;
		r.bottom = r.top + 15;
		r.left = 10;
		r.right = r.left + be_plain_font->StringWidth("Width of icon:") + 5 + 30;
		char temp[10];
		sprintf(temp, "%d", prefs->deskbar_icon_width);
		iconwidth = new BTextControl(r, "Width", "Width of icon:", temp,
			new BMessage(PRV_DESKBAR_ICON_WIDTH));
		bbox->AddChild(iconwidth);
		iconwidth->SetDivider(be_plain_font->StringWidth("Width of icon:") + 5);
		//iconwidth->SetModificationMessage(new BMessage(PRV_DESKBAR_ICON_WIDTH));
		
		for (int x = 0; x < 256; x++) {
			if (x < '0' || x > '9') iconwidth->TextView()->DisallowChar(x);
		}
		iconwidth->TextView()->SetMaxBytes(2);
		
		colorcontrol->SetValue(prefs->deskbar_active_color);
	}
}

void ConfigView::AttachedToWindow() {
	BView::AttachedToWindow();
	
	// AttachedToWindow() gets called every time this tab is brought
	// to the front, but we only want this initialization to happen once
	if (first_time_attached) {
		BMessenger messenger(this);
		colorcontrol->SetTarget(messenger);
		if (fadecolors != NULL) fadecolors->SetTarget(messenger);
		if (active != NULL) active->SetTarget(messenger);
		if (idle != NULL) idle->SetTarget(messenger);
		if (frame != NULL) frame->SetTarget(messenger);
		if (iconwidth != NULL) iconwidth->SetTarget(messenger);
		
		first_time_attached = false;
	}
}

void ConfigView::MessageReceived(BMessage *message) {
	PrefsWindow *prefswindow = (PrefsWindow *)Window();
	if (prefswindow == NULL) return;
	Prefs *prefs = prefswindow->prefs;
	BMessenger *messenger = prefswindow->messenger;
	
	switch (message->what) {
		// These two send the color and the status of the fade checkbox together
		case PRV_NORMAL_FADE_COLORS:
		case PRV_NORMAL_CHANGE_COLOR: {
			bool fade_colors = (bool)fadecolors->Value();
			int32 bar_color = colorcontrol->Value();
			message->AddInt32("color", bar_color);
			message->AddBool("fade", fade_colors);
			prefs->normal_fade_colors = fade_colors;
			prefs->normal_bar_color = bar_color;
			messenger->SendMessage(message);
			break;
		}
		// Share the single color control among three values
		case PRV_MINI_ACTIVE:
			colorcontrol->SetValue(prefs->mini_active_color);
			break;
		case PRV_MINI_IDLE:
			colorcontrol->SetValue(prefs->mini_idle_color);
			break;
		case PRV_MINI_FRAME:
			colorcontrol->SetValue(prefs->mini_frame_color);
			break;
		case PRV_MINI_CHANGE_COLOR: {
			int32 color = colorcontrol->Value();
			if (active->Value()) {
				prefs->mini_active_color = color;
			} else if (idle->Value()) {
				prefs->mini_idle_color = color;
			} else {
				prefs->mini_frame_color = color;
			}
			message->AddInt32("active_color", prefs->mini_active_color);
			message->AddInt32("idle_color", prefs->mini_idle_color);
			message->AddInt32("frame_color", prefs->mini_frame_color);
			messenger->SendMessage(message);
			break;
		}
		case PRV_DESKBAR_ACTIVE:
			colorcontrol->SetValue(prefs->deskbar_active_color);
			break;
		case PRV_DESKBAR_IDLE:
			colorcontrol->SetValue(prefs->deskbar_idle_color);
			break;
		case PRV_DESKBAR_FRAME:
			colorcontrol->SetValue(prefs->deskbar_frame_color);
			break;
		case PRV_DESKBAR_ICON_WIDTH:
			UpdateDeskbarIconWidth();
			break;
		case PRV_DESKBAR_CHANGE_COLOR: {
			int32 color = colorcontrol->Value();
			if (active->Value()) {
				prefs->deskbar_active_color = color;
			} else if (idle->Value()) {
				prefs->deskbar_idle_color = color;
			} else {
				prefs->deskbar_frame_color = color;
			}
			message->AddInt32("active_color", prefs->deskbar_active_color);
			message->AddInt32("idle_color", prefs->deskbar_idle_color);
			message->AddInt32("frame_color", prefs->deskbar_frame_color);
			messenger->SendMessage(message);
			break;
		}
		case PRV_BOTTOM_DEFAULTS:
			ResetDefaults();
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}

void ConfigView::UpdateDeskbarIconWidth() {
	PrefsWindow *prefswindow = (PrefsWindow *)Window();
	if (prefswindow == NULL) return;
	Prefs *prefs = prefswindow->prefs;
	BMessenger *messenger = prefswindow->messenger;
	
	// Make sure the width shows at least one pixel per CPU and
	// that it will fit in the tray in any Deskbar orientation
	int width = atoi(iconwidth->Text());
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
		iconwidth->SetText(temp);
	}
	
	BMessage *message = new BMessage(PRV_DESKBAR_ICON_WIDTH);
	message->AddInt32("width", width);
	prefs->deskbar_icon_width = width;
	messenger->SendMessage(message);
	delete message;
}

// Only reset our own controls to default
void ConfigView::ResetDefaults() {
	PrefsWindow *prefswindow = (PrefsWindow *)Window();
	if (prefswindow == NULL) return;
	Prefs *prefs = prefswindow->prefs;
	BMessenger *messenger = prefswindow->messenger;
	
	if (mode == NORMAL_WINDOW_MODE) {
		colorcontrol->SetValue(DEFAULT_NORMAL_BAR_COLOR);
		fadecolors->SetValue(DEFAULT_NORMAL_FADE_COLORS);
	} else if (mode == MINI_WINDOW_MODE) {
		prefs->mini_active_color = DEFAULT_MINI_ACTIVE_COLOR;
		prefs->mini_idle_color = DEFAULT_MINI_IDLE_COLOR;
		prefs->mini_frame_color = DEFAULT_MINI_FRAME_COLOR;
		if (active->Value()) {
			colorcontrol->SetValue(DEFAULT_MINI_ACTIVE_COLOR);
		} else if (idle->Value()) {
			colorcontrol->SetValue(DEFAULT_MINI_IDLE_COLOR);
		} else {
			colorcontrol->SetValue(DEFAULT_MINI_FRAME_COLOR);
		}
		BMessage *message = new BMessage(PRV_MINI_CHANGE_COLOR);
		message->AddInt32("active_color", DEFAULT_MINI_ACTIVE_COLOR);
		message->AddInt32("idle_color", DEFAULT_MINI_IDLE_COLOR);
		message->AddInt32("frame_color", DEFAULT_MINI_FRAME_COLOR);
		messenger->SendMessage(message);
	} else {	
		prefs->deskbar_active_color = DEFAULT_DESKBAR_ACTIVE_COLOR;
		prefs->deskbar_idle_color = DEFAULT_DESKBAR_IDLE_COLOR;
		prefs->deskbar_frame_color = DEFAULT_DESKBAR_FRAME_COLOR;
		if (active->Value()) {
			colorcontrol->SetValue(DEFAULT_DESKBAR_ACTIVE_COLOR);
		} else if (idle->Value()) {
			colorcontrol->SetValue(DEFAULT_DESKBAR_IDLE_COLOR);
		} else {
			colorcontrol->SetValue(DEFAULT_DESKBAR_FRAME_COLOR);
		}
		BMessage *message = new BMessage(PRV_DESKBAR_CHANGE_COLOR);
		message->AddInt32("active_color", DEFAULT_DESKBAR_ACTIVE_COLOR);
		message->AddInt32("idle_color", DEFAULT_DESKBAR_IDLE_COLOR);
		message->AddInt32("frame_color", DEFAULT_DESKBAR_FRAME_COLOR);
		messenger->SendMessage(message);
	
		char temp[10];
		sprintf(temp, "%d", DEFAULT_DESKBAR_ICON_WIDTH);
		iconwidth->SetText(temp);
		// Need to force the model message to be sent
		iconwidth->Invoke();
	}
}