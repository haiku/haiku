//*****************************************************************************
//
//	File:		MiniPulseView.cpp
//
//	Written by:	Arve Hjonnevag and Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//*****************************************************************************

#include "MiniPulseView.h"
#include "Common.h"
#include <Catalog.h>
#include <interface/Window.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MiniPulseView"


MiniPulseView::MiniPulseView(BRect rect, const char *name, Prefs *prefs) : 
	PulseView(rect, name) {

	mode1->SetLabel(B_TRANSLATE("Normal mode"));
	mode1->SetMessage(new BMessage(PV_NORMAL_MODE));
	mode2->SetLabel(B_TRANSLATE("Deskbar mode"));
	mode2->SetMessage(new BMessage(PV_DESKBAR_MODE));
	quit = new BMenuItem(B_TRANSLATE("Quit"), new BMessage(PV_QUIT), 0, 0);
	popupmenu->AddSeparatorItem();
	popupmenu->AddItem(quit);
	
	// Our drawing covers every pixel in the view, so no reason to
	// take the time (and to flicker) by resetting the view color
	SetViewColor(B_TRANSPARENT_COLOR);
	
	active_color.red = (prefs->mini_active_color & 0xff000000) >> 24;
	active_color.green = (prefs->mini_active_color & 0x00ff0000) >> 16;
	active_color.blue = (prefs->mini_active_color & 0x0000ff00) >> 8;
	
	idle_color.red = (prefs->mini_idle_color & 0xff000000) >> 24;
	idle_color.green = (prefs->mini_idle_color & 0x00ff0000) >> 16;
	idle_color.blue = (prefs->mini_idle_color & 0x0000ff00) >> 8;
	
	frame_color.red = (prefs->mini_frame_color & 0xff000000) >> 24;
	frame_color.green = (prefs->mini_frame_color & 0x00ff0000) >> 16;
	frame_color.blue = (prefs->mini_frame_color & 0x0000ff00) >> 8;
}

// These two are only used by DeskbarPulseView, and so do nothing
MiniPulseView::MiniPulseView(BRect rect, const char *name)
 :
 PulseView(rect, name)
{

}

MiniPulseView::MiniPulseView(BMessage *message) 
 :
 PulseView(message)
{

}

// This method is used by DeskbarPulseView as well
void MiniPulseView::Draw(BRect rect) {
	system_info sys_info;
	get_system_info(&sys_info);
	if (sys_info.cpu_count > B_MAX_CPU_COUNT || sys_info.cpu_count <= 0)
		return;
	
	BRect bounds(Bounds());
	SetDrawingMode(B_OP_COPY);

	int h = bounds.IntegerHeight() - 2;
	float top = 1, left = 1;
	float bottom = top + h;
	float bar_width = (bounds.Width()) / sys_info.cpu_count - 2;
	float right = bar_width + left;
	
	for (int x = 0; x < sys_info.cpu_count; x++) {
		int bar_height = (int)(cpu_times[x] * (h + 1));
		if (bar_height > h) bar_height = h;
		double rem = cpu_times[x] * (h + 1) - bar_height;

		rgb_color fraction_color;
		fraction_color.red = (uint8)(idle_color.red + rem 
			* (active_color.red - idle_color.red));
		fraction_color.green = (uint8)(idle_color.green + rem 
			* (active_color.green - idle_color.green));
		fraction_color.blue = (uint8)(idle_color.blue + rem 
			* (active_color.blue - idle_color.blue));
		fraction_color.alpha = 0xff;

		int idle_height = h - bar_height;
		SetHighColor(frame_color);
		StrokeRect(BRect(left - 1, top - 1, right + 1, bottom + 1));
		if (idle_height > 0) {
			SetHighColor(idle_color);
			FillRect(BRect(left, top, right, top + idle_height - 1));
		}
		SetHighColor(fraction_color);
		FillRect(BRect(left, bottom - bar_height, right, bottom - bar_height));
		if (bar_height > 0) {
			SetHighColor(active_color);
			FillRect(BRect(left, bottom - bar_height + 1, right, bottom));
		}
		left += bar_width + 2;
		right += bar_width + 2;
	}
}

void MiniPulseView::Pulse() {
	// Don't recalculate and redraw if this view is hidden
	if (!IsHidden()) {
		Update();
		Draw(Bounds());
	}
}

void MiniPulseView::FrameResized(float width, float height) {
	Draw(Bounds());
}

void MiniPulseView::AttachedToWindow() {
	BMessenger messenger(Window());
	mode1->SetTarget(messenger);
	mode2->SetTarget(messenger);
	preferences->SetTarget(messenger);
	about->SetTarget(messenger);
	quit->SetTarget(messenger);
	
	system_info sys_info;
	get_system_info(&sys_info);
	if (sys_info.cpu_count >= 2) {
		for (int x = 0; x < sys_info.cpu_count; x++) {
			cpu_menu_items[x]->SetTarget(messenger);
		}
	}
}

// Redraw the view with the new colors but do not call
// Update() again - we don't want to recalculate activity
void MiniPulseView::UpdateColors(BMessage *message) {
	int32 ac = message->FindInt32("active_color");
	int32 ic = message->FindInt32("idle_color");
	int32 fc = message->FindInt32("frame_color");
	
	active_color.red = (ac & 0xff000000) >> 24;
	active_color.green = (ac & 0x00ff0000) >> 16;
	active_color.blue = (ac & 0x0000ff00) >> 8;

	idle_color.red = (ic & 0xff000000) >> 24;
	idle_color.green = (ic & 0x00ff0000) >> 16;
	idle_color.blue = (ic & 0x0000ff00) >> 8;

	frame_color.red = (fc & 0xff000000) >> 24;
	frame_color.green = (fc & 0x00ff0000) >> 16;
	frame_color.blue = (fc & 0x0000ff00) >> 8;
	
	Draw(Bounds());
}

MiniPulseView::~MiniPulseView() {

}
