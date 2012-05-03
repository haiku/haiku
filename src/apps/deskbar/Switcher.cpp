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


#include "Switcher.h"

#include <string.h>
#include <stdlib.h>
#include <float.h>

#include <Bitmap.h>
#include <Debug.h>
#include <Font.h>
#include <Mime.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>

#include "BarApp.h"
#include "ResourceSet.h"
#include "WindowMenuItem.h"
#include "icons.h"
#include "tracker_private.h"

#define _ALLOW_STICKY_ 0
	// allows you to press 's' to keep the switcher window on screen


static const color_space kIconFormat = B_RGBA32;


class TTeamGroup {
public:
							TTeamGroup();
							TTeamGroup(BList* teams, uint32 flags, char* name,
								const char* signature);
	virtual					~TTeamGroup();

			void			Draw(BView* view, BRect bounds, bool main);

			BList*			TeamList() const
								{ return fTeams; }
			const char*		Name() const
								{ return fName; }
			const char*		Signature() const
								{ return fSignature; }
			uint32			Flags() const
								{ return fFlags; }
			const BBitmap*	SmallIcon() const
								{ return fSmallIcon; }
			const BBitmap*	LargeIcon() const
								{ return fLargeIcon; }

private:
			BList*			fTeams;
			uint32			fFlags;
			char			fSignature[B_MIME_TYPE_LENGTH];
			char*			fName;
			BBitmap*		fSmallIcon;
			BBitmap*		fLargeIcon;
};

class TSwitcherWindow : public BWindow {
public:
							TSwitcherWindow(BRect frame,
								TSwitchManager* manager);
	virtual					~TSwitcherWindow();

	virtual bool			QuitRequested();
	virtual void			MessageReceived(BMessage* message);
	virtual void			Show();
	virtual void			Hide();
	virtual void			WindowActivated(bool state);

			void			DoKey(uint32 key, uint32 modifiers);
			TIconView*		IconView();
			TWindowView*	WindowView();
			TBox*			TopView();
			bool			HairTrigger();
			void			Update(int32 previous, int32 current,
								int32 prevSlot, int32 currentSlot,
								bool forward);
			int32			SlotOf(int32);
			void			Redraw(int32 index);

private:
			TSwitchManager*	fManager;
			TIconView*		fIconView;
			TBox*			fTopView;
			TWindowView*	fWindowView;
			bool			fHairTrigger;
			bool			fSkipKeyRepeats;
};

class TWindowView : public BView {
public:
							TWindowView(BRect frame, TSwitchManager* manager,
								TSwitcherWindow* switcher);

			void			UpdateGroup(int32 groupIndex, int32 windowIndex);

	virtual void			AttachedToWindow();
	virtual void			Draw(BRect update);
	virtual void			Pulse();
	virtual void			GetPreferredSize(float* w, float* h);
			void			ScrollTo(float x, float y)
							{
								ScrollTo(BPoint(x, y));
							}
	virtual void			ScrollTo(BPoint where);

			void			ShowIndex(int32 windex);
			BRect			FrameOf(int32 index) const;

private:
			int32			fCurrentToken;
			float			fItemHeight;
			TSwitcherWindow* fSwitcher;
			TSwitchManager*	fManager;
};

class TIconView : public BView {
public:
							TIconView(BRect frame, TSwitchManager* manager,
								TSwitcherWindow* switcher);
	virtual					~TIconView();

			void			Showing();
			void			Hiding();

	virtual void			KeyDown(const char* bytes, int32 numBytes);
	virtual void			Pulse();
	virtual void			MouseDown(BPoint point);
	virtual void			Draw(BRect updateRect);

			void			ScrollTo(float x, float y)
							{
								ScrollTo(BPoint(x, y));
							}
	virtual void	ScrollTo(BPoint where);
			void			Update(int32 previous, int32 current,
								int32 previousSlot, int32 currentSlot,
								bool forward);
			void			DrawTeams(BRect update);
			int32			SlotOf(int32) const;
			BRect			FrameOf(int32) const;
			int32			ItemAtPoint(BPoint) const;
			int32			IndexAt(int32 slot) const;
			void			CenterOn(int32 index);

private:
			void			CacheIcons(TTeamGroup* group);
			void			AnimateIcon(BBitmap* startIcon, BBitmap* endIcon);

			bool			fAutoScrolling;
			TSwitcherWindow* fSwitcher;
			TSwitchManager*	fManager;
			BBitmap*		fOffBitmap;
			BView*			fOffView;
			BBitmap*		fCurrentSmall;
			BBitmap*		fCurrentLarge;
};

class TBox : public BBox {
public:
							TBox(BRect bounds, TSwitchManager* manager,
								TSwitcherWindow* window, TIconView* iconView);

	virtual void			Draw(BRect update);
	virtual void			AllAttached();
	virtual void			DrawIconScrollers(bool force);
	virtual void			DrawWindowScrollers(bool force);
	virtual void			MouseDown(BPoint where);

private:
			TSwitchManager*	fManager;
			TSwitcherWindow* fWindow;
			TIconView*		fIconView;
			BRect			fCenter;
			bool			fLeftScroller;
			bool			fRightScroller;
			bool			fUpScroller;
			bool			fDownScroller;
};


const int32 kHorizontalMargin = 11;
const int32 kVerticalMargin = 10;

// SLOT_SIZE must be divisible by 4. That's because of the scrolling
// animation. If this needs to change then look at TIconView::Update()

const int32 kSlotSize = 36;
const int32 kScrollStep = kSlotSize / 2;
const int32 kNumSlots = 7;
const int32 kCenterSlot = 3;

const int32 kWindowScrollSteps = 3;


//	#pragma mark -


static int32
LowBitIndex(uint32 value)
{
	int32 result = 0;
	int32 bitMask = 1;

	if (value == 0)
		return -1;

	while (result < 32 && (value & bitMask) == 0) {
		result++;
		bitMask = bitMask << 1;
	}
	return result;
}


inline bool
IsVisibleInCurrentWorkspace(const window_info* windowInfo)
{
	// The window list is always ordered from the top front visible window
	// (the first on the list), going down through all the other visible
	// windows, then all hidden or non-workspace visible windows at the end.
	//     layer > 2  : normal visible window
	//     layer == 2 : reserved for the desktop window (visible also)
	//     layer < 2  : hidden (0) and non workspace visible window (1)
	return windowInfo->layer > 2;
}


bool
IsKeyDown(int32 key)
{
	key_info keyInfo;

	get_key_info(&keyInfo);
	return (keyInfo.key_states[key >> 3] & (1 << ((7 - key) & 7))) != 0;
}


bool
IsWindowOK(const window_info* windowInfo)
{
	// is_mini (true means that the window is minimized).
	// if not, then show_hide >= 1 means that the window is hidden.
	// If the window is both minimized and hidden, then you get :
	//     TWindow->is_mini = false;
	//     TWindow->was_mini = true;
	//     TWindow->show_hide >= 1;

	if (windowInfo->feel != _STD_W_TYPE_)
		return false;

	if (windowInfo->is_mini)
		return true;

	return windowInfo->show_hide_level <= 0;
}


bool
OKToUse(const TTeamGroup* teamGroup)
{
	if (!teamGroup)
		return false;

	// skip background applications
	if ((teamGroup->Flags() & B_BACKGROUND_APP) != 0)
		return false;

	// skip the Deskbar itself
	if (strcasecmp(teamGroup->Signature(), kDeskbarSignature) == 0)
		return false;

	return true;
}


int
SmartStrcmp(const char* s1, const char* s2)
{
	if (strcasecmp(s1, s2) == 0)
		return 0;

	// if the strings on differ in spaces or underscores they still match
	while (*s1 && *s2) {
		if ((*s1 == ' ') || (*s1 == '_')) {
			s1++;
			continue;
		}
		if ((*s2 == ' ') || (*s2 == '_')) {
			s2++;
			continue;
		}
		if (*s1 != *s2) {
			// they differ
			return 1;
		}
		s1++;
		s2++;
	}

	// if one of the strings ended before the other
	// TODO: could process trailing spaces and underscores
	if (*s1)
		return 1;
	if (*s2)
		return 1;

	return 0;
}


