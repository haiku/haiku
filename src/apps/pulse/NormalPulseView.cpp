//****************************************************************************************
//
//	File:		NormalPulseView.cpp
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "NormalPulseView.h"
#include "Common.h"
#include "Pictures"
#include <interface/Bitmap.h>
#include <interface/Dragger.h>
#include <Window.h>
#include <stdlib.h>
#include <stdio.h>

NormalPulseView::NormalPulseView(BRect rect) : PulseView(rect, "NormalPulseView") {
	rgb_color color = { 168, 168, 168, 0xff };
	SetViewColor(color);
	SetLowColor(color);

	mode1->SetLabel("Mini Mode");
	mode1->SetMessage(new BMessage(PV_MINI_MODE));
	mode2->SetLabel("Deskbar Mode");
	mode2->SetMessage(new BMessage(PV_DESKBAR_MODE));
	
	DetermineVendorAndProcessor();

	// Allocate progress bars and button pointers
	system_info sys_info;
	get_system_info(&sys_info);
	progress_bars = new ProgressBar *[sys_info.cpu_count];
	cpu_buttons = new CPUButton *[sys_info.cpu_count];
	
	// Set up the CPU activity bars and buttons
	for (int x = 0; x < sys_info.cpu_count; x++) {
		BRect r(PROGRESS_MLEFT, PROGRESS_MTOP + ITEM_OFFSET * x,
			PROGRESS_MLEFT + ProgressBar::PROGRESS_WIDTH,
			PROGRESS_MTOP + ITEM_OFFSET * x + ProgressBar::PROGRESS_HEIGHT);
		progress_bars[x] = new ProgressBar(r, "CPU Progress Bar");
		AddChild(progress_bars[x]);
		
		r.Set(CPUBUTTON_MLEFT, CPUBUTTON_MTOP + ITEM_OFFSET * x,
			CPUBUTTON_MLEFT + CPUBUTTON_WIDTH,
			CPUBUTTON_MTOP + ITEM_OFFSET * x + CPUBUTTON_HEIGHT);
		char temp[4];
		sprintf(temp, "%d", x + 1);
		cpu_buttons[x] = new CPUButton(r, "CPUButton", temp, NULL);
		AddChild(cpu_buttons[x]);
		
		//	If there is only 1 cpu it will be hidden below
		//	thus, no need to add the dragger as it will still
		//	be visible when replicants are turned on
		if (sys_info.cpu_count > 1) {
			BRect dragger_rect;
			dragger_rect = r;
			dragger_rect.top = dragger_rect.bottom;
			dragger_rect.left = dragger_rect.right;
			dragger_rect.bottom += 7;
			dragger_rect.right += 7;
			dragger_rect.OffsetBy(-1, -1);
			BDragger *dragger = new BDragger(dragger_rect, cpu_buttons[x], 0);
			AddChild(dragger);
		}
	}
	
	if (sys_info.cpu_count == 1) {
		progress_bars[0]->MoveBy(-3, 12);
		cpu_buttons[0]->Hide();
	}
}

int NormalPulseView::CalculateCPUSpeed() {
	system_info sys_info;
	get_system_info(&sys_info);

	int target = sys_info.cpu_clock_speed / 1000000;
	int frac = target % 100;
	int delta = -frac;
	int at = 0;
	int freqs[] = { 100, 50, 25, 75, 33, 67, 20, 40, 60, 80, 10, 30, 70, 90 };

	for (uint x = 0; x < sizeof(freqs) / sizeof(freqs[0]); x++) {
		int ndelta = freqs[x] - frac;
		if (abs(ndelta) < abs(delta)) {
			at = freqs[x];
			delta = ndelta;
		}
	}
	return target + delta;
}

