/*
 * Copyright 2002-2005 Haiku
 * Distributed under the terms of the MIT license.
 *
 * Updated by Sikosis (beos@gravity24hr.com)
 *
 * Copyright 1999, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 * Written by:	Daniel Switkin
 */


#include "PulseApp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include <Alert.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <Rect.h>
#include <TextView.h>

#include <syscalls.h>

#include "Common.h"
#include "PulseWindow.h"
#include "DeskbarPulseView.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PulseApp"


PulseApp::PulseApp(int argc, char **argv)
	: BApplication(APP_SIGNATURE),
	fPrefs(new Prefs()),
	fRunFromReplicant(false),
	fIsRunning(false),
	fPrefsWindow(NULL)
{
	int mini = false, deskbar = false, normal = false;
	uint32 framecolor = 0, activecolor = 0, idlecolor = 0;

	while (1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"deskbar", 0, &deskbar, true},
			{"width", 1, 0, 'w'},
			{"framecolor", 1, 0, 0},
			{"activecolor", 1, 0, 0},
			{"idlecolor", 1, 0, 0},
			{"mini", 0, &mini, true},
			{"normal", 0, &normal, true},
			{"help", 0, 0, 'h'},
			{0,0,0,0}
		};
		int c = getopt_long(argc, argv, "hw:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 0:
				switch (option_index) {
					case 2: /* framecolor */
					case 3: /* activecolor */
					case 4: /* idlecolor */
						uint32 rgb = strtoul(optarg, NULL, 0);
						rgb = rgb << 8;
						rgb |= 0x000000ff;

						switch (option_index) {
							case 2:
								framecolor = rgb;
								break;
							case 3:
								activecolor = rgb;
								break;
							case 4:
								idlecolor = rgb;
								break;
						}
						break;
				}
				break;
			case 'w':
				fPrefs->deskbar_icon_width = atoi(optarg);
				if (fPrefs->deskbar_icon_width < GetMinimumViewWidth())
					fPrefs->deskbar_icon_width = GetMinimumViewWidth();
				else if (fPrefs->deskbar_icon_width > 50) fPrefs->deskbar_icon_width = 50;
				break;
			case 'h':
			case '?':
				Usage();
				break;
			default:
				printf("?? getopt returned character code 0%o ??\n", c);
				break;
		}
	}

	if (deskbar) {
		fPrefs->window_mode = DESKBAR_MODE;
		if (activecolor != 0)
			fPrefs->deskbar_active_color = activecolor;
		if (idlecolor != 0)
			fPrefs->deskbar_idle_color = idlecolor;
		if (framecolor != 0)
			fPrefs->deskbar_frame_color = framecolor;
	} else if (mini) {
		fPrefs->window_mode = MINI_WINDOW_MODE;
		if (activecolor != 0)
			fPrefs->mini_active_color = activecolor;
		if (idlecolor != 0)
			fPrefs->mini_idle_color = idlecolor;
		if (framecolor != 0)
			fPrefs->mini_frame_color = framecolor;
	} else if (normal)
		fPrefs->window_mode = NORMAL_WINDOW_MODE;

	fPrefs->Save();
}


void
PulseApp::ReadyToRun()
{
	if (!fRunFromReplicant)
		BuildPulse();

	fIsRunning = true;
}


void
PulseApp::BuildPulse()
{
	PulseWindow *pulseWindow = NULL;

	if (fPrefs->window_mode == MINI_WINDOW_MODE)
		pulseWindow = new PulseWindow(fPrefs->mini_window_rect);
	else
		pulseWindow = new PulseWindow(fPrefs->normal_window_rect);

	pulseWindow->MoveOnScreen(B_MOVE_IF_PARTIALLY_OFFSCREEN);
	pulseWindow->Show();
}


PulseApp::~PulseApp()
{
	fPrefs->Save();

	delete fPrefs;
}