//	#pragma mark -


TTeamGroup::TTeamGroup()
	:
	fTeams(NULL),
	fFlags(0),
	fName(NULL),
	fSmallIcon(NULL),
	fLargeIcon(NULL)
{
	fSignature[0] = '\0';
}


TTeamGroup::TTeamGroup(BList* teams, uint32 flags, char* name,
		const char* signature)
	:
	fTeams(teams),
	fFlags(flags),
	fName(name),
	fSmallIcon(NULL),
	fLargeIcon(NULL)
{
	strlcpy(fSignature, signature, sizeof(fSignature));

	fSmallIcon = new BBitmap(BRect(0, 0, 15, 15), kIconFormat);
	fLargeIcon = new BBitmap(BRect(0, 0, 31, 31), kIconFormat);

	app_info appInfo;
	if (be_roster->GetAppInfo(signature, &appInfo) == B_OK) {
		BNode node(&(appInfo.ref));
		if (node.InitCheck() == B_OK) {
			BNodeInfo nodeInfo(&node);
			if (nodeInfo.InitCheck() == B_OK) {
				nodeInfo.GetTrackerIcon(fSmallIcon, B_MINI_ICON);
				nodeInfo.GetTrackerIcon(fLargeIcon, B_LARGE_ICON);
			}
		}
	}
}


TTeamGroup::~TTeamGroup()
{
	delete fTeams;
	free(fName);
	delete fSmallIcon;
	delete fLargeIcon;
}


void
TTeamGroup::Draw(BView* view, BRect bounds, bool main)
{
	BRect rect;
	if (main) {
		rect = fLargeIcon->Bounds();
		rect.OffsetTo(bounds.LeftTop());
		rect.OffsetBy(2, 2);
		view->DrawBitmap(fLargeIcon, rect);
	} else {
		rect = fSmallIcon->Bounds();
		rect.OffsetTo(bounds.LeftTop());
		rect.OffsetBy(10, 10);
		view->DrawBitmap(fSmallIcon, rect);
	}
}


//	#pragma mark -


TSwitchManager::TSwitchManager(BPoint point)
	: BHandler("SwitchManager"),
	fMainMonitor(create_sem(1, "main_monitor")),
	fBlock(false),
	fSkipUntil(0),
	fLastSwitch(0),
	fQuickSwitchIndex(-1),
	fQuickSwitchWindow(-1),
	fGroupList(10),
	fCurrentIndex(0),
	fCurrentSlot(0),
	fWindowID(-1)
{
	BRect rect(point.x, point.y,
		point.x + (kSlotSize * kNumSlots) - 1 + (2 * kHorizontalMargin),
		point.y + 82);
	fWindow = new TSwitcherWindow(rect, this);
	fWindow->AddHandler(this);

	fWindow->Lock();
	fWindow->Run();

	BList tmpList;
	TBarApp::Subscribe(BMessenger(this), &tmpList);

	for (int32 i = 0; ; i++) {
		BarTeamInfo* barTeamInfo = (BarTeamInfo*)tmpList.ItemAt(i);
		if (!barTeamInfo)
			break;

		TTeamGroup* tinfo = new TTeamGroup(barTeamInfo->teams,
			barTeamInfo->flags, barTeamInfo->name, barTeamInfo->sig);
		fGroupList.AddItem(tinfo);

		barTeamInfo->teams = NULL;
		barTeamInfo->name = NULL;

		delete barTeamInfo;
	}
	fWindow->Unlock();
}


TSwitchManager::~TSwitchManager()
{
	for (int32 i = fGroupList.CountItems(); i-- > 0;) {
		TTeamGroup* teamInfo = static_cast<TTeamGroup*>(fGroupList.ItemAt(i));
		delete teamInfo;
	}
}


void
TSwitchManager::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SOME_APP_QUIT:
		{
			// This is only sent when last team of a matching set quits
			team_id teamID;
			int i = 0;
			TTeamGroup* tinfo;
			message->FindInt32("team", &teamID);

			while ((tinfo = (TTeamGroup*)fGroupList.ItemAt(i)) != NULL) {
				if (tinfo->TeamList()->HasItem((void*)teamID)) {
					fGroupList.RemoveItem(i);

					if (OKToUse(tinfo)) {
						fWindow->Redraw(i);
						if (i <= fCurrentIndex) {
							fCurrentIndex--;
							CycleApp(true);
						}
					}
					delete tinfo;
					break;
				}
				i++;
			}
			break;
		}

		case B_SOME_APP_LAUNCHED:
		{
			BList* teams;
			const char* name;
			BBitmap* smallIcon;
			uint32 flags;
			const char* signature;

			if (message->FindPointer("teams", (void**)&teams) != B_OK)
				break;

			if (message->FindPointer("icon", (void**)&smallIcon) != B_OK) {
				delete teams;
				break;
			}

			delete smallIcon;

			if (message->FindString("sig", &signature) != B_OK) {
				delete teams;
				break;
			}

			if (message->FindInt32("flags", (int32*)&flags) != B_OK) {
				delete teams;
				break;
			}

			if (message->FindString("name", &name) != B_OK) {
				delete teams;
				break;
			}

			TTeamGroup* tinfo = new TTeamGroup(teams, flags, strdup(name),
				signature);

			fGroupList.AddItem(tinfo);
			if (OKToUse(tinfo))
				fWindow->Redraw(fGroupList.CountItems() - 1);

			break;
		}

		case kAddTeam:
		{
			const char* signature = message->FindString("sig");
			team_id team = message->FindInt32("team");

			for (int32 i = 0; i < fGroupList.CountItems(); i++) {
				TTeamGroup* tinfo = (TTeamGroup*)fGroupList.ItemAt(i);
				if (strcasecmp(tinfo->Signature(), signature) == 0) {
					if (!(tinfo->TeamList()->HasItem((void*)team)))
						tinfo->TeamList()->AddItem((void*)team);
					break;
				}
			}
			break;
		}

		case kRemoveTeam:
		{
			team_id team = message->FindInt32("team");

			for (int32 i = 0; i < fGroupList.CountItems(); i++) {
				TTeamGroup* tinfo = (TTeamGroup*)fGroupList.ItemAt(i);
				if (tinfo->TeamList()->HasItem((void*)team)) {
					tinfo->TeamList()->RemoveItem((void*)team);
					break;
				}
			}
			break;
		}

		case 'TASK':
		{
			// The first TASK message calls MainEntry. Subsequent ones
			// call Process().
			bigtime_t time;
			message->FindInt64("when", (int64*)&time);

			// The fSkipUntil stuff can be removed once the new input_server
			// starts differentiating initial key_downs from KeyDowns generated
			// by auto-repeat. Until then the fSkipUntil stuff helps, but it
			// isn't perfect.
			if (time < fSkipUntil)
				break;

			status_t status = acquire_sem_etc(fMainMonitor, 1, B_TIMEOUT, 0);
			if (status != B_OK) {
				if (!fWindow->IsHidden() && !fBlock) {
					// Want to skip TASK msgs posted before the window
					// was made visible. Better UI feel if we do this.
					if (time > fSkipUntil) {
						uint32 modifiers = 0;
						message->FindInt32("modifiers", (int32*)&modifiers);
						int32 key = 0;
						message->FindInt32("key", &key);

						Process((modifiers & B_SHIFT_KEY) == 0, key == 0x11);
					}
				}
			} else
				MainEntry(message);

			break;
		}

		default:
			break;
	}
}


void
TSwitchManager::_SortApps()
{
	team_id* teams;
	int32 count;
	if (BPrivate::get_application_order(current_workspace(), &teams, &count)
		!= B_OK)
		return;

	BList groups;
	if (!groups.AddList(&fGroupList)) {
		free(teams);
		return;
	}

	fGroupList.MakeEmpty();

	for (int32 i = 0; i < count; i++) {
		// find team
		TTeamGroup* info = NULL;
		for (int32 j = 0; (info = (TTeamGroup*)groups.ItemAt(j)) != NULL; j++) {
			if (info->TeamList()->HasItem((void*)teams[i])) {
				groups.RemoveItem(j);
				break;
			}
		}

		if (info != NULL)
			fGroupList.AddItem(info);
	}

	fGroupList.AddList(&groups);
		// add the remaining entries
	free(teams);
}