void NormalPulseView::DetermineVendorAndProcessor() {
	system_info sys_info;
	get_system_info(&sys_info);

	// Initialize logos
	BRect r(0, 0, 63, 62);
	cpu_logo = new BBitmap(r, B_COLOR_8_BIT);
#if __POWERPC__
	cpu_logo->SetBits(Anim1, 11718, 0, B_COLOR_8_BIT);
#endif
#if __INTEL__
	if (sys_info.cpu_type < B_CPU_AMD_X86) {
		cpu_logo->SetBits(IntelLogo, 11718, 0, B_COLOR_8_BIT);
	} else {
		cpu_logo->SetBits(BlankLogo, 11718, 0, B_COLOR_8_BIT);
	}
#endif
	
#if __INTEL__
	// Determine x86 vendor name - Intel just set for completeness
	switch (sys_info.cpu_type & B_CPU_X86_VENDOR_MASK) {
		case B_CPU_INTEL_X86:
			strcpy(vendor, "Intel");
			break;
		case B_CPU_AMD_X86:
			strcpy(vendor, "AMD");
			break;
		case B_CPU_CYRIX_X86:
			strcpy(vendor, "CYRIX");
			break;
		case B_CPU_IDT_X86:
			strcpy(vendor, "IDT");
			break;
		case B_CPU_RISE_X86:
			strcpy(vendor, "RISE");
			break;
		default:
			strcpy(vendor, "Unknown");
			break;
	}
#endif

	// Determine CPU type
	switch(sys_info.cpu_type) {
#if __POWERPC__
		case B_CPU_PPC_603:
			strcpy(processor, "603");
			break;
		case B_CPU_PPC_603e:
			strcpy(processor, "603e");
			break;
		case B_CPU_PPC_750:
			strcpy(processor, "750");
			break;
		case B_CPU_PPC_604:
			strcpy(processor, "604");
			break;
		case B_CPU_PPC_604e:
			strcpy(processor, "604e");
			break;
#endif

#if __INTEL__
		case B_CPU_X86:
			strcpy(processor, "Unknown x86");
			break;
		case B_CPU_INTEL_PENTIUM:
		case B_CPU_INTEL_PENTIUM75:
			strcpy(processor, "Pentium");
			break;
		case B_CPU_INTEL_PENTIUM_486_OVERDRIVE:
		case B_CPU_INTEL_PENTIUM75_486_OVERDRIVE:
			strcpy(processor, "Pentium OD");
			break;
		case B_CPU_INTEL_PENTIUM_MMX:
		case B_CPU_INTEL_PENTIUM_MMX_MODEL_8:
			strcpy(processor, "Pentium MMX");
			break;
		case B_CPU_INTEL_PENTIUM_PRO:
			strcpy(processor, "Pentium Pro");
			break;
		case B_CPU_INTEL_PENTIUM_II_MODEL_3:
		case B_CPU_INTEL_PENTIUM_II_MODEL_5:
			strcpy(processor, "Pentium II");
			break;
		case B_CPU_INTEL_CELERON:
			strcpy(processor, "Celeron");
			break;
		case B_CPU_INTEL_PENTIUM_III:
			strcpy(processor, "Pentium III");
			break;
		case B_CPU_AMD_K5_MODEL0:
		case B_CPU_AMD_K5_MODEL1:
		case B_CPU_AMD_K5_MODEL2:
		case B_CPU_AMD_K5_MODEL3:
			strcpy(processor, "K5");
			break;
		case B_CPU_AMD_K6_MODEL6:
		case B_CPU_AMD_K6_MODEL7:
			strcpy(processor, "K6");
			break;
		case B_CPU_AMD_K6_2:
			strcpy(processor, "K6-2");
			break;
		case B_CPU_AMD_K6_III:
			strcpy(processor, "K6-III");
			break;
		case B_CPU_AMD_ATHLON_MODEL1:
			strcpy(processor, "Athlon");
			break;
		case B_CPU_CYRIX_GXm:
			strcpy(processor, "GXm");
			break;
		case B_CPU_CYRIX_6x86MX:
			strcpy(processor, "6x86MX");
			break;
		case B_CPU_IDT_WINCHIP_C6:
			strcpy(processor, "WinChip C6");
			break;
		case B_CPU_IDT_WINCHIP_2:
			strcpy(processor, "WinChip 2");
			break;
		case B_CPU_RISE_mP6:
			strcpy(processor, "mP6");
			break;
#endif
		default:
			strcpy(processor, "Unknown");
			break;
	}
}

