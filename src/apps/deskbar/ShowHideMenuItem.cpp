/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/


#include "ShowHideMenuItem.h"

#include <Debug.h>
#include <Roster.h>
#include <WindowInfo.h>

#include "WindowMenuItem.h"
#include "tracker_private.h"


const float kHPad = 10.0f;
const float kVPad = 2.0f;


TShowHideMenuItem::TShowHideMenuItem(const char* title, const BList* teams,
	uint32 action)
	:
	BMenuItem(title, NULL),
	fTeams(teams),
	fAction(action)
{
	BFont font(be_plain_font);
	fTitleWidth = ceilf(font.StringWidth(title));
	font_height fontHeight;
	font.GetHeight(&fontHeight);
	fTitleAscent = ceilf(fontHeight.ascent);
	fTitleDescent = ceilf(fontHeight.descent + fontHeight.leading);
}


void
TShowHideMenuItem::GetContentSize(float* width, float* height)
{
	*width = kHPad + fTitleWidth + kHPad;
	*height = fTitleAscent + fTitleDescent;
	*height += kVPad * 2;
}


void
TShowHideMenuItem::DrawContent()
{
	BRect frame(Frame());
	float labelHeight = fTitleAscent + fTitleDescent;
	BPoint drawLoc;
	drawLoc.x = kHPad;
	drawLoc.y = ((frame.Height() - labelHeight) / 2);
	Menu()->MovePenBy(drawLoc.x, drawLoc.y);
	BMenuItem::DrawContent();
}


status_t
TShowHideMenuItem::Invoke(BMessage*)
{
	bool doZoom = false;
	BRect zoomRect(0, 0, 0, 0);
	BMenuItem* item = Menu()->Superitem();

	if (item->Menu()->Window() != NULL) {
		zoomRect = item->Menu()->ConvertToScreen(item->Frame());
		doZoom = true;
	}
	return TeamShowHideCommon(static_cast<int32>(fAction), fTeams, zoomRect,
		doZoom);
}


status_t
TShowHideMenuItem::TeamShowHideCommon(int32 action, const BList* teamList,
	BRect zoomRect, bool doZoom)
{
	if (teamList == NULL)
		return B_BAD_VALUE;

	for (int32 i = teamList->CountItems() - 1; i >= 0; i--) {
		team_id team = (addr_t)teamList->ItemAt(i);

		switch (action) {
			case B_MINIMIZE_WINDOW:
				do_minimize_team(zoomRect, team, doZoom && i == 0);
				break;

			case B_BRING_TO_FRONT:
				do_bring_to_front_team(zoomRect, team, doZoom && i == 0);
				break;

			case B_QUIT_REQUESTED:
			{
				BMessenger messenger((char*)NULL, team);
				uint32 command = B_QUIT_REQUESTED;
				app_info aInfo;
				be_roster->GetRunningAppInfo(team, &aInfo);

				if (strcasecmp(aInfo.signature, kTrackerSignature) == 0)
					command = 'Tall';

				messenger.SendMessage(command);
				break;
			}
		}
	}

	return B_OK;
}
