/*
	ProcessController © 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org
	Copyright (c) 2006-2018, Haiku, Inc. All rights reserved.

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "ProcessController.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <AboutWindow.h>
#include <Alert.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <debugger.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Dragger.h>
#include <File.h>
#include <FindDirectory.h>
#include <MessageRunner.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Screen.h>
#include <TextView.h>

#include <scheduler.h>
#include <syscalls.h>

#include "AutoIcon.h"
#include "Colors.h"
#include "IconMenuItem.h"
#include "MemoryBarMenu.h"
#include "MemoryBarMenuItem.h"
#include "PCWorld.h"
#include "Preferences.h"
#include "QuitMenu.h"
#include "TeamBarMenu.h"
#include "TeamBarMenuItem.h"
#include "ThreadBarMenu.h"
#include "Utilities.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ProcessController"


const char* kDeskbarItemName = "ProcessController";
const char* kClassName = "ProcessController";

const char* kFrameColorPref = "deskbar_frame_color";
const char* kIdleColorPref = "deskbar_idle_color";
const char* kActiveColorPref = "deskbar_active_color";

static const char* const kDebuggerSignature
	= "application/x-vnd.Haiku-Debugger";

const rgb_color kKernelBlue = {20, 20, 231,	255};
const rgb_color kIdleGreen = {110, 190,110,	255};

ProcessController* gPCView;
uint32 gCPUcount;
rgb_color gUserColor;
rgb_color gUserColorSelected;
rgb_color gIdleColor;
rgb_color gIdleColorSelected;
rgb_color gKernelColor;
rgb_color gKernelColorSelected;
rgb_color gFrameColor;
rgb_color gFrameColorSelected;
rgb_color gMenuBackColorSelected;
rgb_color gMenuBackColor;
rgb_color gWhiteSelected;
ThreadBarMenu* gCurrentThreadBarMenu;
bool gInDeskbar = false;

#define addtopbottom(x) if (top) popup->AddItem(x); else popup->AddItem(x, 0)

status_t thread_popup(void *arg);

int32			gPopupFlag = 0;
thread_id		gPopupThreadID = 0;

typedef struct {
	BPoint		where;
	BRect		clickToOpenRect;
	bool		top;
} Tpopup_param;

#define DEBUG_THREADS 1

status_t thread_quit_application(void *arg);
status_t thread_debug_thread(void *arg);

typedef struct {
	thread_id	thread;
	sem_id		sem;
	time_t		totalTime;
} Tdebug_thead_param;

// Bar layout depending on number of CPUs
// This is used only in case the replicant width is 16

typedef struct {
	float	cpu_width;
	float	cpu_inter;
	float	mem_width;
} layoutT;

layoutT layout[] = {
	{ 1, 1, 1 },
	{ 5, 1, 5 },	// 1
	{ 3, 1, 4 },	// 2
	{ 2, 1, 3 },
	{ 2, 0, 3 },	// 4
	{ 1, 1, 1 },
	{ 1, 1, 2 },
	{ 1, 1, 1 },
	{ 1, 0, 3 },	// 8
	{ 1, 1, 1 },
	{ 1, 0, 3 },
	{ 1, 0, 2 },
	{ 1, 0, 1 }		// 12
};


extern "C" _EXPORT BView* instantiate_deskbar_item(float maxWidth,
	float maxHeight);

extern "C" _EXPORT BView*
instantiate_deskbar_item(float maxWidth, float maxHeight)
{
	gInDeskbar = true;
	return new ProcessController(maxHeight - 1, maxHeight - 1);
}


//	#pragma mark -


ProcessController::ProcessController(BRect frame, bool temp)
	: BView(frame, kDeskbarItemName, B_FOLLOW_TOP_BOTTOM,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fProcessControllerIcon(kSignature),
	fProcessorIcon(k_cpu_mini),
	fTrackerIcon(kTrackerSig),
	fDeskbarIcon(kDeskbarSig),
	fTerminalIcon(kTerminalSig),
	kCPUCount(sysconf(_SC_NPROCESSORS_CONF)),
	fTemp(temp),
	fLastBarHeight(new float[kCPUCount]),
	fCPUTimes(new double[kCPUCount]),
	fPrevActive(new bigtime_t[kCPUCount])
{
	if (!temp) {
		Init();

		frame.OffsetTo(B_ORIGIN);
		frame.top = frame.bottom - 7;
		frame.left = frame.right - 7;
		BDragger* dragger = new BDragger(frame, this, B_FOLLOW_BOTTOM);
		AddChild(dragger);
	}
}

ProcessController::ProcessController(BMessage *data)
	: BView(data),
	fProcessControllerIcon(kSignature),
	fProcessorIcon(k_cpu_mini),
	fTrackerIcon(kTrackerSig),
	fDeskbarIcon(kDeskbarSig),
	fTerminalIcon(kTerminalSig),
	kCPUCount(sysconf(_SC_NPROCESSORS_CONF)),
	fTemp(false),
	fLastBarHeight(new float[kCPUCount]),
	fCPUTimes(new double[kCPUCount]),
	fPrevActive(new bigtime_t[kCPUCount])
{
	Init();
}


ProcessController::ProcessController(float width, float height)
	:
	BView(BRect (0, 0, width, height), kDeskbarItemName, B_FOLLOW_NONE,
		B_WILL_DRAW),
	fProcessControllerIcon(kSignature),
	fProcessorIcon(k_cpu_mini),
	fTrackerIcon(kTrackerSig),
	fDeskbarIcon(kDeskbarSig),
	fTerminalIcon(kTerminalSig),
	kCPUCount(sysconf(_SC_NPROCESSORS_CONF)),
	fTemp(false),
	fLastBarHeight(new float[kCPUCount]),
	fCPUTimes(new double[kCPUCount]),
	fPrevActive(new bigtime_t[kCPUCount])
{
	Init();
}


ProcessController::~ProcessController()
{
	if (!fTemp) {
		if (gPopupThreadID) {
			status_t return_value;
			wait_for_thread (gPopupThreadID, &return_value);
		}
	}

	delete fMessageRunner;
	gPCView = NULL;

	delete[] fPrevActive;
	delete[] fCPUTimes;
	delete[] fLastBarHeight;
}


void
ProcessController::Init()
{
	memset(fLastBarHeight, 0, sizeof(float) * kCPUCount);
	memset(fCPUTimes, 0, sizeof(double) * kCPUCount);
	memset(fPrevActive, 0, sizeof(bigtime_t) * kCPUCount);

	gPCView = this;
	fMessageRunner = NULL;
	fLastMemoryHeight = 0;
	fPrevTime = 0;
}


void
ProcessController::_HandleDebugRequest(team_id team, thread_id thread)
{
	char paramString[16];
	char idString[16];
	strlcpy(paramString, thread > 0 ? "--thread" : "--team",
		sizeof(paramString));
	snprintf(idString, sizeof(idString), "%" B_PRId32,
		thread > 0 ? thread : team);

	const char* argv[] = {paramString, idString, NULL};
	status_t error = be_roster->Launch(kDebuggerSignature, 2, argv);
	if (error != B_OK) {
		// TODO: notify user
	}
}


ProcessController*
ProcessController::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, kClassName))
		return NULL;

	return new ProcessController(data);
}


status_t
ProcessController::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);
	data->AddString("add_on", kSignature);
	data->AddString("class", kClassName);
	return B_OK;
}


void
ProcessController::MessageReceived(BMessage *message)
{
	team_id team;
	thread_id thread;
	BAlert *alert;
	char	question[1000];
	switch (message->what) {
		case 'Puls':
			Update ();
			DoDraw (false);
			break;

		case 'QtTm':
			if (message->FindInt32("team", &team) == B_OK) {
				resume_thread(spawn_thread(thread_quit_application,
					B_TRANSLATE("Quit application"), B_NORMAL_PRIORITY,
					(void*)(addr_t)team));
			}
			break;

		case 'KlTm':
			if (message->FindInt32("team", &team) == B_OK) {
				info_pack infos;
				if (get_team_info(team, &infos.team_info) == B_OK) {
					get_team_name_and_icon(infos);
					snprintf(question, sizeof(question),
					B_TRANSLATE("What do you want to do with the team \"%s\"?"),
					infos.team_name);
					alert = new BAlert(B_TRANSLATE("Please confirm"), question,
					B_TRANSLATE("Cancel"), B_TRANSLATE("Debug this team!"),
					B_TRANSLATE("Kill this team!"), B_WIDTH_AS_USUAL,
					B_STOP_ALERT);
					alert->SetShortcut(0, B_ESCAPE);
					int result = alert->Go();
					switch (result) {
						case 1:
							_HandleDebugRequest(team, -1);
							break;
						case 2:
							kill_team(team);
							break;
						default:
							break;
					}
				} else {
					alert = new BAlert(B_TRANSLATE("Info"),
						B_TRANSLATE("This team is already gone" B_UTF8_ELLIPSIS),
						B_TRANSLATE("Ok!"), NULL, NULL, B_WIDTH_AS_USUAL,
						B_STOP_ALERT);
					alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
					alert->Go();
				}
			}
			break;

		case 'KlTh':
			if (message->FindInt32("thread", &thread) == B_OK) {
				thread_info	thinfo;
				if (get_thread_info(thread, &thinfo) == B_OK) {
					#if DEBUG_THREADS
					snprintf(question, sizeof(question),
						B_TRANSLATE("What do you want to do "
						"with the thread \"%s\"?"), thinfo.name);
					alert = new BAlert(B_TRANSLATE("Please confirm"), question,
						B_TRANSLATE("Cancel"), B_TRANSLATE("Debug this thread!"),
						B_TRANSLATE("Kill this thread!"), B_WIDTH_AS_USUAL,
						B_STOP_ALERT);
					alert->SetShortcut(0, B_ESCAPE);

					#define KILL 2
					#else
					snprintf(question, sizeof(question),
						B_TRANSLATE("Are you sure you want "
						"to kill the thread \"%s\"?"), thinfo.name);
					alert = new BAlert(B_TRANSLATE("Please confirm"), question,
						B_TRANSLATE("Cancel"), B_TRANSLATE("Kill this thread!"),
						NULL, B_WIDTH_AS_USUAL,	B_STOP_ALERT);
					alert->SetShortcut(0, B_ESCAPE);

					#define KILL 1
					#endif
					alert->SetShortcut(0, B_ESCAPE);
					int r = alert->Go();
					if (r == KILL)
						kill_thread(thread);
					#if DEBUG_THREADS
					else if (r == 1)
						_HandleDebugRequest(thinfo.team, thinfo.thread);
					#endif
				} else {
					alert = new BAlert(B_TRANSLATE("Info"),
						B_TRANSLATE("This thread is already gone" B_UTF8_ELLIPSIS),
						B_TRANSLATE("Ok!"),	NULL, NULL,
						B_WIDTH_AS_USUAL, B_STOP_ALERT);
					alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
					alert->Go();
				}
			}
			break;

		case 'PrTh':
			if (message->FindInt32("thread", &thread) == B_OK) {
				int32 new_priority;
				if (message->FindInt32("priority", &new_priority) == B_OK)
					set_thread_priority(thread, new_priority);
			}
			break;

		case 'Trac':
		{
			BPath trackerPath;
			if (find_directory(B_SYSTEM_DIRECTORY, &trackerPath) == B_OK
				&& trackerPath.Append("Tracker") == B_OK) {
				launch(kTrackerSig, trackerPath.Path());
			}
			break;
		}

		case 'Dbar':
		{
			BPath deskbarPath;
			if (find_directory(B_SYSTEM_DIRECTORY, &deskbarPath) == B_OK
				&& deskbarPath.Append("Deskbar") == B_OK) {
				launch(kDeskbarSig, deskbarPath.Path());
			}
			break;
		}

		case 'Term':
		{
			BPath terminalPath;
			if (find_directory(B_SYSTEM_APPS_DIRECTORY, &terminalPath) == B_OK
				&& terminalPath.Append("Terminal") == B_OK) {
				launch(kTerminalSig, terminalPath.Path());
			}
			break;
		}

		case 'AlDb':
		{
			if (!be_roster->IsRunning(kDeskbarSig)) {
				BPath deskbarPath;
				if (find_directory(B_SYSTEM_DIRECTORY, &deskbarPath) == B_OK
					&& deskbarPath.Append("Deskbar") == B_OK) {
					launch(kDeskbarSig, deskbarPath.Path());
				}
			}
			BDeskbar deskbar;
			if (gInDeskbar || deskbar.HasItem (kDeskbarItemName))
				deskbar.RemoveItem (kDeskbarItemName);
			else
				move_to_deskbar(deskbar);
			break;
		}

		case 'CPU ':
		{
			uint32 cpu;
			if (message->FindInt32("cpu", (int32*)&cpu) == B_OK) {
				bool last = true;
				for (unsigned int p = 0; p < gCPUcount; p++) {
					if (p != cpu && _kern_cpu_enabled(p)) {
						last = false;
						break;
					}
				}
				if (last) {
					alert = new BAlert(B_TRANSLATE("Info"),
						B_TRANSLATE("This is the last active processor"
						B_UTF8_ELLIPSIS "\n"
						"You can't turn it off!"),
						B_TRANSLATE("That's no Fun!"), NULL, NULL,
						B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
					alert->Go();
				} else
					_kern_set_cpu_enabled(cpu, !_kern_cpu_enabled(cpu));
			}
			break;
		}

		case 'Schd':
		{
			int32 mode;
			if (message->FindInt32 ("mode", &mode) == B_OK)
				set_scheduler_mode(mode);
			break;
		}

		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;

		default:
			BView::MessageReceived(message);
	}
}


void
ProcessController::AboutRequested()
{
	BAboutWindow* window = new BAboutWindow(
		B_TRANSLATE_SYSTEM_NAME("ProcessController"), kSignature);

	const char* extraCopyrights[] = {
		"2004 beunited.org",
		"1997-2001 Georges-Edouard Berenger",
		NULL
	};

	const char* authors[] = {
		"Georges-Edouard Berenger",
		NULL
	};

	window->AddCopyright(2007, "Haiku, Inc.", extraCopyrights);
	window->AddAuthors(authors);

	window->Show();
}


void
ProcessController::DefaultColors()
{
	swap_color.red = 203;
	swap_color.green = 0;
	swap_color.blue = 0;
	swap_color.alpha = 255;
	bool set = false;

	if (!set) {
		active_color = kKernelBlue;
		active_color = tint_color (active_color, B_LIGHTEN_2_TINT);
		idle_color = active_color;
		idle_color.green /= 3;
		idle_color.red /= 3;
		idle_color.blue /= 3;
		frame_color = kBlack;
		mix_colors (memory_color, active_color, swap_color, 0.2);
	}
}


void
ProcessController::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Parent())
		SetViewColor(B_TRANSPARENT_COLOR);
	else
		SetViewColor(kBlack);

	Preferences tPreferences(kPreferencesFileName, NULL, false);
	DefaultColors();

	system_info info;
	get_system_info(&info);
	gCPUcount = info.cpu_count;
	Update();

	gIdleColor = kIdleGreen;
	gIdleColorSelected = tint_color(gIdleColor, B_HIGHLIGHT_BACKGROUND_TINT);
	gKernelColor = kKernelBlue;
	gKernelColorSelected = tint_color(gKernelColor, B_HIGHLIGHT_BACKGROUND_TINT);
	gUserColor = tint_color(gKernelColor, B_LIGHTEN_2_TINT);
	gUserColorSelected = tint_color(gUserColor, B_HIGHLIGHT_BACKGROUND_TINT);
	gFrameColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_HIGHLIGHT_BACKGROUND_TINT);
	gFrameColorSelected = tint_color(gFrameColor, B_HIGHLIGHT_BACKGROUND_TINT);
	gMenuBackColor = ui_color(B_MENU_BACKGROUND_COLOR);
	gMenuBackColorSelected = ui_color(B_MENU_SELECTION_BACKGROUND_COLOR);
	gWhiteSelected = tint_color(kWhite, B_HIGHLIGHT_BACKGROUND_TINT);

	BMessenger messenger(this);
	BMessage message('Puls');
	fMessageRunner = new BMessageRunner(messenger, &message, 250000, -1);
}



void
ProcessController::MouseDown(BPoint where)
{
	if (atomic_add(&gPopupFlag, 1) > 0) {
		atomic_add(&gPopupFlag, -1);
		return;
	}

	Tpopup_param* param = new Tpopup_param;
	ConvertToScreen(&where);
	param->where = where;
	param->clickToOpenRect = Frame ();
	ConvertToScreen (&param->clickToOpenRect);
	param->top = where.y < BScreen(this->Window()).Frame().bottom-50;

	gPopupThreadID = spawn_thread(thread_popup, "Popup holder thread",
		B_URGENT_DISPLAY_PRIORITY, param);
	resume_thread(gPopupThreadID);
}


void
ProcessController::Draw(BRect)
{
	SetDrawingMode(B_OP_COPY);
	DoDraw(true);
}


void
ProcessController::DoDraw(bool force)
{
	BRect bounds(Bounds());

	float h = floorf(bounds.Height ()) - 2;
	float top = 1, left = 1;
	float bottom = top + h;
	float barWidth;
	float barGap;
	float memWidth;
	if (gCPUcount <= 12 && bounds.Width() == 15) {
		// Use fixed sizes for small icon sizes
		barWidth = layout[gCPUcount].cpu_width;
		barGap = layout[gCPUcount].cpu_inter;
		memWidth = layout[gCPUcount].mem_width;
	} else {
		memWidth = floorf((bounds.Height() + 1) / 8);
		barGap = ((bounds.Width() + 1) / gCPUcount) > 3 ? 1 : 0;
		barWidth = floorf((bounds.Width() - 1 - memWidth - barGap * gCPUcount)
			/ gCPUcount);
	}
	// interspace
	float right = left + gCPUcount * (barWidth + barGap) - barGap;
	float leftMem = bounds.Width() - memWidth;
		// right of CPU frame...
	if (force && Parent()) {
		SetHighColor(Parent()->ViewColor());
		FillRect(BRect(right + 1, top - 1, leftMem, bottom + 1));
	}

	if (force) {
		SetHighColor(frame_color);
		StrokeRect(BRect(left - 1, top - 1, right, bottom + 1));
		if (gCPUcount > 1 && barGap == 1) {
			for (unsigned int x = 1; x < gCPUcount; x++)
				StrokeLine(BPoint(left + x * barWidth + x - 1, top),
					BPoint(left + x * barWidth + x - 1, bottom));
		}
	}

	if (force)
		StrokeRect(BRect(leftMem - 1, top - 1, leftMem + memWidth, bottom + 1));

	for (unsigned int x = 0; x < gCPUcount; x++) {
		right = left + barWidth - 1;
		float rem = fCPUTimes[x] * (h + 1);
		float barHeight = floorf (rem);
		rem -= barHeight;
		float limit = bottom - barHeight;	// horizontal line
		float previousLimit = bottom - fLastBarHeight[x];
		float idleTop = top;

		if (!force && previousLimit > top)
			idleTop = previousLimit - 1;
		if (limit > idleTop) {
			SetHighColor(idle_color);
			FillRect(BRect(left, idleTop, right, limit - 1));
		}
		if (barHeight <= h) {
			rgb_color fraction_color;
			mix_colors(fraction_color, idle_color, active_color, rem);
			SetHighColor(fraction_color);
			StrokeLine(BPoint(left, bottom - barHeight), BPoint(right,
				bottom - barHeight));
		}
		float active_bottom = bottom;
		if (!force && previousLimit < bottom)
			active_bottom = previousLimit + 1;
		if (limit < active_bottom) {
			SetHighColor(active_color);
			FillRect(BRect(left, limit + 1, right, active_bottom));
		}
		left += barWidth + barGap;
		fLastBarHeight[x] = barHeight;
	}

	float rightMem = bounds.Width() - 1;
	float rem = fMemoryUsage * (h + 1);
	float barHeight = floorf(rem);
	rem -= barHeight;

	rgb_color used_memory_color;
	float sq = fMemoryUsage * fMemoryUsage;
	sq *= sq;
	sq *= sq;
	mix_colors(used_memory_color, memory_color, swap_color, sq);

	float limit = bottom - barHeight;	// horizontal line
	float previousLimit = bottom - fLastMemoryHeight;
	float free_top = top;
	if (!force && previousLimit > top)
		free_top = previousLimit - 1;
	if (limit > free_top) {
		SetHighColor (idle_color);
		FillRect(BRect(leftMem, free_top, rightMem, limit - 1));
	}
	if (barHeight <= h) {
		rgb_color fraction_color;
		mix_colors(fraction_color, idle_color, used_memory_color, rem);
		SetHighColor(fraction_color);
		StrokeLine(BPoint(leftMem, bottom - barHeight), BPoint(rightMem,
			bottom - barHeight));
	}
	float usedBottom = bottom;
//	if (!force && previousLimit < bottom)
//		usedBottom = previousLimit + 1;
	if (limit < usedBottom) {
		SetHighColor(used_memory_color);
		FillRect(BRect(leftMem, limit + 1, rightMem, usedBottom));
	}
	fLastMemoryHeight = barHeight;
}


void
ProcessController::Update()
{
	system_info info;
	get_system_info(&info);
	bigtime_t now = system_time();

	cpu_info* cpuInfos = new cpu_info[gCPUcount];
	get_cpu_info(0, gCPUcount, cpuInfos);

	fMemoryUsage = float(info.used_pages) / float(info.max_pages);
	// Calculate work done since last call to Update() for each CPU
	for (unsigned int x = 0; x < gCPUcount; x++) {
		bigtime_t load = cpuInfos[x].active_time - fPrevActive[x];
		bigtime_t passed = now - fPrevTime;
		float cpuTime = float(load) / float(passed);

		fPrevActive[x] = cpuInfos[x].active_time;
		if (load > passed)
			fPrevActive[x] -= load - passed; // save overload for next period...
		if (cpuTime < 0)
			cpuTime = 0;
		if (cpuTime > 1)
			cpuTime = 1;
		fCPUTimes[x] = cpuTime;
	}
	fPrevTime = now;

	delete[] cpuInfos;
}


//	#pragma mark -


status_t
thread_popup(void *arg)
{
	Tpopup_param* param = (Tpopup_param*) arg;
	int32 mcookie, hcookie;
	unsigned long m;
	long h;
	BMenuItem* item;
	bool top = param->top;

	system_info systemInfo;
	get_system_info(&systemInfo);
	info_pack* infos = new info_pack[systemInfo.used_teams];
	// TODO: this doesn't necessarily get all teams
	for (m = 0, mcookie = 0; m < systemInfo.used_teams; m++) {
		infos[m].team_icon = NULL;
		infos[m].team_name[0] = 0;
		infos[m].thread_info = NULL;
		if (get_next_team_info(&mcookie, &infos[m].team_info) == B_OK) {
			infos[m].thread_info = new thread_info[infos[m].team_info.thread_count];
			for (h = 0, hcookie = 0; h < infos[m].team_info.thread_count; h++) {
				if (get_next_thread_info(infos[m].team_info.team, &hcookie,
						&infos[m].thread_info[h]) != B_OK)
					infos[m].thread_info[h].thread = -1;
			}
			get_team_name_and_icon(infos[m], true);
		} else {
			systemInfo.used_teams = m;
			infos[m].team_info.team = -1;
		}
	}

	BPopUpMenu* popup = new BPopUpMenu("Global Popup", false, false);
	popup->SetFont(be_plain_font);

	// Quit section
	BMenu* QuitPopup = new QuitMenu(B_TRANSLATE("Quit an application"),
	infos, systemInfo.used_teams);
	QuitPopup->SetFont(be_plain_font);
	popup->AddItem(QuitPopup);

	// Memory Usage section
	MemoryBarMenu* MemoryPopup = new MemoryBarMenu(B_TRANSLATE("Memory usage"),
	infos, systemInfo);
	int64 committedMemory = (int64)systemInfo.used_pages * B_PAGE_SIZE / 1024;
	for (m = 0; m < systemInfo.used_teams; m++) {
		if (infos[m].team_info.team >= 0) {
			MemoryBarMenuItem* memoryItem =
				new MemoryBarMenuItem(infos[m].team_name,
					infos[m].team_info.team, infos[m].team_icon, false, NULL);
			MemoryPopup->AddItem(memoryItem);
			memoryItem->UpdateSituation(committedMemory);
		}
	}

	addtopbottom(MemoryPopup);

	// CPU Load section
	TeamBarMenu* CPUPopup = new TeamBarMenu(B_TRANSLATE("Threads and CPU "
	"usage"), infos, systemInfo.used_teams);
	for (m = 0; m < systemInfo.used_teams; m++) {
		if (infos[m].team_info.team >= 0) {
			ThreadBarMenu* TeamPopup = new ThreadBarMenu(infos[m].team_name,
				infos[m].team_info.team, infos[m].team_info.thread_count);
			BMessage* kill_team = new BMessage('KlTm');
			kill_team->AddInt32("team", infos[m].team_info.team);
			TeamBarMenuItem* item = new TeamBarMenuItem(TeamPopup, kill_team,
				infos[m].team_info.team, infos[m].team_icon, false);
			item->SetTarget(gPCView);
			CPUPopup->AddItem(item);
		}
	}

	addtopbottom(CPUPopup);
	addtopbottom(new BSeparatorItem());

	// CPU on/off section
	if (gCPUcount > 1) {
		for (unsigned int i = 0; i < gCPUcount; i++) {
			char item_name[32];
			sprintf (item_name, B_TRANSLATE("Processor %d"), i + 1);
			BMessage* m = new BMessage ('CPU ');
			m->AddInt32 ("cpu", i);
			item = new IconMenuItem (gPCView->fProcessorIcon, item_name, m);
			if (_kern_cpu_enabled(i))
				item->SetMarked (true);
			item->SetTarget(gPCView);
			addtopbottom(item);
		}
		addtopbottom (new BSeparatorItem ());
	}

	// Scheduler modes
	static const char* schedulerModes[] = { B_TRANSLATE_MARK("Low latency"),
		B_TRANSLATE_MARK("Power saving") };
	unsigned int modesCount = sizeof(schedulerModes) / sizeof(const char*);
	int32 currentMode = get_scheduler_mode();
	for (unsigned int i = 0; i < modesCount; i++) {
		BMessage* m = new BMessage('Schd');
		m->AddInt32("mode", i);
		item = new BMenuItem(B_TRANSLATE(schedulerModes[i]), m);
		if ((uint32)currentMode == i)
			item->SetMarked(true);
		item->SetTarget(gPCView);
		addtopbottom(item);
	}
	addtopbottom(new BSeparatorItem());

	if (!be_roster->IsRunning(kTrackerSig)) {
		item = new IconMenuItem(gPCView->fTrackerIcon,
		B_TRANSLATE("Restart Tracker"), new BMessage('Trac'));
		item->SetTarget(gPCView);
		addtopbottom(item);
	}
	if (!be_roster->IsRunning(kDeskbarSig)) {
		item = new IconMenuItem(gPCView->fDeskbarIcon,
		B_TRANSLATE("Restart Deskbar"), new BMessage('Dbar'));
		item->SetTarget(gPCView);
		addtopbottom(item);
	}

	item = new IconMenuItem(gPCView->fTerminalIcon,
	B_TRANSLATE("New Terminal"), new BMessage('Term'));
	item->SetTarget(gPCView);
	addtopbottom(item);

	addtopbottom(new BSeparatorItem());

	bool showLiveInDeskbarItem = gInDeskbar;
	if (!showLiveInDeskbarItem) {
		int32 cookie = 0;
		image_info info;
		while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
			if (info.type == B_APP_IMAGE) {
				// only show the Live in Deskbar item if a) we're running in
				// deskbar itself, or b) we're running in PC's team.
				if (strstr(info.name, "ProcessController") != NULL) {
					showLiveInDeskbarItem = true;
					break;
				}
			}
		}
	}

	if (showLiveInDeskbarItem && be_roster->IsRunning(kDeskbarSig)) {
		item = new BMenuItem(B_TRANSLATE("Live in the Deskbar"),
			new BMessage('AlDb'));
		BDeskbar deskbar;
		item->SetMarked(gInDeskbar || deskbar.HasItem(kDeskbarItemName));
		item->SetTarget(gPCView);
		addtopbottom(item);
		addtopbottom(new BSeparatorItem ());
	}


	item = new IconMenuItem(gPCView->fProcessControllerIcon,
	B_TRANSLATE("About ProcessController" B_UTF8_ELLIPSIS),
		new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget(gPCView);
	addtopbottom(item);

	param->where.x -= 5;
	param->where.y -= 8;
	popup->Go(param->where, true, true, param->clickToOpenRect);

	delete popup;
	for (m = 0; m < systemInfo.used_teams; m++) {
		if (infos[m].team_info.team >= 0) {
			delete[] infos[m].thread_info;
			delete infos[m].team_icon;
		}
	}
	delete[] infos;
	delete param;
	atomic_add (&gPopupFlag, -1);
	gPopupThreadID = 0;

	return B_OK;
}


status_t
thread_quit_application(void *arg)
{
	BMessenger messenger(NULL, (addr_t)arg);
	messenger.SendMessage(B_QUIT_REQUESTED);
	return B_OK;
}


status_t
thread_debug_thread(void *arg)
{
	Tdebug_thead_param*	param = (Tdebug_thead_param*) arg;
	debug_thread(param->thread);
	delete param;
	return B_OK;
}