void NormalPulseView::Draw(BRect rect) {
	PushState();

	// Black frame
	SetHighColor(0, 0, 0);
	BRect frame = Bounds();
	frame.right--;
	frame.bottom--;
	StrokeRect(frame);

	// Bevelled edges
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(1, 1), BPoint(frame.right - 1, 1));
	StrokeLine(BPoint(1, 1), BPoint(1, frame.bottom - 1));
	SetHighColor(80, 80, 80);
	StrokeLine(BPoint(frame.right, 1), BPoint(frame.right, frame.bottom));
	StrokeLine(BPoint(2, frame.bottom), BPoint(frame.right - 1, frame.bottom));

	// Dividing line
	SetHighColor(96, 96, 96);
	StrokeLine(BPoint(1, frame.bottom + 1), BPoint(frame.right, frame.bottom + 1));
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(1, frame.bottom + 2), BPoint(frame.right, frame.bottom + 2));
	
	// Processor picture
	DrawBitmap(cpu_logo, BPoint(10, 10));
	
#if __INTEL__
	// Do nothing in the case of Intel CPUs - they already have a logo
	if (strcmp(vendor, "Intel") != 0) {
		SetDrawingMode(B_OP_OVER);
		SetHighColor(240,240,240);
	
		float width = StringWidth(vendor);
		MovePenTo(10 + (32 - width / 2), 30);
		DrawString(vendor);
	}
#endif

	// Draw processor type and speed
	char buf[500];
	sprintf(buf, "%d MHz", CalculateCPUSpeed());
	SetDrawingMode(B_OP_OVER);
	SetHighColor(240, 240, 240);

	float width = StringWidth(processor);
	MovePenTo(10 + (32 - width / 2), 48);
	DrawString(processor);

	width = StringWidth(buf);
	MovePenTo(10 + (32 - width / 2), 60);
	DrawString(buf);
	
	PopState();
}

void NormalPulseView::Pulse() {
	// Don't recalculate and redraw if this view is hidden
	if (!IsHidden()) {
		Update();
		if (Window()->Lock()) {
			system_info sys_info;
			get_system_info(&sys_info);
		
			// Set the value of each CPU bar
			for (int x = 0; x < sys_info.cpu_count; x++) {
				progress_bars[x]->Set(max_c(0, cpu_times[x] * 100));
			}
			
			Sync();
			Window()->Unlock();
		}
	}
}

void NormalPulseView::AttachedToWindow() {
	// Use a smaller font on x86 to accomodate longer processor names
	SetFont(be_bold_font);
#if __INTEL__
	SetFontSize(7);
#else
	SetFontSize(9);
#endif
	prev_time = system_time();
	
	BMessenger messenger(Window());
	mode1->SetTarget(messenger);
	mode2->SetTarget(messenger);
	preferences->SetTarget(messenger);
	about->SetTarget(messenger);
	
	system_info sys_info;
	get_system_info(&sys_info);
	if (sys_info.cpu_count >= 2) {
		for (int x = 0; x < sys_info.cpu_count; x++) {
			cpu_menu_items[x]->SetTarget(messenger);
		}
	}
}

void NormalPulseView::UpdateColors(BMessage *message) {
	int32 color = message->FindInt32("color");
	bool fade = message->FindBool("fade");
	system_info sys_info;
	get_system_info(&sys_info);
	
	for (int x = 0; x < sys_info.cpu_count; x++) {
		progress_bars[x]->UpdateColors(color, fade);
		cpu_buttons[x]->UpdateColors(color);
	}
}

NormalPulseView::~NormalPulseView() {
	delete cpu_logo;
	delete cpu_buttons;
	delete progress_bars;
}