/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ThreadBarMenuItem.h"

#include "Colors.h"
#include "PriorityMenu.h"
#include "ProcessController.h"
#include "Utilities.h"

#include <stdio.h>


ThreadBarMenuItem::ThreadBarMenuItem(const char* title, thread_id thread,
		BMenu *menu, BMessage* msg)
	: BMenuItem(menu, msg), fThreadID(thread)
{
	SetLabel(title);
	get_thread_info(fThreadID, &fThreadInfo);
	fLastTime = system_time();
	fKernel = -1;
	fGrenze1 = -1;
	fGrenze2 = -1;
}


void
ThreadBarMenuItem::DrawContent()
{
	if (fKernel < 0)
		BarUpdate();
	DrawBar(true);
	Menu()->MovePenTo(ContentLocation());
	BMenuItem::DrawContent();
}


void
ThreadBarMenuItem::DrawBar(bool force)
{
	bool selected = IsSelected();
	BRect frame = Frame();
	BMenu* menu = Menu();
	rgb_color highColor = menu->HighColor();

	BFont font;
	menu->GetFont(&font);
	frame = bar_rect(frame, &font);

	if (fKernel < 0)
		return;

	if (fGrenze1 < 0)
		force = true;

	if (force) {
		if (selected)
			menu->SetHighColor(gFrameColorSelected);
		else
			menu->SetHighColor(gFrameColor);
		menu->StrokeRect(frame);
	}

	frame.InsetBy(1, 1);
	BRect r = frame;
	float grenze1 = frame.left + (frame.right - frame.left) * fKernel;
	float grenze2 = frame.left + (frame.right - frame.left) * (fKernel + fUser);
	if (grenze1 > frame.right)
		grenze1 = frame.right;
	if (grenze2 > frame.right)
		grenze2 = frame.right;
	r.right = grenze1;

	if (!force)
		r.left = fGrenze1;

	if (r.left < r.right) {
		if (selected)
			menu->SetHighColor(gKernelColorSelected);
		else
			menu->SetHighColor(gKernelColor);
		menu->FillRect(r);
	}

	r.left = grenze1;
	r.right = grenze2;
	if (!force) {
		if (fGrenze2 > r.left && r.left >= fGrenze1)
			r.left = fGrenze2;
		if (fGrenze1 < r.right && r.right  <=  fGrenze2)
			r.right = fGrenze1;
	}
	if (r.left < r.right) {
		thread_info threadInfo;
		bool idleThread = false;
		if (get_thread_info(fThreadID, &threadInfo) == B_OK)
			idleThread = threadInfo.priority == B_IDLE_PRIORITY;

		if (selected) {
			menu->SetHighColor(
				idleThread ? gIdleColorSelected : gUserColorSelected);
		} else
			menu->SetHighColor(idleThread ? gIdleColor : gUserColor);
		menu->FillRect(r);
	}
	r.left = grenze2;
	r.right = frame.right;
	if (!force)
		r.right = fGrenze2;
	if (r.left < r.right) {
		if (selected)
			menu->SetHighColor(gWhiteSelected);
		else
			menu->SetHighColor(kWhite);
		menu->FillRect(r);
	}
	menu->SetHighColor(highColor);
	fGrenze1 = grenze1;
	fGrenze2 = grenze2;
}


void
ThreadBarMenuItem::GetContentSize(float* width, float* height)
{
	BMenuItem::GetContentSize(width, height);
	*width += 10 + kBarWidth;
}


void
ThreadBarMenuItem::Highlight(bool on)
{
	if (on) {
		PriorityMenu* popup = (PriorityMenu*)Submenu();
		if (popup)
			popup->Update(fThreadInfo.priority);
	}
	BMenuItem::Highlight(on);
}


void
ThreadBarMenuItem::BarUpdate()
{
	thread_info info;
	if (get_thread_info(fThreadID, &info) == B_OK) {
		bigtime_t now = system_time();
		fKernel = double(info.kernel_time - fThreadInfo.kernel_time) / double(now - fLastTime);
		fUser = double(info.user_time - fThreadInfo.user_time) / double(now - fLastTime);
		if (info.priority == B_IDLE_PRIORITY) {
			fUser += fKernel;
			fKernel = 0;
		}
		fThreadInfo.user_time = info.user_time;
		fThreadInfo.kernel_time = info.kernel_time;
		fLastTime = now;
		if (IsSelected ()) {
			PriorityMenu* popup = (PriorityMenu*)Submenu();
			if (popup && info.priority != fThreadInfo.priority)
				popup->Update (info.priority);
		}
		fThreadInfo.priority = info.priority;
	} else
		fKernel = -1;
}
