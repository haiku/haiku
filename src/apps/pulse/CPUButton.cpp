//*****************************************************************************
//
//	File:		CPUButton.cpp
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//*****************************************************************************

#include "CPUButton.h"

#include <stdlib.h>
#include <string.h>

#include <Alert.h>
#include <Catalog.h>
#include <Dragger.h>
#include <PopUpMenu.h>
#include <TextView.h>
#include <ViewPrivate.h>

#include <syscalls.h>

#include "PulseApp.h"
#include "PulseView.h"
#include "Common.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CPUButton"


CPUButton::CPUButton(BRect rect, const char *name, const char *label, BMessage *message)
	: BControl(rect, name, label, message, B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetValue(B_CONTROL_ON);
	SetViewColor(B_TRANSPARENT_COLOR);
	fReplicant = false;

	_InitData();
}


CPUButton::CPUButton(BMessage *message)
	: BControl(message)
{
	fReplicant = true;

	/* We remove the dragger if we are in deskbar */
	if (CountChildren() > 1)
		RemoveChild(ChildAt(1));

	ResizeTo(CPUBUTTON_WIDTH, CPUBUTTON_HEIGHT);

	_InitData();
}


CPUButton::~CPUButton()
{
}


void
CPUButton::_InitData()
{
	fOffColor.red = fOffColor.green = fOffColor.blue = 184;
	fOffColor.alpha = 255;

	fCPU = atoi(Label()) - 1;
}


void
CPUButton::_AddDragger()
{
	BRect rect(Bounds());
	rect.top = rect.bottom - 7;
	rect.left = rect.right - 7;
	BDragger* dragger = new BDragger(rect, this,
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(dragger);
}


//! Redraw the button depending on whether it's up or down
void
CPUButton::Draw(BRect rect)
{
	bool value = (bool)Value();
	SetHighColor(value ? fOnColor : fOffColor);

	if (!fReplicant) {
		SetLowColor(Parent()->LowColor());
		FillRect(Bounds(), B_SOLID_LOW);
	}

	BRect bounds = Bounds();
	if (fReplicant && !fReplicantInDeskbar) {
		bounds.bottom -= 4;
		bounds.right -= 4;
	} else if (!fReplicant) {
		bounds.bottom -= 7;
		bounds.right -= 7;
	}
	BRect color_rect(bounds);
	color_rect.InsetBy(2, 2);
	if (value) {
		color_rect.bottom -= 1;
		color_rect.right -= 1;
	}
	FillRect(bounds);

	if (value)
		SetHighColor(80, 80, 80);
	else
		SetHighColor(255, 255, 255);

	BPoint start(0, 0);
	BPoint end(bounds.right, 0);
	StrokeLine(start, end);
	end.Set(0, bounds.bottom);
	StrokeLine(start, end);

	if (value)
		SetHighColor(32, 32, 32);
	else
		SetHighColor(216, 216, 216);

	start.Set(1, 1);
	end.Set(bounds.right - 1, 1);
	StrokeLine(start, end);
	end.Set(1, bounds.bottom - 1);
	StrokeLine(start, end);

	if (value)
		SetHighColor(216, 216, 216);
	else
		SetHighColor(80, 80, 80);

	start.Set(bounds.left + 1, bounds.bottom - 1);
	end.Set(bounds.right - 1, bounds.bottom - 1);
	StrokeLine(start, end);
	start.Set(bounds.right - 1, bounds.top + 1);
	StrokeLine(start, end);

	if (value)
		SetHighColor(255, 255, 255);
	else
		SetHighColor(32, 32, 32);

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


//! Track the mouse without blocking the window
void
CPUButton::MouseDown(BPoint point)
{
	BPoint mousePosition;
	uint32 mouseButtons;

	GetMouse(&mousePosition, &mouseButtons);

	if ((B_PRIMARY_MOUSE_BUTTON & mouseButtons) != 0) {
		SetValue(!Value());
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	} else if ((B_SECONDARY_MOUSE_BUTTON & mouseButtons) != 0
		&& fReplicantInDeskbar) {
		BPopUpMenu *menu = new BPopUpMenu(B_TRANSLATE("Deskbar menu"));
		menu->AddItem(new BMenuItem(B_TRANSLATE("About Pulse" B_UTF8_ELLIPSIS),
			new BMessage(B_ABOUT_REQUESTED)));
		menu->AddSeparatorItem();
		menu->AddItem(new BMenuItem(B_TRANSLATE("Remove replicant"),
			new BMessage(kDeleteReplicant)));
		menu->SetTargetForItems(this);

		ConvertToScreen(&point);
		menu->Go(point, true, true, true);
	}
}


void
CPUButton::MouseUp(BPoint point)
{
	if (IsTracking()) {
		if (Bounds().Contains(point))
			Invoke();

		SetTracking(false);
	}
}


void
CPUButton::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (IsTracking()) {
		if (transit == B_ENTERED_VIEW || transit == B_EXITED_VIEW)
			SetValue(!Value());
	}
}


status_t
CPUButton::Invoke(BMessage *message)
{
	if (!LastEnabledCPU(fCPU)) {
		_kern_set_cpu_enabled(fCPU, Value());
	} else {
		BAlert *alert = new BAlert(B_TRANSLATE("Info"),
			B_TRANSLATE("You can't disable the last active CPU."),
			B_TRANSLATE("OK"));
		alert->SetShortcut(0, B_ESCAPE);
		alert->Go(NULL);
		SetValue(!Value());
	}

	return B_OK;
}


CPUButton *
CPUButton::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "CPUButton"))
		return NULL;

	return new CPUButton(data);
}


status_t
CPUButton::Archive(BMessage *data, bool deep) const
{
	BControl::Archive(data, deep);
	data->AddString("add_on", APP_SIGNATURE);
	data->AddString("class", "CPUButton");
	return B_OK;
}


void
CPUButton::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_ABOUT_REQUESTED: {
			PulseApp::ShowAbout(false);
			break;
		}
		case PV_REPLICANT_PULSE: {
			// Make sure we're consistent with our CPU
			if (_kern_cpu_enabled(fCPU) != Value() && !IsTracking())
				SetValue(!Value());
			break;
		}
		case kDeleteReplicant: {
			Window()->PostMessage(kDeleteReplicant, this, NULL);
			break;
		}
		default:
			BControl::MessageReceived(message);
			break;
	}
}


void
CPUButton::UpdateColors(int32 color)
{
	fOnColor.red = (color & 0xff000000) >> 24;
	fOnColor.green = (color & 0x00ff0000) >> 16;
	fOnColor.blue = (color & 0x0000ff00) >> 8;
	Draw(Bounds());
}


void
CPUButton::AttachedToWindow()
{
	SetTarget(this);
	SetFont(be_plain_font);
	SetFontSize(10);
	
	fReplicantInDeskbar = false;

	if (fReplicant) {
		if (strcmp(Window()->Title(), B_TRANSLATE("Deskbar"))) {
			// Make room for dragger
			ResizeBy(4, 4);

			_AddDragger();
		} else
			fReplicantInDeskbar = true;

		Prefs *prefs = new Prefs();
		UpdateColors(prefs->normal_bar_color);
		delete prefs;
	} else {
		PulseApp *pulseapp = (PulseApp *)be_app;
		UpdateColors(pulseapp->prefs->normal_bar_color);
		_AddDragger();
	}

	BMessenger messenger(this);
	fPulseRunner = new BMessageRunner(messenger, new BMessage(PV_REPLICANT_PULSE),
		200000, -1);
}


void
CPUButton::DetachedFromWindow()
{
	delete fPulseRunner;
}