void
TSwitchManager::MainEntry(BMessage* message)
{
	bigtime_t now = system_time();
	bigtime_t timeout = now + 180000;
		// The above delay has a good "feel" found by trial and error

	app_info appInfo;
	be_roster->GetActiveAppInfo(&appInfo);

	bool resetQuickSwitch = false;

	if (now > fLastSwitch + 400000) {
		_SortApps();
		resetQuickSwitch = true;
	}

	fLastSwitch = now;

	int32 index;
	fCurrentIndex = FindTeam(appInfo.team, &index) != NULL ? index : 0;

	if (resetQuickSwitch) {
		fQuickSwitchIndex = fCurrentIndex;
		fQuickSwitchWindow = fCurrentWindow;
	}

	int32 key;
	message->FindInt32("key", (int32*)&key);

	uint32 modifierKeys = 0;
	while (system_time() < timeout) {
		modifierKeys = modifiers();
		if (!IsKeyDown(key)) {
			QuickSwitch(message);
			return;
		}
		if ((modifierKeys & B_CONTROL_KEY) == 0) {
			QuickSwitch(message);
			return;
		}
		snooze(20000);
			// Must be a multiple of the delay used above
	}

	Process((modifierKeys & B_SHIFT_KEY) == 0, key == 0x11);
}


void
TSwitchManager::Stop(bool do_action, uint32)
{
	fWindow->Hide();
	if (do_action)
		ActivateApp(true, true);

	release_sem(fMainMonitor);
}


TTeamGroup*
TSwitchManager::FindTeam(team_id teamID, int32* index)
{
	int i = 0;
	TTeamGroup* info;
	while ((info = (TTeamGroup*)fGroupList.ItemAt(i)) != NULL) {
		if (info->TeamList()->HasItem((void*)teamID)) {
			*index = i;
			return info;
		}
		i++;
	}

	return NULL;
}


void
TSwitchManager::Process(bool forward, bool byWindow)
{
	bool hidden = false;
	if (fWindow->Lock()) {
		hidden = fWindow->IsHidden();
		fWindow->Unlock();
	}
	if (byWindow) {
		// If hidden we need to get things started by switching to correct app
		if (hidden)
			SwitchToApp(fCurrentIndex, fCurrentIndex, forward);
		CycleWindow(forward, true);
	} else
		CycleApp(forward, false);

	if (hidden) {
		// more auto keyrepeat code
		// Because of key repeats we don't want to respond to any extraneous
		// 'TASK' messages until the window is completely shown. So block here.
		// the WindowActivated hook function will unblock.
		fBlock = true;

		if (fWindow->Lock()) {
			BRect screenFrame = BScreen().Frame();
			BRect windowFrame = fWindow->Frame();

			if (!screenFrame.Contains(windowFrame)) {
				// center the window
				BPoint point((screenFrame.left + screenFrame.right) / 2,
					(screenFrame.top + screenFrame.bottom) / 2);

				point.x -= (windowFrame.Width() / 2);
				point.y -= (windowFrame.Height() / 2);
				fWindow->MoveTo(point);
			}

			fWindow->Show();
			fWindow->Unlock();
		}
	}
}


void
TSwitchManager::QuickSwitch(BMessage* message)
{
	uint32 modifiers = 0;
	message->FindInt32("modifiers", (int32*)&modifiers);
	int32 key = 0;
	message->FindInt32("key", &key);

	team_id team;
	if (message->FindInt32("team", &team) == B_OK) {
		bool forward = (modifiers & B_SHIFT_KEY) == 0;

		if (key == 0x11) {
			// TODO: add the same switch logic we have for apps!
			SwitchWindow(team, forward, true);
		} else {
			if (fQuickSwitchIndex >= 0) {
				// Switch to the first app inbetween to make it always the next
				// app to switch to after the quick switch.
				int32 current = fCurrentIndex;
				SwitchToApp(current, fQuickSwitchIndex, false);
				ActivateApp(false, false);

				fCurrentIndex = current;
			}

			CycleApp(forward, true);
		}
	}

	release_sem(fMainMonitor);
}


int32
TSwitchManager::CountVisibleGroups()
{
	int32 result = 0;

	int32 count = fGroupList.CountItems();
	for (int32 i = 0; i < count; i++) {
		if (!OKToUse((TTeamGroup*)fGroupList.ItemAt(i)))
			continue;

		result++;
	}
	return result;
}


void
TSwitchManager::CycleWindow(bool forward, bool wrap)
{
	int32 max = CountWindows(fCurrentIndex);
	int32 prev = fCurrentWindow;
	int32 next = fCurrentWindow;

	if (forward) {
		next++;
		if (next >= max) {
			if (!wrap)
				return;
			next = 0;
		}
	} else {
		next--;
		if (next < 0) {
			if (!wrap)
				return;
			next = max - 1;
		}
	}
	fCurrentWindow = next;

	if (fCurrentWindow != prev)
		fWindow->WindowView()->ShowIndex(fCurrentWindow);
}


void
TSwitchManager::CycleApp(bool forward, bool activateNow)
{
	int32 startIndex = fCurrentIndex;

	if (_FindNextValidApp(forward)) {
		// if we're here then we found a good one
		SwitchToApp(startIndex, fCurrentIndex, forward);

		if (!activateNow)
			return;

		ActivateApp(false, false);
	}
}


bool
TSwitchManager::_FindNextValidApp(bool forward)
{
	int32 startIndex = fCurrentIndex;
	int32 max = fGroupList.CountItems();

	for (;;) {
		if (forward) {
			fCurrentIndex++;
			if (fCurrentIndex >= max)
				fCurrentIndex = 0;
		} else {
			fCurrentIndex--;
			if (fCurrentIndex < 0)
				fCurrentIndex = max - 1;
		}

		if (fCurrentIndex == startIndex) {
			// we've gone completely through the list without finding
			// a good app. Oh well.
			break;
		}

		if (OKToUse((TTeamGroup*)fGroupList.ItemAt(fCurrentIndex)))
			return true;
	}

	return false;
}


void
TSwitchManager::SwitchToApp(int32 previousIndex, int32 newIndex, bool forward)
{
	int32 previousSlot = fCurrentSlot;

	fCurrentIndex = newIndex;
	if (!OKToUse((TTeamGroup *)fGroupList.ItemAt(fCurrentIndex)))
		_FindNextValidApp(forward);

	fCurrentSlot = fWindow->SlotOf(fCurrentIndex);
	fCurrentWindow = 0;

	fWindow->Update(previousIndex, fCurrentIndex, previousSlot, fCurrentSlot,
		forward);
}


