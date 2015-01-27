/*
 * Copyright 2015 Haiku, Inc. All rights reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include "MinimizeAllInputFilter.h"

#include <string.h>

#include <new>

#include <InterfaceDefs.h>
#include <Message.h>
#include <OS.h>
#include <Roster.h>
#include <WindowInfo.h>

#include <tracker_private.h>


#define _MINIMIZE_ALL_		'_WMA'


extern "C" BInputServerFilter* instantiate_input_filter() {
	return new(std::nothrow) MinimizeAllInputFilter();
}


MinimizeAllInputFilter::MinimizeAllInputFilter()
{
}


filter_result
MinimizeAllInputFilter::Filter(BMessage* message, BList* _list)
{
	switch (message->what) {
		case B_KEY_DOWN:
		{
			const char* bytes;
			if (message->FindString("bytes", &bytes) != B_OK)
				break;

			int32 modifiers;
			if (message->FindInt32("modifiers", &modifiers) != B_OK)
				break;

			int32 modifiersHeld = modifiers & (B_COMMAND_KEY
				| B_CONTROL_KEY | B_OPTION_KEY | B_MENU_KEY | B_SHIFT_KEY);

			bool minimize;
			if (modifiersHeld == (B_COMMAND_KEY | B_CONTROL_KEY))
				minimize = true;
			else if (modifiersHeld
					== (B_COMMAND_KEY | B_CONTROL_KEY | B_SHIFT_KEY)) {
				minimize = false;
			} else
				break;

			int32 cookie = 0;
			team_info teamInfo;
			while (get_next_team_info(&cookie, &teamInfo) == B_OK) {
				app_info appInfo;
				be_roster->GetRunningAppInfo(teamInfo.team, &appInfo);
				team_id team = appInfo.team;
				be_roster->ActivateApp(team);

				if (be_roster->GetActiveAppInfo(&appInfo) == B_OK
					&& (appInfo.flags & B_BACKGROUND_APP) == 0
					&& strcasecmp(appInfo.signature, kDeskbarSignature) != 0) {
					BRect zoomRect;
					if (minimize)
						do_minimize_team(zoomRect, team, false);
					else
						do_bring_to_front_team(zoomRect, team, false);
				}
			}

			return B_SKIP_MESSAGE;
		}
	}

	return B_DISPATCH_MESSAGE;
}


status_t
MinimizeAllInputFilter::InitCheck()
{
	return B_OK;
}
