//****************************************************************************************
//
//	File:		CPUButton.cpp
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "CPUButton.h"
#include "PulseApp.h"
#include "PulseView.h"
#include "Common.h"
#include <interface/Alert.h>
#include <stdlib.h>

CPUButton::CPUButton(BRect rect, const char *name, const char *label, BMessage *message) :
	BControl(rect, name, label, message, B_FOLLOW_NONE, B_WILL_DRAW) {

	off_color.red = off_color.green = off_color.blue = 184;
	off_color.alpha = 255;
	SetValue(B_CONTROL_ON);
	SetViewColor(B_TRANSPARENT_COLOR);
	replicant = false;
}

CPUButton::CPUButton(BMessage *message) : BControl(message) {
	off_color.red = off_color.green = off_color.blue = 184;
	off_color.alpha = 255;
	replicant = true;
}

// Redraw the button depending on whether it's up or down
void CPUButton::Draw(BRect rect) {
	bool value = (bool)Value();
	if (value) SetHighColor(on_color);
	else SetHighColor(off_color);

	BRect bounds = Bounds();
	BRect color_rect(bounds);
	color_rect.InsetBy(2, 2);
	if (value) {
		color_rect.bottom -= 1;
		color_rect.right -= 1;
	}
	FillRect(bounds);
	
	if (value) SetHighColor(80, 80, 80);
	else SetHighColor(255, 255, 255);
	BPoint start(0, 0);
	BPoint end(bounds.right, 0);
	StrokeLine(start, end);
	end.Set(0, bounds.bottom);
	StrokeLine(start, end);
	
	if (value) SetHighColor(32, 32, 32);
	else SetHighColor(216, 216, 216);
	start.Set(1, 1);
	end.Set(bounds.right - 1, 1);
	StrokeLine(start, end);
	end.Set(1, bounds.bottom - 1);
	StrokeLine(start, end);
	
	if (value) SetHighColor(216, 216, 216);
	else SetHighColor(80, 80, 80);
	start.Set(bounds.left + 1, bounds.bottom - 1);
	end.Set(bounds.right - 1, bounds.bottom - 1);
	StrokeLine(start, end);
	start.Set(bounds.right - 1, bounds.top + 1);
	StrokeLine(start, end);

	if (value) SetHighColor(255, 255, 255);
	else SetHighColor(32, 32, 32);
	start.Set(bounds.left, bounds.bottom);
	end.Set(bounds.right, bounds.bottom);
	StrokeLine(start, end);
	start.Set(bounds.right, bounds.top);
	StrokeLine(start, end);
	
	if (value) {
		SetHighColor(0, 0, 0);
		start.Set(bounds.left + 2, bounds.bottom - 2);
		end.Set(bounds.right - 2, bounds.bottom - 2);
		StrokeLine(start, end);
		start.Set(bounds.right - 2, bounds.top + 2);
		StrokeLine(start, end);
	}
	
	// Try to keep the text centered
	BFont font;
	GetFont(&font);
	int label_width = (int)font.StringWidth(Label());
	int rect_width = bounds.IntegerWidth() - 1;
	int rect_height = bounds.IntegerHeight();
	font_height fh;
	font.GetHeight(&fh);
	int label_height = (int)fh.ascent;
	int x_pos = (int)(((double)(rect_width - label_width) / 2.0) + 0.5);
	int y_pos = (rect_height - label_height) / 2 + label_height;
	
	MovePenTo(x_pos, y_pos);
	SetHighColor(0, 0, 0);
	SetDrawingMode(B_OP_OVER);
	DrawString(Label());
}

// Track the mouse without blocking the window
void CPUButton::MouseDown(BPoint point) {
	SetValue(!Value());
	SetTracking(true);
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}

void CPUButton::MouseUp(BPoint point) {
	if (Bounds().Contains(point)) Invoke();
	SetTracking(false);
}

void CPUButton::MouseMoved(BPoint point, uint32 transit, const BMessage *message) {
	if (IsTracking()) {
		if (transit == B_ENTERED_VIEW || transit == B_EXITED_VIEW) SetValue(!Value());
	}
}

status_t CPUButton::Invoke(BMessage *message) {
	int my_cpu = atoi(Label()) - 1;
	
	if (!LastEnabledCPU(my_cpu)) {
		_kset_cpu_state_(my_cpu, Value());
	} else {
		BAlert *alert = new BAlert(NULL, "You can't disable the last active CPU.", "OK");
		alert->Go(NULL);
		SetValue(!Value());
	}
	
	return B_OK;
}

CPUButton *CPUButton::Instantiate(BMessage *data) {
	if (!validate_instantiation(data, "CPUButton")) return NULL;
	return new CPUButton(data);
}

status_t CPUButton::Archive(BMessage *data, bool deep) const {
	BControl::Archive(data, deep);
	data->AddString("add_on", APP_SIGNATURE);
	data->AddString("class", "CPUButton");
	return B_OK;
}

void CPUButton::MessageReceived(BMessage *message) {
	switch(message->what) {
		case B_ABOUT_REQUESTED: {
			BAlert *alert = new BAlert("Info", "Pulse\n\nBy David Ramsey and Arve Hjønnevåg\nRevised by Daniel Switkin", "OK");
			// Use the asynchronous version so we don't block the window's thread
			alert->Go(NULL);
			break;
		}
		case PV_REPLICANT_PULSE: {
			// Make sure we're consistent with our CPU
			int my_cpu = atoi(Label()) - 1;
			if (_kget_cpu_state_(my_cpu) != Value() && !IsTracking()) SetValue(!Value());
			break;
		}
		default:
			BControl::MessageReceived(message);
			break;
	}
}

void CPUButton::UpdateColors(int32 color) {
	on_color.red = (color & 0xff000000) >> 24;
	on_color.green = (color & 0x00ff0000) >> 16;
	on_color.blue = (color & 0x0000ff00) >> 8;
	Draw(Bounds());
}

void CPUButton::AttachedToWindow() {
	SetTarget(this);
	SetFont(be_plain_font);
	SetFontSize(10);
	
	if (replicant) {
		Prefs *prefs = new Prefs();
		UpdateColors(prefs->normal_bar_color);
		delete prefs;
	} else {
		PulseApp *pulseapp = (PulseApp *)be_app;
		UpdateColors(pulseapp->prefs->normal_bar_color);
	}
	
	BMessenger messenger(this);
	messagerunner = new BMessageRunner(messenger, new BMessage(PV_REPLICANT_PULSE),
		200000, -1);
}

CPUButton::~CPUButton() {
	delete messagerunner;
}