bool
TSwitchManager::ActivateApp(bool forceShow, bool allowWorkspaceSwitch)
{
	// Let's get the info about the selected window. If it doesn't exist
	// anymore then get info about first window. If that doesn't exist then
	// do nothing.
	client_window_info* windowInfo = WindowInfo(fCurrentIndex, fCurrentWindow);
	if (windowInfo == NULL) {
		windowInfo = WindowInfo(fCurrentIndex, 0);
		if (windowInfo == NULL)
			return false;
	}

	int32 currentWorkspace = current_workspace();
	TTeamGroup* teamGroup = (TTeamGroup*)fGroupList.ItemAt(fCurrentIndex);

	// Let's handle the easy case first: There's only 1 team in the group
	if (teamGroup->TeamList()->CountItems() == 1) {
		bool result;
		if (forceShow && (fCurrentWindow != 0 || windowInfo->is_mini)) {
			do_window_action(windowInfo->server_token, B_BRING_TO_FRONT,
				BRect(0, 0, 0, 0), false);
		}

		if (!forceShow && windowInfo->is_mini) {
			// we aren't unhiding minimized windows, so we can't do
			// anything here
			result = false;
		} else if (!allowWorkspaceSwitch
			&& (windowInfo->workspaces & (1 << currentWorkspace)) == 0) {
			// we're not supposed to switch workspaces so abort.
			result = false;
		} else {
			result = true;
			be_roster->ActivateApp((team_id)teamGroup->TeamList()->ItemAt(0));
		}

		ASSERT(windowInfo);
		free(windowInfo);
		return result;
	}

	// Now the trickier case. We're trying to Bring to the Front a group
	// of teams. The current window (defined by fCurrentWindow) will define
	// which workspace we're going to. Then, once that is determined we
	// want to bring to the front every window of the group of teams that
	// lives in that workspace.

	if ((windowInfo->workspaces & (1 << currentWorkspace)) == 0) {
		if (!allowWorkspaceSwitch) {
			// If the first window in the list isn't in current workspace,
			// then none are. So we can't switch to this app.
			ASSERT(windowInfo);
			free(windowInfo);
			return false;
		}
		int32 destWorkspace = LowBitIndex(windowInfo->workspaces);
		// now switch to that workspace
		activate_workspace(destWorkspace);
	}

	if (!forceShow && windowInfo->is_mini) {
		// If the first window in the list is hidden then no windows in
		// this group are visible. So we can't switch to this app.
		ASSERT(windowInfo);
		free(windowInfo);
		return false;
	}

	int32 tokenCount;
	int32* tokens = get_token_list(-1, &tokenCount);
	if (tokens == NULL) {
		ASSERT(windowInfo);
		free(windowInfo);
		return true;
			// weird error, so don't try to recover
	}

	BList windowsToActivate;

	// Now we go through all the windows in the current workspace list in order.
	// As we hit member teams we build the "activate" list.
	for (int32 i = 0; i < tokenCount; i++) {
		client_window_info* matchWindowInfo = get_window_info(tokens[i]);
		if (!matchWindowInfo) {
			// That window probably closed. Just go to the next one.
			continue;
		}
		if (!IsVisibleInCurrentWorkspace(matchWindowInfo)) {
			// first non-visible in workspace window means we're done.
			free(matchWindowInfo);
			break;
		}
		if (matchWindowInfo->server_token != windowInfo->server_token
			&& teamGroup->TeamList()->HasItem((void*)matchWindowInfo->team))
			windowsToActivate.AddItem((void*)matchWindowInfo->server_token);

		free(matchWindowInfo);
	}

	free(tokens);

	// Want to go through the list backwards to keep windows in same relative
	// order.
	int32 i = windowsToActivate.CountItems() - 1;
	for (; i >= 0; i--) {
		int32 wid = (int32) windowsToActivate.ItemAt(i);
		do_window_action(wid, B_BRING_TO_FRONT, BRect(0, 0, 0, 0), false);
	}

	// now bring the select window on top of everything.

	do_window_action(windowInfo->server_token, B_BRING_TO_FRONT,
		BRect(0, 0, 0, 0), false);

	free(windowInfo);
	return true;
}


void
TSwitchManager::QuitApp()
{
	// check if we're in the last slot already (the last usable team group)

	TTeamGroup* teamGroup;
	int32 count = 0;

	for (int32 i = fCurrentIndex + 1; i < fGroupList.CountItems(); i++) {
		teamGroup = (TTeamGroup*)fGroupList.ItemAt(i);

		if (!OKToUse(teamGroup))
			continue;

		count++;
	}

	teamGroup = (TTeamGroup*)fGroupList.ItemAt(fCurrentIndex);

	if (count == 0) {
		// switch to previous app in the list so that we don't jump to
		// the start of the list (try to keep the same position when
		// the apps at the current index go away)
		CycleApp(false, false);
	}

	// send the quit request to all teams in this group

	for (int32 i = teamGroup->TeamList()->CountItems(); i-- > 0;) {
		team_id team = (team_id)teamGroup->TeamList()->ItemAt(i);
		app_info info;
		if (be_roster->GetRunningAppInfo(team, &info) == B_OK) {
			if (!strcasecmp(info.signature, kTrackerSignature)) {
				// Tracker can't be quit this way
				continue;
			}

			BMessenger messenger(NULL, team);
			messenger.SendMessage(B_QUIT_REQUESTED);
		}
	}
}


void
TSwitchManager::HideApp()
{
	// hide all teams in this group

	TTeamGroup* teamGroup = (TTeamGroup*)fGroupList.ItemAt(fCurrentIndex);

	for (int32 i = teamGroup->TeamList()->CountItems(); i-- > 0;) {
		team_id team = (team_id)teamGroup->TeamList()->ItemAt(i);
		app_info info;
		if (be_roster->GetRunningAppInfo(team, &info) == B_OK)
			do_minimize_team(BRect(), team, false);
	}
}


client_window_info*
TSwitchManager::WindowInfo(int32 groupIndex, int32 windowIndex)
{
	TTeamGroup* teamGroup = (TTeamGroup*)fGroupList.ItemAt(groupIndex);
	if (teamGroup == NULL)
		return NULL;

	int32 tokenCount;
	int32* tokens = get_token_list(-1, &tokenCount);
	if (tokens == NULL)
		return NULL;

	int32 matches = 0;

	// Want to find the "windowIndex'th" window in window order that belongs
	// the the specified group (groupIndex). Since multiple teams can belong to
	// the same group (multiple-launch apps) we get the list of _every_
	// window and go from there.

	client_window_info* result = NULL;
	for (int32 i = 0; i < tokenCount; i++) {
		client_window_info* windowInfo = get_window_info(tokens[i]);
		if (windowInfo) {
			// skip hidden/special windows
			if (IsWindowOK(windowInfo)
				&& (teamGroup->TeamList()->HasItem((void*)windowInfo->team))) {
				// this window belongs to the team!
				if (matches == windowIndex) {
					// we found it!
					result = windowInfo;
					break;
				}
				matches++;
			}
			free(windowInfo);
		}
		// else - that window probably closed. Just go to the next one.
	}

	free(tokens);

	return result;
}


int32
TSwitchManager::CountWindows(int32 groupIndex, bool )
{
	TTeamGroup* teamGroup = (TTeamGroup*)fGroupList.ItemAt(groupIndex);
	if (!teamGroup)
		return 0;

	int32 result = 0;

	for (int32 i = 0; ; i++) {
		team_id	teamID = (team_id)teamGroup->TeamList()->ItemAt(i);
		if (teamID == 0)
			break;

		int32 count;
		int32* tokens = get_token_list(teamID, &count);
		if (!tokens)
			continue;

		for (int32 i = 0; i < count; i++) {
			window_info	*windowInfo = get_window_info(tokens[i]);
			if (windowInfo) {
				if (IsWindowOK(windowInfo))
					result++;
				free(windowInfo);
			}
		}
		free(tokens);
	}

	return result;
}


void
TSwitchManager::ActivateWindow(int32 windowID)
{
	if (windowID == -1)
		windowID = fWindowID;

	do_window_action(windowID, B_BRING_TO_FRONT, BRect(0, 0, 0, 0), false);
}


void
TSwitchManager::SwitchWindow(team_id team, bool, bool activate)
{
	// Find the _last_ window in the current workspace that belongs
	// to the group. This is the window to activate.

	int32 index;
	TTeamGroup* teamGroup = FindTeam(team, &index);

	// cycle through the windows in the active application
	int32 count;
	int32* tokens = get_token_list(-1, &count);
	if (tokens == NULL)
		return;

	for (int32 i = count - 1; i >= 0; i--) {
		client_window_info* windowInfo = get_window_info(tokens[i]);
		if (windowInfo && IsVisibleInCurrentWorkspace(windowInfo)
			&& teamGroup->TeamList()->HasItem((void*)windowInfo->team)) {
			fWindowID = windowInfo->server_token;
			if (activate)
				ActivateWindow(windowInfo->server_token);

			free(windowInfo);
			break;
		}
		free(windowInfo);
	}
	free(tokens);
}


void
TSwitchManager::Unblock()
{
	fBlock = false;
	fSkipUntil = system_time();
}


int32
TSwitchManager::CurrentIndex()
{
	return fCurrentIndex;
}


int32
TSwitchManager::CurrentWindow()
{
	return fCurrentWindow;
}


int32
TSwitchManager::CurrentSlot()
{
	return fCurrentSlot;
}


BList*
TSwitchManager::GroupList()
{
	return &fGroupList;
}


//	#pragma mark -


