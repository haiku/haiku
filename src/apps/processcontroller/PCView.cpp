/*
	PCView.cpp

	ProcessController © 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org 

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

#include "PCView.h"
#include "IconMenuItem.h"
#include "PCWorld.h"
#include "AboutPC.h"
#include "Preferences.h"
#include "PCUtils.h"
#include "Colors.h"

#include <Alert.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Dragger.h>
#include <File.h>
#include <FindDirectory.h>
#include <MessageRunner.h>
#include <Path.h>
#include <Roster.h>
#include <Screen.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* kDeskbarItemName = "ProcessController";
const char* kClassName = "ProcessController";

const char* kMimicPulsePref = "mimic_pulse";
const char* kFrameColorPref = "deskbar_frame_color";
const char* kIdleColorPref = "deskbar_idle_color";
const char* kActiveColorPref = "deskbar_active_color";

const char* kPuseSettings = "Pulse_settings";

const rgb_color kKernelBlue =	{20, 20, 231,	255};
const rgb_color kIdleGreen = {110, 190,110,	255};

ProcessController*	gPCView;
int32				gCPUcount;
rgb_color 			gUserColor;
rgb_color 			gUserColorSelected;
rgb_color 			gIdleColor;
rgb_color 			gIdleColorSelected;
rgb_color 			gKernelColor;
rgb_color 			gKernelColorSelected;
rgb_color			gFrameColor;
rgb_color			gFrameColorSelected;
rgb_color			gMenuBackColorSelected;
rgb_color			gMenuBackColor;
rgb_color			gWhiteSelected;
ThreadBarMenu*		gCurrentThreadBarMenu;
bool				gInDeskbar = false;
int32				gMimicPulse = 0;

#define DEBUG_THREADS 1

long thread_quit_application(void *arg);
long thread_debug_thread(void *arg);

typedef struct {
	thread_id	thread;
	sem_id		sem;
	time_t		totalTime;
} Tdebug_thead_param;

extern "C" _EXPORT BView *instantiate_deskbar_item(void);
extern "C" _EXPORT BView *instantiate_deskbar_item(void)
{
	gInDeskbar = true;
	return new ProcessController ();
}

ProcessController::ProcessController(BRect frame, bool temp)
	:BView(frame, kDeskbarItemName, B_FOLLOW_NONE, B_WILL_DRAW),
	fProcessControllerIcon (kSignature), fProcessorIcon (k_cpu_mini), 
	fTrackerIcon (kTrackerSig), fDeskbarIcon (kDeskbarSig), fTerminalIcon (kTerminalSig),
	fTemp(temp)
{
	if (!temp) {
		Init();
		frame.OffsetTo(B_ORIGIN);
		frame.top = frame.bottom - 7;
		frame.left = frame.right - 7;
		BDragger*	dw = new BDragger(frame, this);
		AddChild(dw);
	}
}

ProcessController::ProcessController(BMessage *data):BView(data),
	fProcessControllerIcon (kSignature), fProcessorIcon (k_cpu_mini), 
	fTrackerIcon (kTrackerSig), fDeskbarIcon (kDeskbarSig), fTerminalIcon (kTerminalSig),
	fTemp (false)
{
	Init();
}

ProcessController::ProcessController ()
	:BView(BRect (0, 0, 15, 15), kDeskbarItemName, B_FOLLOW_NONE, B_WILL_DRAW),
	fProcessControllerIcon (kSignature), fProcessorIcon (k_cpu_mini), 
	fTrackerIcon (kTrackerSig), fDeskbarIcon (kDeskbarSig), fTerminalIcon (kTerminalSig),
	fTemp (false)
{
	Init();
}

ProcessController::~ProcessController()
{
	if (!fTemp) {
		if (gPopupThreadID)
		{
			status_t	return_value;
			wait_for_thread (gPopupThreadID, &return_value);
		}
	}
	delete fMessageRunner;
	gPCView = NULL;
}

void ProcessController::Init()
{
	gPCView = this;
	fMessageRunner = NULL;
	memset(fLastBarHeight, 0, sizeof(fLastBarHeight));
	fLastMemoryHeight = 0;
	memset(fCPUTimes, 0, sizeof(fCPUTimes));
	memset(fPrevActive, 0, sizeof(fPrevActive));
	fPrevTime = 0;
}

ProcessController *ProcessController::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, kClassName))
		return NULL;
	return new ProcessController(data);
}

status_t ProcessController::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);
	data->AddString("add_on", kSignature);
	data->AddString("class", kClassName);
	return B_OK;
}

void ProcessController::MessageReceived(BMessage *message)
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
			if (message->FindInt32("team", &team) == B_OK)
				resume_thread(spawn_thread(thread_quit_application, "Quit application", B_NORMAL_PRIORITY, (void*) team));
			break;

		case 'KlTm':
			if (message->FindInt32("team", &team) == B_OK) {
				infosPack	infos;
				if (get_team_info(team, &infos.tminfo) == B_OK) {
					get_team_name_and_icon(infos);
					sprintf(question, "Do you really want to kill the team \"%s\"?", infos.tmname);
					alert = new BAlert("", question, "Cancel", "Yes, Kill this Team!", NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
					alert->SetShortcut(0, B_ESCAPE);
					if (alert->Go())
						kill_team(team);
				} else {
					alert = new BAlert("", "This team is already gone...", "Ok!", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
					alert->Go();
				}
			}
			break;

		case 'KlTh':
			if (message->FindInt32("thread", &thread) == B_OK) {
				thread_info	thinfo;
				if (get_thread_info(thread, &thinfo) == B_OK) {
					#if DEBUG_THREADS
					sprintf(question, "What do you want to do with the thread \"%s\"?", thinfo.name);
					alert = new BAlert("", question, "Cancel", "Debug this Thread!", "Kill this thread!", B_WIDTH_AS_USUAL, B_STOP_ALERT);
					#define KILL 2
					#else
					sprintf(question, "Are you sure you want to kill the thread \"%s\"?", thinfo.name);
					alert = new BAlert("", question, "Cancel", "Kill this thread!", NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
					#define KILL 1
					#endif
					alert->SetShortcut(0, B_ESCAPE);
					int r = alert->Go();
					if (r == KILL)
						kill_thread(thread);
					#if DEBUG_THREADS
					else if (r == 1) {
						Tdebug_thead_param* param = new Tdebug_thead_param;
						param->thread = thread;
						if (thinfo.state == B_THREAD_WAITING)
							param->sem = thinfo.sem;
						else
							param->sem = -1;
						param->totalTime = thinfo.user_time+thinfo.kernel_time;
						resume_thread(spawn_thread(thread_debug_thread, "Debug thread", B_NORMAL_PRIORITY, param));
					}
					#endif
				} else {
					alert = new BAlert("", "This thread is already gone...", "Ok!", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
					alert->Go();
				}
			}
			break;

		case 'PrTh':
			if (message->FindInt32("thread", &thread) == B_OK) {
				long new_priority;
				if (message->FindInt32("priority", &new_priority) == B_OK)
					set_thread_priority(thread, new_priority);
			}
			break;
			
		case 'Trac':
			launch (kTrackerSig, "/boot/beos/system/Tracker");
			break;

		case 'Dbar':
			launch (kDeskbarSig, "/boot/beos/system/Deskbar");
			break;
		
		case 'Term':
			launch (kTerminalSig, "/boot/beos/apps/Terminal");
			break;

		case 'AlDb':
			{
				if (!be_roster->IsRunning(kDeskbarSig))
					launch (kDeskbarSig, "/boot/beos/system/Deskbar");
				BDeskbar db;
				if (gInDeskbar || db.HasItem (kDeskbarItemName))
					db.RemoveItem (kDeskbarItemName);
				else
					move_to_deskbar (db);
			}
			break;

		case 'CPU ':
			{
				int32 cpu;
				if (message->FindInt32 ("cpu", &cpu) == B_OK)
				{
					bool last = true;
					for (int p = 0; p < gCPUcount; p++)
						if (p != cpu && _kget_cpu_state_ (p))
						{
							last = false;
							break;
						}
					if (last)
					{
						alert = new BAlert("", "This is the last active processor...\nYou can't turn it off!",
							"That's no Fun!", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
						alert->Go ();
					}
					else
						_kset_cpu_state_ (cpu, !_kget_cpu_state_ (cpu));
				}
			}
			break;
		
		case 'Colo':
			{
				GebsPreferences tPreferences (kPreferencesFileName);
				gMimicPulse = (gMimicPulse == 0);
				tPreferences.SaveInt32 (gMimicPulse, kMimicPulsePref);
				DefaultColors ();
			}
			break;

		case 'Docu':
			{
				entry_ref	ref;
				find_self (ref);
				BEntry		entry (&ref);
				BDirectory	parent;
				entry.GetParent (&parent);
				entry.SetTo (&parent, "ProcessController's Doc/ProcessController's Notice.html");	
				BPath path (&entry);
				if (path.InitCheck () == B_OK)
				{
					char	url[B_PATH_NAME_LENGTH + 64];
					sprintf (url, "file://%s", path.Path ());
					char*		argv[2];
					argv[0] = url;
					argv[1] = 0;
					be_roster->Launch ("text/html", 1, argv);
				}
				else
				{
					alert = new BAlert("", "ProcessController's documentation could not be found. You may want to reinstall ProcessController...",
						"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					alert->Go ();
				}
			}
			break;

		case B_ABOUT_REQUESTED:
			new AboutPC (BScreen ().Frame ());
			break;
		
		default:
			BView::MessageReceived (message);
	}
}

void ProcessController::DefaultColors ()
{
	swap_color.red = 203;
	swap_color.green = 0;
	swap_color.blue = 0;
	swap_color.alpha = 255;
	bool set = false;
	if (gMimicPulse)
	{
		BPath	prefpath;
		if (find_directory (B_USER_SETTINGS_DIRECTORY, &prefpath) == B_OK)
		{
			BDirectory	prefdir (prefpath.Path ());
			BEntry		entry;
			prefdir.FindEntry (kPuseSettings, &entry);
			BFile		file (&entry, B_READ_ONLY);
			if (file.InitCheck() == B_OK)
			{
				int32	f, i, a;
				if (file.ReadAttr(kFrameColorPref, B_INT32_TYPE, 0, &f, 4) == 4 && 
					file.ReadAttr(kActiveColorPref, B_INT32_TYPE, 0, &a, 4) == 4 && 
					file.ReadAttr(kIdleColorPref, B_INT32_TYPE, 0, &i, 4) == 4)
				{
					active_color.red = (a & 0xff000000) >> 24;
					active_color.green = (a & 0x00ff0000) >> 16;
					active_color.blue = (a & 0x0000ff00) >> 8;
					active_color.alpha = 255;
					
					idle_color.red = (i & 0xff000000) >> 24;
					idle_color.green = (i & 0x00ff0000) >> 16;
					idle_color.blue = (i & 0x0000ff00) >> 8;
					idle_color.alpha = 255;
					
					frame_color.red = (f & 0xff000000) >> 24;
					frame_color.green = (f & 0x00ff0000) >> 16;
					frame_color.blue = (f & 0x0000ff00) >> 8;
					frame_color.alpha = 255;
					
					mix_colors (memory_color, active_color, swap_color, 0.8);
					
					set = true;
				}
				else
				{
					BAlert * alert = new BAlert("", "I couldn't read Pulse's preferences...",
						"Sorry!", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					alert->Go ();
				}
			}
			else
			{
				BAlert * alert = new BAlert("", "I couldn't find Pulse's preferences...",
					"Sorry!", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go ();
			}
		}
	}
	if (!set)
	{
		active_color = kKernelBlue;
		active_color = tint_color (active_color, B_LIGHTEN_2_TINT);
		idle_color = active_color;
		idle_color.green /= 3;
		idle_color.red /= 3;
		idle_color.blue /= 3;
		frame_color = kBlack;
		mix_colors (memory_color, active_color, swap_color, 0.2);
	}
//	idle_color = kWhite;
}

void ProcessController::AttachedToWindow ()
{
	BView::AttachedToWindow ();
	if (Parent ())
		SetViewColor (B_TRANSPARENT_COLOR);
	else
		SetViewColor (kBlack);
	GebsPreferences tPreferences (kPreferencesFileName, NULL, false);
	tPreferences.ReadInt32 (gMimicPulse, kMimicPulsePref);
	DefaultColors ();
	system_info sys_info;
	get_system_info (&sys_info);
	gCPUcount = sys_info.cpu_count;
	Update ();
	gIdleColor = kIdleGreen;
	gIdleColorSelected = tint_color (gIdleColor, B_HIGHLIGHT_BACKGROUND_TINT);
	gKernelColor = kKernelBlue;
	gKernelColorSelected = tint_color (gKernelColor, B_HIGHLIGHT_BACKGROUND_TINT);
//	gKernelColor = tint_color(gUserColor, B_DARKEN_1_TINT);
	gUserColor = tint_color (gKernelColor, B_LIGHTEN_2_TINT);
	gUserColorSelected = tint_color (gUserColor, B_HIGHLIGHT_BACKGROUND_TINT);
	gFrameColor = tint_color (ui_color (B_PANEL_BACKGROUND_COLOR), B_HIGHLIGHT_BACKGROUND_TINT);
	gFrameColorSelected = tint_color (gFrameColor, B_HIGHLIGHT_BACKGROUND_TINT);
	gMenuBackColor = ui_color (B_MENU_BACKGROUND_COLOR);
	// Depending on which version of the system we use, choose the menu selection color...
	if (before_dano())
		gMenuBackColorSelected = tint_color (gMenuBackColor, B_HIGHLIGHT_BACKGROUND_TINT);	// R5 & before
	else
		gMenuBackColorSelected = ui_color (B_MENU_SELECTION_BACKGROUND_COLOR);				// Dano & up
	gWhiteSelected = tint_color (kWhite, B_HIGHLIGHT_BACKGROUND_TINT);
	BMessenger	messenger (this);
	BMessage	message ('Puls');
	fMessageRunner = new BMessageRunner (messenger, &message, 250000, -1);
}

typedef struct {
	float	cpu_width;
	float	cpu_inter;
	float	mem_width;
} layoutT;

layoutT layout[] = {
	{ 1, 1, 1 },
	{ 5, 1, 5 },	// 1
	{ 3, 1, 4 },	// 2
	{ 1, 1, 1 },
	{ 2, 0, 3 },	// 4
	{ 1, 1, 1 },
	{ 1, 1, 1 },
	{ 1, 1, 1 },
	{ 1, 0, 3 } };	// 8

void ProcessController::Draw(BRect)
{
	SetDrawingMode (B_OP_COPY);
	DoDraw (true);
}

void ProcessController::DoDraw (bool force)
{
	//gCPUcount = 1;
	BRect bounds (Bounds ());

	float h = floorf (bounds.Height ()) - 2;
	float top = 1, left = 1;
	float bottom = top + h;
	float bar_width = layout[gCPUcount].cpu_width;
	// interspace
	float right = left + gCPUcount * (bar_width + layout[gCPUcount].cpu_inter) - layout[gCPUcount].cpu_inter; // right of CPU frame...
	if (force && Parent ())
	{
		SetHighColor (Parent ()->ViewColor ());
		FillRect (BRect (right + 1, top - 1, right + 2, bottom + 1));
	}

	if (force)
	{
		SetHighColor (frame_color);
		StrokeRect (BRect (left - 1, top - 1, right, bottom + 1));
		if (gCPUcount == 2)
			StrokeLine (BPoint (left + bar_width, top), BPoint (left + bar_width, bottom));
	}
	float leftMem = bounds.Width () - layout[gCPUcount].mem_width;
	if (force)
		StrokeRect (BRect (leftMem - 1, top - 1, leftMem + layout[gCPUcount].mem_width, bottom + 1));
	
	for (int x = 0; x < gCPUcount; x++)
	{
		right = left + bar_width - 1;
		float rem = fCPUTimes[x] * (h + 1);
		float bar_height = floorf (rem);
		rem -= bar_height;
		float limit = bottom - bar_height;	// horizontal line
		float previous_limit = bottom - fLastBarHeight[x];
		float idle_top = top;
		if (!force && previous_limit > top)
			idle_top = previous_limit - 1;
		if (limit > idle_top)
		{
			SetHighColor (idle_color);
			FillRect (BRect (left, idle_top, right, limit - 1));
		}
		if (bar_height <= h)
		{
			rgb_color fraction_color;
			mix_colors (fraction_color, idle_color, active_color, rem);
			SetHighColor (fraction_color);
			StrokeLine (BPoint (left, bottom - bar_height), BPoint (right, bottom - bar_height));
		}
		float active_bottom = bottom;
		if (!force && previous_limit < bottom)
			active_bottom = previous_limit + 1;
		if (limit < active_bottom)
		{
			SetHighColor(active_color);
			FillRect(BRect(left, limit + 1, right, active_bottom));
		}
		left += layout[gCPUcount].cpu_width + layout[gCPUcount].cpu_inter;
		fLastBarHeight[x] = bar_height;
	}
	float rightMem = bounds.Width () - 1;
	float rem = fMemoryUsage * (h + 1);
	float bar_height = floorf (rem);
	rem -= bar_height;

	rgb_color used_memory_color;
	float sq = fMemoryUsage * fMemoryUsage;
	sq *= sq;
	sq *= sq;
	mix_colors (used_memory_color, memory_color, swap_color, sq);

	float limit = bottom - bar_height;	// horizontal line
	float previous_limit = bottom - fLastMemoryHeight;
	float free_top = top;
	if (!force && previous_limit > top)
		free_top = previous_limit - 1;
	if (limit > free_top)
	{
		SetHighColor (idle_color);
		FillRect (BRect (leftMem, free_top, rightMem, limit - 1));
	}
	if (bar_height <= h)
	{
		rgb_color fraction_color;
		mix_colors (fraction_color, idle_color, used_memory_color, rem);
		SetHighColor (fraction_color);
		StrokeLine (BPoint (leftMem, bottom - bar_height), BPoint (rightMem, bottom - bar_height));
	}
	float used_bottom = bottom;
//	if (!force && previous_limit < bottom)
//		used_bottom = previous_limit + 1;
	if (limit < used_bottom)
	{
		SetHighColor (used_memory_color);
		FillRect (BRect (leftMem, limit + 1, rightMem, used_bottom));
	}
	fLastMemoryHeight = bar_height;
}

void ProcessController::Update () {
	system_info sys_info;
	get_system_info(&sys_info);
	bigtime_t now = system_time();

	fMemoryUsage = float(sys_info.used_pages) / float(sys_info.max_pages);
	// Calculate work done since last call to Update() for each CPU
	for (int x = 0; x < gCPUcount; x++) {
		bigtime_t	load = sys_info.cpu_infos[x].active_time - fPrevActive[x];
		bigtime_t	passed = now - fPrevTime;
		float cpu_time = float(load) / float(passed);
		fPrevActive[x] = sys_info.cpu_infos[x].active_time;
		if (load > passed)
			fPrevActive[x] -= load - passed; // save overload for next period...
		if (cpu_time < 0) cpu_time = 0;
		if (cpu_time > 1) cpu_time = 1;
		fCPUTimes[x] = cpu_time;
	}
	fPrevTime = now;
}

long thread_quit_application(void *arg)
{
	BMessenger messenger (NULL, (team_id) arg);
	messenger.SendMessage (B_QUIT_REQUESTED);
	return B_OK;
}

long thread_debug_thread (void *arg)
{
	Tdebug_thead_param*	param = (Tdebug_thead_param*) arg;
	thread_info	thinfo;
	get_thread_info(param->thread, &thinfo);
	char	texte[4096];
	sprintf (texte, "db %d", int(param->thread));
	system (texte);
	if (param->sem >= 0 && thinfo.state == B_THREAD_WAITING && param->sem == thinfo.sem) {
		snooze(1000000);
		get_thread_info(param->thread, &thinfo);
		if (thinfo.state == B_THREAD_WAITING && param->sem == thinfo.sem && param->totalTime == thinfo.user_time + thinfo.kernel_time)
		{
			// the thread has been waiting for this semaphore since the before the alert, not doing anything... Let's push it out of there!
			sem_info sinfo;
			thread_info thinfo;
			infosPack	infos;
			if (get_sem_info (param->sem, &sinfo) == B_OK && get_thread_info (param->thread, &thinfo) == B_OK && get_team_info (thinfo.team, &infos.tminfo) == B_OK)
			{
				sprintf (texte, "This thread is waiting for the semaphore called \"%s\". As long as it waits for this semaphore, "
					"you won't be able to debug that thread.\n", sinfo.name);
				if (sinfo.team == thinfo.team)
					strcat (texte, "This semaphore belongs to the thread's team.\n\nShould I release this semaphore?\n");
				else
				{
					get_team_name_and_icon (infos);
					char moretexte[1024];
					sprintf (moretexte, "\nWARNING! This semaphore belongs to the team \"%s\"!\n\nShould I release this semaphore anyway?\n", infos.tmname);
					strcat (texte, moretexte);
				}
				BAlert* alert = new BAlert("", texte, "Cancel", "Release", NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
				alert->SetShortcut(0, B_ESCAPE);
				if (alert->Go())
				{
					get_thread_info (param->thread, &thinfo);
					if (thinfo.state == B_THREAD_WAITING && param->sem == thinfo.sem && param->totalTime == thinfo.user_time + thinfo.kernel_time)
						release_sem(param->sem);
					else
					{
						alert = new BAlert("", "The semaphore wasn't released, because it wasn't necessary anymore!", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
						alert->Go();
					}
				}
			}
		}
	}
	delete param;
	return B_OK;
}
