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
#ifndef SWITCHER_H
#define SWITCHER_H


#include <Box.h>
#include <List.h>
#include <OS.h>
#include <StringView.h>
#include <Window.h>


class TTeamGroup;
class TIconView;
class TBox;
class TWindowView;
class TSwitcherWindow;
struct client_window_info;

class TSwitchManager : public BHandler {
public:
							TSwitchManager(BPoint where);
	virtual					~TSwitchManager();

	virtual void			MessageReceived(BMessage* message);

			void			Stop(bool doAction, uint32 modifiers);
			void			Unblock();
			int32			CurrentIndex();
			int32			CurrentWindow();
			int32			CurrentSlot();
			BList*			GroupList();
			int32			CountVisibleGroups();

			void			QuitApp();
			void			HideApp();
			void			CycleApp(bool forward, bool activate = false);
			void			CycleWindow(bool forward, bool wrap = true);
			void			SwitchToApp(int32 prevIndex, int32 newIndex,
								bool forward);

			client_window_info* WindowInfo(int32 groupIndex, int32 windowIndex);
			int32			CountWindows(int32 groupIndex,
								bool inCurrentWorkspace = false);
			TTeamGroup*		FindTeam(team_id, int32* index);

private:
			void			MainEntry(BMessage* message);
			void			Process(bool forward, bool byWindow = false);
			void			QuickSwitch(BMessage* message);
			void			SwitchWindow(team_id team, bool forward,
								bool activate);
			bool			ActivateApp(bool forceShow,
								bool allowWorkspaceSwitch);
			void			ActivateWindow(int32 windowID = -1);
			void			_SortApps();

			bool			_FindNextValidApp(bool forward);

			TSwitcherWindow* fWindow;
			sem_id			fMainMonitor;
			bool			fBlock;
			bigtime_t		fSkipUntil;
			bigtime_t		fLastSwitch;
			int32			fQuickSwitchIndex;
			int32			fQuickSwitchWindow;
			BList			fGroupList;
			int32			fCurrentIndex;
			int32			fCurrentWindow;
			int32			fCurrentSlot;
			int32			fWindowID;
};


#endif	/* SWITCHER_H */