TBox::TBox(BRect bounds, TSwitchManager* manager, TSwitcherWindow* window,
		TIconView* iconView)
	:
	BBox(bounds, "top", B_FOLLOW_NONE, B_WILL_DRAW, B_NO_BORDER),
	fManager(manager),
	fWindow(window),
	fIconView(iconView),
	fLeftScroller(false),
	fRightScroller(false),
	fUpScroller(false),
	fDownScroller(false)
{
}


void
TBox::AllAttached()
{
	BRect centerRect(kCenterSlot * kSlotSize, 0,
		(kCenterSlot + 1) * kSlotSize - 1, kSlotSize - 1);
	BRect frame = fIconView->Frame();

	// scroll the centerRect to correct location
	centerRect.OffsetBy(frame.left, frame.top);

	// switch to local coords
	fIconView->ConvertToParent(&centerRect);

	fCenter = centerRect;
}


void
TBox::MouseDown(BPoint where)
{
	if (!fLeftScroller && !fRightScroller && !fUpScroller && !fDownScroller)
		return;

	BRect frame = fIconView->Frame();
	BRect bounds = Bounds();

	if (fLeftScroller) {
		BRect lhit(0, frame.top, frame.left, frame.bottom);
		if (lhit.Contains(where)) {
			// Want to scroll by NUMSLOTS - 1 slots
			int32 previousIndex = fManager->CurrentIndex();
			int32 previousSlot = fManager->CurrentSlot();
			int32 newSlot = previousSlot - (kNumSlots - 1);
			if (newSlot < 0)
				newSlot = 0;
			int32 newIndex = fIconView->IndexAt(newSlot);

			fManager->SwitchToApp(previousIndex, newIndex, false);
		}
	}

	if (fRightScroller) {
		BRect rhit(frame.right, frame.top, bounds.right, frame.bottom);
		if (rhit.Contains(where)) {
			// Want to scroll by NUMSLOTS - 1 slots
			int32 previousIndex = fManager->CurrentIndex();
			int32 previousSlot = fManager->CurrentSlot();
			int32 newSlot = previousSlot + (kNumSlots - 1);
			int32 newIndex = fIconView->IndexAt(newSlot);

			if (newIndex < 0) {
				// don't have a page full to scroll
				int32 valid = fManager->CountVisibleGroups();
				newIndex = fIconView->IndexAt(valid - 1);
			}
			fManager->SwitchToApp(previousIndex, newIndex, true);
		}
	}

	frame = fWindow->WindowView()->Frame();
	if (fUpScroller) {
		BRect hit1(frame.left - 10, frame.top, frame.left,
			(frame.top + frame.bottom) / 2);
		BRect hit2(frame.right, frame.top, frame.right + 10,
			(frame.top + frame.bottom) / 2);
		if (hit1.Contains(where) || hit2.Contains(where)) {
			// Want to scroll up 1 window
			fManager->CycleWindow(false, false);
		}
	}

	if (fDownScroller) {
		BRect hit1(frame.left - 10, (frame.top + frame.bottom) / 2,
			frame.left, frame.bottom);
		BRect hit2(frame.right, (frame.top + frame.bottom) / 2,
			frame.right + 10, frame.bottom);
		if (hit1.Contains(where) || hit2.Contains(where)) {
			// Want to scroll down 1 window
			fManager->CycleWindow(true, false);
		}
	}
}


void
TBox::Draw(BRect update)
{
	static const int32 kChildInset = 7;
	static const int32 kWedge = 6;

	BBox::Draw(update);

	// The fancy border around the icon view

	BRect bounds = Bounds();
	float height = fIconView->Bounds().Height();
	float center = (bounds.right + bounds.left) / 2;

	BRect box(3, 3, bounds.right - 3, 3 + height + kChildInset * 2);
	rgb_color white = {255, 255, 255, 255};
	rgb_color standardGray = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color veryDarkGray = {128, 128, 128, 255};
	rgb_color darkGray = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_1_TINT);

	// Fill the area with dark gray
	SetHighColor(darkGray);
	box.InsetBy(1, 1);
	FillRect(box);

	box.InsetBy(-1, -1);

	BeginLineArray(50);

	// The main frame around the icon view
	AddLine(box.LeftTop(), BPoint(center - kWedge, box.top), veryDarkGray);
	AddLine(BPoint(center + kWedge, box.top), box.RightTop(), veryDarkGray);

	AddLine(box.LeftBottom(), BPoint(center - kWedge, box.bottom),
		veryDarkGray);
	AddLine(BPoint(center + kWedge, box.bottom), box.RightBottom(),
		veryDarkGray);
	AddLine(box.LeftBottom() + BPoint(1, 1),
		BPoint(center - kWedge, box.bottom + 1), white);
	AddLine(BPoint(center + kWedge, box.bottom) + BPoint(0, 1),
		box.RightBottom() + BPoint(1, 1), white);

	AddLine(box.LeftTop(), box.LeftBottom(), veryDarkGray);
	AddLine(box.RightTop(), box.RightBottom(), veryDarkGray);
	AddLine(box.RightTop() + BPoint(1, 1), box.RightBottom() + BPoint(1, 1),
		white);

	// downward pointing area at top of frame
	BPoint point(center - kWedge, box.top);
	AddLine(point, point + BPoint(kWedge, kWedge), veryDarkGray);
	AddLine(point + BPoint(kWedge, kWedge), BPoint(center + kWedge, point.y),
		veryDarkGray);

	AddLine(point + BPoint(1, 0), point + BPoint(1, 0)
		+ BPoint(kWedge - 1, kWedge - 1), white);

	AddLine(point + BPoint(2, -1) + BPoint(kWedge - 1, kWedge - 1),
		BPoint(center + kWedge - 1, point.y), darkGray);

	BPoint topPoint = point;

	// upward pointing area at bottom of frame
	point.y = box.bottom;
	point.x = center - kWedge;
	AddLine(point, point + BPoint(kWedge, -kWedge), veryDarkGray);
	AddLine(point + BPoint(kWedge, -kWedge),
		BPoint(center + kWedge, point.y), veryDarkGray);

	AddLine(point + BPoint(1, 0),
		point + BPoint(1, 0) + BPoint(kWedge - 1, -(kWedge - 1)), white);

	AddLine(point + BPoint(2 , 1) + BPoint(kWedge - 1, -(kWedge - 1)),
		BPoint(center + kWedge - 1, point.y), darkGray);

	BPoint bottomPoint = point;

	EndLineArray();

	// fill the downward pointing arrow area
	SetHighColor(standardGray);
	FillTriangle(topPoint + BPoint(2, 0),
		topPoint + BPoint(2, 0) + BPoint(kWedge - 2, kWedge - 2),
		BPoint(center + kWedge - 2, topPoint.y));

	// fill the upward pointing arrow area
	SetHighColor(standardGray);
	FillTriangle(bottomPoint + BPoint(2, 0),
		bottomPoint + BPoint(2, 0) + BPoint(kWedge - 2, -(kWedge - 2)),
		BPoint(center + kWedge - 2, bottomPoint.y));

	DrawIconScrollers(false);
	DrawWindowScrollers(false);

}


void
TBox::DrawIconScrollers(bool force)
{
	rgb_color backgroundColor = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_1_TINT);
	rgb_color dark = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_4_TINT);
	bool updateLeft = false;
	bool updateRight = false;

	BRect rect = fIconView->Bounds();
	if (rect.left > (kSlotSize * kCenterSlot)) {
		updateLeft = true;
		fLeftScroller = true;
	} else {
		fLeftScroller = false;
		if (force)
			updateLeft = true;
	}

	int32 maxIndex = fManager->GroupList()->CountItems() - 1;
	// last_frame is in fIconView coordinate space
	BRect lastFrame = fIconView->FrameOf(maxIndex);

	if (lastFrame.right > rect.right) {
		updateRight = true;
		fRightScroller = true;
	} else {
		fRightScroller = false;
		if (force)
			updateRight = true;
	}

	PushState();
	SetDrawingMode(B_OP_COPY);

	rect = fIconView->Frame();
	if (updateLeft) {
		BPoint pt1, pt2, pt3;
		pt1.x = rect.left - 5;
		pt1.y = floorf((rect.bottom + rect.top) / 2);
		pt2.x = pt3.x = pt1.x + 3;
		pt2.y = pt1.y - 3;
		pt3.y = pt1.y + 3;

		if (fLeftScroller) {
			SetHighColor(dark);
			FillTriangle(pt1, pt2, pt3);
		} else if (force) {
			SetHighColor(backgroundColor);
			FillRect(BRect(pt1.x, pt2.y, pt3.x, pt3.y));
		}
	}
	if (updateRight) {
		BPoint pt1, pt2, pt3;
		pt1.x = rect.right + 4;
		pt1.y = rintf((rect.bottom + rect.top) / 2);
		pt2.x = pt3.x = pt1.x - 4;
		pt2.y = pt1.y - 4;
		pt3.y = pt1.y + 4;

		if (fRightScroller) {
			SetHighColor(dark);
			FillTriangle(pt1, pt2, pt3);
		} else if (force) {
			SetHighColor(backgroundColor);
			FillRect(BRect(pt3.x, pt2.y, pt1.x, pt3.y));
		}
	}

	PopState();
}