void
PulseApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case PV_PREFERENCES:
		{
			// This message can be posted before ReadyToRun from
			// BRoster::Launch, in that case, take note to not show the main
			// window but only the preferences
			if (!fIsRunning)
				fRunFromReplicant = true;
			BMessenger from;
			message->FindMessenger("settingsListener", &from);


			if (fPrefsWindow != NULL) {
				fPrefsWindow->Activate(true);
				break;
			}
			// If the window is already open, bring it to the front
			if (fPrefsWindow != NULL) {
				fPrefsWindow->Activate(true);
				break;
			}
			// Otherwise launch a new preferences window
			PulseApp *pulseapp = (PulseApp *)be_app;
			fPrefsWindow = new PrefsWindow(pulseapp->fPrefs->prefs_window_rect,
				B_TRANSLATE("Pulse settings"), &from,
				pulseapp->fPrefs);
			if (fRunFromReplicant) {
				fPrefsWindow->SetFlags(fPrefsWindow->Flags()
					| B_QUIT_ON_WINDOW_CLOSE);
			}
			fPrefsWindow->Show();

			break;
		}

		case PV_ABOUT:
			// This message can be posted before ReadyToRun from
			// BRoster::Launch, in that case, take note to not show the main
			// window but only the about box
			if (!fIsRunning)
				fRunFromReplicant = true;
			PostMessage(B_ABOUT_REQUESTED);
			break;

		case PRV_QUIT:
			fPrefsWindow = NULL;
			fRunFromReplicant = false;
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
PulseApp::AboutRequested()
{
	BString name = B_TRANSLATE("Pulse");

	BString message = B_TRANSLATE(
		"%s\n\nBy David Ramsey and Arve Hjønnevåg\n"
		"Revised by Daniel Switkin\n");
	message.ReplaceFirst("%s", name);
	BAlert *alert = new BAlert(B_TRANSLATE("Info"),
		message.String(), B_TRANSLATE("OK"));

	if (fRunFromReplicant)
		alert->SetFlags(alert->Flags() | B_QUIT_ON_WINDOW_CLOSE);

	BTextView* view = alert->TextView();
	BFont font;

	view->SetStylable(true);
	view->GetFont(&font);

	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, name.Length(), &font);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	// Use the asynchronous version so we don't block the window's thread
	alert->Go(NULL);
	fRunFromReplicant = false;
}

//	#pragma mark -


/** Make sure we don't disable the last CPU - this is needed by
 *	descendants of PulseView for the popup menu and for CPUButton
 *	both as a replicant and not.
 */

bool
LastEnabledCPU(unsigned int my_cpu)
{
	system_info sys_info;
	get_system_info(&sys_info);
	if (sys_info.cpu_count == 1)
		return true;

	for (unsigned int x = 0; x < sys_info.cpu_count; x++) {
		if (x == my_cpu)
			continue;
		if (_kern_cpu_enabled(x) == 1)
			return false;
	}
	return true;
}


/**	Ensure that the mini mode and deskbar mode always show an indicator
 *	for each CPU, at least one pixel wide.
 */

int
GetMinimumViewWidth()
{
	system_info sys_info;
	get_system_info(&sys_info);
	return (sys_info.cpu_count * 2) + 1;
}


void
Usage()
{
	puts(B_TRANSLATE("Usage: Pulse [--mini] [-w width] [--width=width]\n"
		"\t[--deskbar] [--normal] [--framecolor 0xrrggbb]\n"
		"\t[--activecolor 0xrrggbb] [--idlecolor 0xrrggbb]"));
	exit(0);
}


bool
LoadInDeskbar()
{
	PulseApp *pulseapp = (PulseApp *)be_app;
	BDeskbar *deskbar = new BDeskbar();
	// Don't allow two copies in the Deskbar at once
	if (deskbar->HasItem("DeskbarPulseView")) {
		delete deskbar;
		return false;
	}

	// Must be 16 pixels high, the width is retrieved from the Prefs class
	int width = pulseapp->fPrefs->deskbar_icon_width;
	int min_width = GetMinimumViewWidth();
	if (width < min_width) {
		pulseapp->fPrefs->deskbar_icon_width = min_width;
		width = min_width;
	}

	float height = deskbar->MaxItemHeight();
	BRect rect(0, 0, width - 1, height - 1);
	DeskbarPulseView *replicant = new DeskbarPulseView(rect);
	status_t err = deskbar->AddItem(replicant);
	delete replicant;
	delete deskbar;
	if (err != B_OK) {
		BString message;
		snprintf(message.LockBuffer(512), 512,
			B_TRANSLATE("Installing in Deskbar failed\n%s"), strerror(err));
		message.UnlockBuffer();
		BAlert *alert = new BAlert(B_TRANSLATE("Error"),
			message.String(), B_TRANSLATE("OK"));
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
		return false;
	}

	return true;
}


int
main(int argc, char **argv)
{
	PulseApp *pulseapp = new PulseApp(argc, argv);
	pulseapp->Run();
	delete pulseapp;
	return 0;
}