void
TBox::DrawWindowScrollers(bool force)
{
	rgb_color backgroundColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color dark = tint_color(backgroundColor, B_DARKEN_4_TINT);
	bool updateUp = false;
	bool updateDown = false;

	BRect rect = fWindow->WindowView()->Bounds();
	if (rect.top != 0) {
		updateUp = true;
		fUpScroller = true;
	} else {
		fUpScroller = false;
		if (force)
			updateUp = true;
	}

	int32 groupIndex = fManager->CurrentIndex();
	int32 maxIndex = fManager->CountWindows(groupIndex) - 1;

	BRect lastFrame(0, 0, 0, 0);
	if (maxIndex >= 0)
		lastFrame = fWindow->WindowView()->FrameOf(maxIndex);

	if (maxIndex >= 0 && lastFrame.bottom > rect.bottom) {
		updateDown = true;
		fDownScroller = true;
	} else {
		fDownScroller = false;
		if (force)
			updateDown = true;
	}

	PushState();
	SetDrawingMode(B_OP_COPY);

	rect = fWindow->WindowView()->Frame();
	rect.InsetBy(-3, 0);
	if (updateUp) {
		if (fUpScroller) {
			SetHighColor(dark);
			BPoint pt1, pt2, pt3;
			pt1.x = rect.left - 6;
			pt1.y = rect.top + 3;
			pt2.y = pt3.y = pt1.y + 4;
			pt2.x = pt1.x - 4;
			pt3.x = pt1.x + 4;
			FillTriangle(pt1, pt2, pt3);

			pt1.x += rect.Width() + 12;
			pt2.x += rect.Width() + 12;
			pt3.x += rect.Width() + 12;
			FillTriangle(pt1, pt2, pt3);
		} else if (force) {
			FillRect(BRect(rect.left - 10, rect.top + 3, rect.left - 2,
				rect.top + 7), B_SOLID_LOW);
			FillRect(BRect(rect.right + 2, rect.top + 3, rect.right + 10,
				rect.top + 7), B_SOLID_LOW);
		}
	}
	if (updateDown) {
		if (fDownScroller) {
			SetHighColor(dark);
			BPoint pt1, pt2, pt3;
			pt1.x = rect.left - 6;
			pt1.y = rect.bottom - 3;
			pt2.y = pt3.y = pt1.y - 4;
			pt2.x = pt1.x - 4;
			pt3.x = pt1.x + 4;
			FillTriangle(pt1, pt2, pt3);

			pt1.x += rect.Width() + 12;
			pt2.x += rect.Width() + 12;
			pt3.x += rect.Width() + 12;
			FillTriangle(pt1, pt2, pt3);
		} else if (force) {
			FillRect(BRect(rect.left - 10, rect.bottom - 7, rect.left - 2,
				rect.bottom - 3), B_SOLID_LOW);
			FillRect(BRect(rect.right + 2, rect.bottom - 7, rect.right + 10,
				rect.bottom - 3), B_SOLID_LOW);
		}
	}

	PopState();
	Sync();
}


//	#pragma mark -


TSwitcherWindow::TSwitcherWindow(BRect frame, TSwitchManager* manager)
	:
	BWindow(frame, "Twitcher", B_MODAL_WINDOW_LOOK,	B_MODAL_ALL_WINDOW_FEEL,
		B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE, B_ALL_WORKSPACES),
	fManager(manager),
	fHairTrigger(true)
{
	BRect rect = frame;
	rect.OffsetTo(B_ORIGIN);
	rect.InsetBy(kHorizontalMargin, 0);
	rect.top = kVerticalMargin;
	rect.bottom = rect.top + kSlotSize - 1;

	fIconView = new TIconView(rect, manager, this);

	rect.top = rect.bottom + (kVerticalMargin * 1 + 4);
	rect.InsetBy(9, 0);

	fWindowView = new TWindowView(rect, manager, this);
	fWindowView->ResizeToPreferred();

	fTopView = new TBox(Bounds(), fManager, this, fIconView);
	AddChild(fTopView);

	SetPulseRate(0);
	fTopView->AddChild(fIconView);
	fTopView->AddChild(fWindowView);
}


TSwitcherWindow::~TSwitcherWindow()
{
}


void
TSwitcherWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_UNMAPPED_KEY_DOWN:
		case B_KEY_DOWN:
		{
			int32 repeats = 0;
			if (message->FindInt32("be:key_repeat", &repeats) == B_OK
				&& (fSkipKeyRepeats || (repeats % 6) != 0))
				break;

			// The first actual key press let's us listening to repeated keys
			fSkipKeyRepeats = false;

			uint32 rawChar;
			uint32 modifiers;
			message->FindInt32("raw_char", 0, (int32*)&rawChar);
			message->FindInt32("modifiers", 0, (int32*)&modifiers);
			DoKey(rawChar, modifiers);
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


void
TSwitcherWindow::Redraw(int32 index)
{
	BRect frame = fIconView->FrameOf(index);
	frame.right = fIconView->Bounds().right;
	fIconView->Invalidate(frame);
}


void
TSwitcherWindow::DoKey(uint32 key, uint32 modifiers)
{
	bool forward = ((modifiers & B_SHIFT_KEY) == 0);

	switch (key) {
		case B_RIGHT_ARROW:
			fManager->CycleApp(true, false);
			break;

		case B_LEFT_ARROW:
		case '1':
			fManager->CycleApp(false, false);
			break;

		case B_UP_ARROW:
			fManager->CycleWindow(false, false);
			break;

		case B_DOWN_ARROW:
			fManager->CycleWindow(true, false);
			break;

		case B_TAB:
			fManager->CycleApp(forward, false);
			break;

		case B_ESCAPE:
			fManager->Stop(false, 0);
			break;

		case B_SPACE:
		case B_ENTER:
			fManager->Stop(true, modifiers);
			break;

		case 'q':
		case 'Q':
			fManager->QuitApp();
			break;

		case 'h':
		case 'H':
			fManager->HideApp();
			break;

#if _ALLOW_STICKY_
		case 's':
		case 'S':
			if (fHairTrigger) {
				SetLook(B_TITLED_WINDOW_LOOK);
				fHairTrigger = false;
			} else {
				SetLook(B_MODAL_WINDOW_LOOK);
				fHairTrigger = true;
			}
			break;
#endif
	}
}


bool
TSwitcherWindow::QuitRequested()
{
	((TBarApp*)be_app)->Settings()->switcherLoc = Frame().LeftTop();
	fManager->Stop(false, 0);
	return false;
}


void
TSwitcherWindow::WindowActivated(bool state)
{
	if (state)
		fManager->Unblock();
}


void
TSwitcherWindow::Update(int32 prev, int32 current, int32 previousSlot,
	int32 currentSlot, bool forward)
{
	if (!IsHidden())
		fIconView->Update(prev, current, previousSlot, currentSlot, forward);
	else
		fIconView->CenterOn(current);

	fWindowView->UpdateGroup(current, 0);
}


void
TSwitcherWindow::Hide()
{
	fIconView->Hiding();
	SetPulseRate(0);
	BWindow::Hide();
}


void
TSwitcherWindow::Show()
{
	fHairTrigger = true;
	fSkipKeyRepeats = true;
	fIconView->Showing();
	SetPulseRate(100000);
	SetLook(B_MODAL_WINDOW_LOOK);
	BWindow::Show();
}


TBox*
TSwitcherWindow::TopView()
{
	return fTopView;
}


bool
TSwitcherWindow::HairTrigger()
{
	return fHairTrigger;
}


inline int32
TSwitcherWindow::SlotOf(int32 i)
{
	return fIconView->SlotOf(i);
}


inline TIconView*
TSwitcherWindow::IconView()
{
	return fIconView;
}


inline TWindowView*
TSwitcherWindow::WindowView()
{
	return fWindowView;
}


//	#pragma mark -


TIconView::TIconView(BRect frame, TSwitchManager* manager,
		TSwitcherWindow* switcherWindow)
	: BView(frame, "main_view", B_FOLLOW_NONE,
		B_WILL_DRAW | B_PULSE_NEEDED),
	fAutoScrolling(false),
	fSwitcher(switcherWindow),
	fManager(manager)
{
	BRect rect(0, 0, kSlotSize - 1, kSlotSize - 1);
	rgb_color color = tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
		B_DARKEN_1_TINT);

	fOffView = new BView(rect, "off_view", B_FOLLOW_NONE, B_WILL_DRAW);
	fOffView->SetHighColor(color);
	fOffBitmap = new BBitmap(rect, B_RGB32, true);
	fOffBitmap->AddChild(fOffView);

	fCurrentSmall = new BBitmap(BRect(0, 0, 15, 15), kIconFormat);
	fCurrentLarge = new BBitmap(BRect(0, 0, 31, 31), kIconFormat);

	SetViewColor(color);
	SetLowColor(color);
}


TIconView::~TIconView()
{
	delete fCurrentSmall;
	delete fCurrentLarge;
	delete fOffBitmap;
}


void
TIconView::KeyDown(const char* /*bytes*/, int32 /*numBytes*/)
{
}


void
TIconView::CacheIcons(TTeamGroup* teamGroup)
{
	const BBitmap* bitmap = teamGroup->SmallIcon();
	ASSERT(bitmap);
	fCurrentSmall->SetBits(bitmap->Bits(), bitmap->BitsLength(), 0,
		bitmap->ColorSpace());

	bitmap = teamGroup->LargeIcon();
	ASSERT(bitmap);
	fCurrentLarge->SetBits(bitmap->Bits(), bitmap->BitsLength(), 0,
		bitmap->ColorSpace());
}


void
TIconView::AnimateIcon(BBitmap* startIcon, BBitmap* endIcon)
{
	BRect centerRect(kCenterSlot * kSlotSize, 0,
		(kCenterSlot + 1) * kSlotSize - 1, kSlotSize - 1);
	BRect startIconBounds = startIcon->Bounds();
	BRect bounds = Bounds();
	float width = startIconBounds.Width();
	int32 amount = (width < 20) ? -2 : 2;

	// center the starting icon inside of centerRect
	float off = (centerRect.Width() - width) / 2;
	startIconBounds.OffsetTo(BPoint(off, off));

	// scroll the centerRect to correct location
	centerRect.OffsetBy(bounds.left, 0);

	BRect destRect = fOffBitmap->Bounds();
	// scroll to the centerRect location
	destRect.OffsetTo(centerRect.left, 0);
	// center the destRect inside of centerRect.
	off = (centerRect.Width() - destRect.Width()) / 2;
	destRect.OffsetBy(BPoint(off, off));

	fOffBitmap->Lock();

	for (int i = 0; i < 2; i++) {
		startIconBounds.InsetBy(amount, amount);
		snooze(20000);
		fOffView->SetDrawingMode(B_OP_COPY);
		fOffView->FillRect(fOffView->Bounds());
		fOffView->SetDrawingMode(B_OP_ALPHA);
		fOffView->DrawBitmap(startIcon, startIconBounds);
		fOffView->Sync();
		DrawBitmap(fOffBitmap, destRect);
	}
	for (int i = 0; i < 2; i++) {
		startIconBounds.InsetBy(amount, amount);
		snooze(20000);
		fOffView->SetDrawingMode(B_OP_COPY);
		fOffView->FillRect(fOffView->Bounds());
		fOffView->SetDrawingMode(B_OP_ALPHA);
		fOffView->DrawBitmap(endIcon, startIconBounds);
		fOffView->Sync();
		DrawBitmap(fOffBitmap, destRect);
	}

	fOffBitmap->Unlock();
}


void
TIconView::Update(int32, int32 current, int32 previousSlot, int32 currentSlot,
	bool forward)
{
	// Animate the shrinking of the currently centered icon.
	AnimateIcon(fCurrentLarge, fCurrentSmall);

	int32 nslots = abs(previousSlot - currentSlot);
	int32 stepSize = kScrollStep;

	if (forward && (currentSlot < previousSlot)) {
		// we were at the end of the list and we just moved to the start
		forward = false;
		if (previousSlot - currentSlot > 4)
			stepSize *= 2;
	} else if (!forward && (currentSlot > previousSlot)) {
		// we're are moving backwards and we just hit start of list and
		// we wrapped to the end.
		forward = true;
		if (currentSlot - previousSlot > 4)
			stepSize *= 2;
	}

	int32 scrollValue = forward ? stepSize : -stepSize;
	int32 total = 0;

	fAutoScrolling = true;
	while (total < (nslots * kSlotSize)) {
		ScrollBy(scrollValue, 0);
		snooze(1000);
		total += stepSize;
		Window()->UpdateIfNeeded();
	}
	fAutoScrolling = false;

	TTeamGroup* teamGroup = (TTeamGroup*)fManager->GroupList()->ItemAt(current);
	ASSERT(teamGroup);
	CacheIcons(teamGroup);

	// Animate the expansion of the currently centered icon
	AnimateIcon(fCurrentSmall, fCurrentLarge);
}


void
TIconView::CenterOn(int32 index)
{
	BRect rect = FrameOf(index);
	ScrollTo(rect.left - (kCenterSlot * kSlotSize), 0);
}


int32
TIconView::ItemAtPoint(BPoint point) const
{
	float tmpPointVerticalIndex = (point.x / kSlotSize) - kCenterSlot;
	if (tmpPointVerticalIndex < 0)
		return -1;

	int32 pointVerticalIndex = (int32)tmpPointVerticalIndex;

	for (int32 i = 0, verticalIndex = 0; ; i++) {

		TTeamGroup* teamGroup = (TTeamGroup*)fManager->GroupList()->ItemAt(i);
		if (teamGroup == NULL)
			break;

		if (!OKToUse(teamGroup))
			continue;

		if (verticalIndex == pointVerticalIndex)
			return i;

		verticalIndex++;
	}
	return -1;
}


void
TIconView::ScrollTo(BPoint where)
{
	BView::ScrollTo(where);
	fSwitcher->TopView()->DrawIconScrollers(true);
}


int32
TIconView::IndexAt(int32 slot) const
{
	BList* list = fManager->GroupList();
	int32 count = list->CountItems();
	int32 slotIndex = 0;

	for (int32 i = 0; i < count; i++) {
		TTeamGroup* teamGroup = (TTeamGroup*)list->ItemAt(i);

		if (!OKToUse(teamGroup))
			continue;

		if (slotIndex == slot) {
			return i;
		}
		slotIndex++;
	}
	return -1;
}


int32
TIconView::SlotOf(int32 index) const
{
	BRect rect = FrameOf(index);

	return (int32)(rect.left / kSlotSize) - kCenterSlot;
}


BRect
TIconView::FrameOf(int32 index) const
{
	BList* list = fManager->GroupList();
	int32 visible = kCenterSlot - 1;
		// first few slots in view are empty

	TTeamGroup* teamGroup;
	for (int32 i = 0; i <= index; i++) {
		teamGroup = (TTeamGroup*)list->ItemAt(i);

		if (!OKToUse(teamGroup))
			continue;

		visible++;
	}

	return BRect(visible * kSlotSize, 0, (visible + 1) * kSlotSize - 1,
		kSlotSize - 1);
}


void
TIconView::DrawTeams(BRect update)
{
	int32 mainIndex = fManager->CurrentIndex();
	BList* list = fManager->GroupList();
	int32 count = list->CountItems();

	BRect rect(kCenterSlot * kSlotSize, 0,
		(kCenterSlot + 1) * kSlotSize - 1, kSlotSize - 1);

	for (int32 i = 0; i < count; i++) {
		TTeamGroup* teamGroup = (TTeamGroup*)list->ItemAt(i);

		if (!OKToUse(teamGroup))
			continue;

		if (rect.Intersects(update) && teamGroup) {
			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

			teamGroup->Draw(this, rect, !fAutoScrolling && (i == mainIndex));

			if (i == mainIndex)
				CacheIcons(teamGroup);

			SetDrawingMode(B_OP_COPY);
		}
		rect.OffsetBy(kSlotSize, 0);
	}
}


void
TIconView::Draw(BRect update)
{
	DrawTeams(update);
}


void
TIconView::MouseDown(BPoint where)
{
	int32 index = ItemAtPoint(where);
	if (index >= 0) {
		int32 previousIndex = fManager->CurrentIndex();
		int32 previousSlot = fManager->CurrentSlot();
		int32 currentSlot = SlotOf(index);
		fManager->SwitchToApp(previousIndex, index, (currentSlot
			> previousSlot));
	}
}


void
TIconView::Pulse()
{
	uint32 modifiersKeys = modifiers();
	if (fSwitcher->HairTrigger() && (modifiersKeys & B_CONTROL_KEY) == 0) {
		fManager->Stop(true, modifiersKeys);
		return;
	}

	if (!fSwitcher->HairTrigger()) {
		uint32 buttons;
		BPoint point;
		GetMouse(&point, &buttons);
		if (buttons != 0) {
			point = ConvertToScreen(point);
			if (!Window()->Frame().Contains(point))
				fManager->Stop(false, 0);
		}
	}
}


void
TIconView::Showing()
{
}


void
TIconView::Hiding()
{
	ScrollTo(B_ORIGIN);
}


//	#pragma mark -


TWindowView::TWindowView(BRect rect, TSwitchManager* manager,
		TSwitcherWindow* window)
	: BView(rect, "wlist_view", B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED),
	fCurrentToken(-1),
	fItemHeight(-1),
	fSwitcher(window),
	fManager(manager)
{
	SetFont(be_plain_font);
}


void
TWindowView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
TWindowView::ScrollTo(BPoint where)
{
	BView::ScrollTo(where);
	fSwitcher->TopView()->DrawWindowScrollers(true);
}


BRect
TWindowView::FrameOf(int32 index) const
{
	return BRect(0, index * fItemHeight, 100, ((index + 1) * fItemHeight) - 1);
}


void
TWindowView::GetPreferredSize(float* _width, float* _height)
{
	font_height	fh;
	be_plain_font->GetHeight(&fh);
	fItemHeight = (int32) fh.ascent + fh.descent;

	// top & bottom margin
	fItemHeight = fItemHeight + 3 + 3;

	// want fItemHeight to be divisible by kWindowScrollSteps.
	fItemHeight = ((((int)fItemHeight) + kWindowScrollSteps)
		/ kWindowScrollSteps) * kWindowScrollSteps;

	*_height = fItemHeight;

	// leave width alone
	*_width = Bounds().Width();
}


void
TWindowView::ShowIndex(int32 newIndex)
{
	// convert index to scroll location
	BPoint point(0, newIndex * fItemHeight);
	BRect bounds = Bounds();

	int32 groupIndex = fManager->CurrentIndex();
	TTeamGroup* teamGroup
		= (TTeamGroup*)fManager->GroupList()->ItemAt(groupIndex);
	if (teamGroup == NULL)
		return;

	window_info* windowInfo = fManager->WindowInfo(groupIndex, newIndex);
	if (windowInfo == NULL)
		return;

	fCurrentToken = windowInfo->server_token;
	free(windowInfo);

	if (bounds.top == point.y)
		return;

	int32 oldIndex = (int32) (bounds.top / fItemHeight);

	int32 stepSize = (int32) (fItemHeight / kWindowScrollSteps);
	int32 scrollValue = (newIndex > oldIndex) ? stepSize : -stepSize;
	int32 total = 0;
	int32 nslots = abs(newIndex - oldIndex);

	while (total < (nslots * (int32)fItemHeight)) {
		ScrollBy(0, scrollValue);
		snooze(10000);
		total += stepSize;
		Window()->UpdateIfNeeded();
	}
}


void
TWindowView::Draw(BRect update)
{
	int32 groupIndex = fManager->CurrentIndex();
	TTeamGroup* teamGroup
		= (TTeamGroup*)fManager->GroupList()->ItemAt(groupIndex);

	if (teamGroup == NULL)
		return;

	BRect bounds = Bounds();
	int32 windowIndex = (int32) (bounds.top / fItemHeight);
	BRect windowRect = bounds;

	windowRect.top = windowIndex * fItemHeight;
	windowRect.bottom = (windowIndex + 1) * fItemHeight - 1;

	for (int32 i = 0; i < 3; i++) {
		if (!update.Intersects(windowRect)) {
			windowIndex++;
			windowRect.OffsetBy(0, fItemHeight);
			continue;
		}

		// is window in current workspace?

		bool local = true;
		bool minimized = false;
		BString title;

		client_window_info* windowInfo
			= fManager->WindowInfo(groupIndex, windowIndex);
		if (windowInfo != NULL) {
			if (SmartStrcmp(windowInfo->name, teamGroup->Name()) != 0)
				title << teamGroup->Name() << ": " << windowInfo->name;
			else
				title = teamGroup->Name();

			int32 currentWorkspace = current_workspace();
			if ((windowInfo->workspaces & (1 << currentWorkspace)) == 0)
				local = false;

			minimized = windowInfo->is_mini;
			free(windowInfo);
		} else
			title = teamGroup->Name();

		if (!title.Length())
			return;

		float stringWidth = StringWidth(title.String());
		float maxWidth = bounds.Width() - (14 + 5);

		if (stringWidth > maxWidth) {
			// window title is too long, need to truncate
			TruncateString(&title, B_TRUNCATE_MIDDLE, maxWidth);
			stringWidth = maxWidth;
		}

		BPoint point((bounds.Width() - (stringWidth + 14 + 5)) / 2,
			windowRect.bottom - 4);
		BPoint p(point.x, (windowRect.top + windowRect.bottom) / 2);
		SetDrawingMode(B_OP_OVER);
		const BBitmap* bitmap = AppResSet()->FindBitmap(B_MESSAGE_TYPE,
			minimized ? R_WindowHiddenIcon : R_WindowShownIcon);
		p.y -= (bitmap->Bounds().bottom - bitmap->Bounds().top) / 2;
		DrawBitmap(bitmap, p);

		if (!local) {
			SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),
				B_DARKEN_4_TINT));
			p.x -= 8;
			p.y += 4;
			StrokeLine(p + BPoint(2, 2), p + BPoint(2, 2));
			StrokeLine(p + BPoint(4, 2), p + BPoint(6, 2));

			StrokeLine(p + BPoint(0, 5), p + BPoint(0, 5));
			StrokeLine(p + BPoint(2, 5), p + BPoint(6, 5));

			StrokeLine(p + BPoint(1, 8), p + BPoint(1, 8));
			StrokeLine(p + BPoint(3, 8), p + BPoint(6, 8));

			SetHighColor(0, 0, 0);
		}

		point.x += 21;
		MovePenTo(point);

		DrawString(title.String());
		SetDrawingMode(B_OP_COPY);

		windowIndex++;
		windowRect.OffsetBy(0, fItemHeight);
	}
}


void
TWindowView::UpdateGroup(int32 , int32 windowIndex)
{
	ScrollTo(0, windowIndex * fItemHeight);
	Invalidate(Bounds());
}


void
TWindowView::Pulse()
{
	// If selected window went away then reset to first window
	window_info	*windowInfo = get_window_info(fCurrentToken);
	if (windowInfo == NULL) {
		Invalidate();
		ShowIndex(0);
	} else
		free(windowInfo);
}
