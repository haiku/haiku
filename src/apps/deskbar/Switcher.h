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

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#ifndef SWITCHER_H
#define SWITCHER_H

#include <Box.h>
#include <List.h>
#include <OS.h>
#include <StringView.h>
#include <Window.h>

class TTeamGroup {
public:
	TTeamGroup();
	TTeamGroup(BList *teams, uint32 flags, char *name, const char *sig);
	virtual ~TTeamGroup();

	void Draw(BView *, BRect bounds, bool main);

	BList *TeamList() const
		{ return fTeams; }
	const char *Name() const
		{ return fName; }
	const char *Sig() const
		{ return fSig; }
	uint32 Flags() const
		{ return fFlags; }
	const BBitmap *SmallIcon() const
		{ return fSmallIcon; }
	const BBitmap *LargeIcon() const
		{ return fLargeIcon; }


private:
	BList *fTeams;
	uint32 fFlags;
	char fSig[B_MIME_TYPE_LENGTH];
	char *fName;
	BBitmap	*fSmallIcon;
	BBitmap	*fLargeIcon;
};

class TIconView;
class TBox;
class TWindowView;
class TSwitcherWindow;
struct window_info;

class TSwitchMgr : public BHandler {
public:
	TSwitchMgr(BPoint where);
	virtual ~TSwitchMgr();

	virtual void MessageReceived(BMessage *);

	void Stop(bool doAction, uint32 mods);
	void Unblock();
	int32 CurIndex();
	int32 CurWindow();
	int32 CurSlot();
	BList *GroupList();
	int32 CountVisibleGroups();
	void CycleApp(bool forward, bool activate = false);	
	void CycleWindow(bool forward, bool wrap = true);	
	void SwitchToApp(int32 prevIndex, int32 newIndex, bool forward);	
	void Touch(bool zero = false);
	bigtime_t IdleTime();

	window_info *WindowInfo(int32 groupIndex, int32 wdIndex);
	int32 CountWindows(int32 groupIndex, bool inCurrentWorkspace = false);
	TTeamGroup *FindTeam(team_id, int32 *index);
		
private:
	void MainEntry(BMessage *);	
	void Process(bool forward, bool byWindow = false);	
	void QuickSwitch(BMessage *);	
	void SwitchWindow(team_id , bool forward, bool activate);
	bool ActivateApp(bool forceShow, bool allowWorkspaceSwitch);
	void ActivateWindow(int32 windowID = -1);

	TSwitcherWindow	*fWindow;
	sem_id fMainMonitor;
	bool fBlock;
	bigtime_t fSkipUntil;
	BList fGroupList;
	int32 fCurIndex;
	int32 fCurWindow;
	int32 fCurSlot;
	int32 fWindowID;
	bigtime_t fLastActivity;
};

class TSwitcherWindow : public BWindow {
public:
	TSwitcherWindow(BRect frame, TSwitchMgr *mgr);
	virtual	~TSwitcherWindow();

	virtual	bool QuitRequested();
	virtual void DispatchMessage(BMessage *, BHandler *);
	virtual void MessageReceived(BMessage *);
	virtual	void Show();
	virtual	void Hide();
	virtual void WindowActivated(bool state);

	void DoKey(uint32 key, uint32 mods);
	TIconView *IconView();
	TWindowView *WindowView();
	TBox *TopView();
	bool HairTrigger();
	void Update(int32 prev, int32 cur, int32 prevSlot, int32 curSlot, bool forward);
	int32 SlotOf(int32);
	void Redraw(int32 index);

private:

	TSwitchMgr *fMgr;
	TIconView *fIconView;
	TBox *fTopView;
	TWindowView *fWindowView;
	bool fHairTrigger;
};

class TWindowView : public BView {
public:
	TWindowView(BRect, TSwitchMgr *, TSwitcherWindow *);

	void UpdateGroup(int32 groupIndex, int32 windowIndex);

	virtual	void Draw(BRect update);
	virtual	void Pulse();
	virtual	void GetPreferredSize(float *w, float *h);
	void ScrollTo(float x, float y)
		{ ScrollTo(BPoint(x,y)); }
	virtual	void ScrollTo(BPoint where);

	void ShowIndex(int32 windex);
	BRect FrameOf(int32 index) const;

private:

	int32 fCurToken;
	float fItemHeight;
	TSwitcherWindow	*fSwitcher;
	TSwitchMgr *fMgr;
	bool fLocal;
};

class TIconView : public BView {
public:
	TIconView(BRect , TSwitchMgr *, TSwitcherWindow *);
	~TIconView();

	void Showing();
	void Hiding();

	virtual void KeyDown(const char *bytes, int32 numBytes);
	virtual	void Pulse();
	virtual	void MouseDown(BPoint );
	virtual	void Draw(BRect );
	
	void ScrollTo(float x, float y)
		{ ScrollTo(BPoint(x,y)); }
	virtual	void ScrollTo(BPoint where);
	void Update(int32 prev, int32 cur, int32 prevSlot, int32 curSlot, bool forward);
	void DrawTeams(BRect update);
	int32 SlotOf(int32) const;
	BRect FrameOf(int32) const;
	int32 ItemAtPoint(BPoint) const;
	int32 IndexAt(int32 slot) const;
	void CenterOn(int32 index);

private:
		
	void CacheIcons(TTeamGroup *);
	void AnimateIcon(BBitmap *startIcon, BBitmap *endIcon);

	bool fAutoScrolling;
	bool fCapsState;
	TSwitcherWindow	*fSwitcher;
	TSwitchMgr *fMgr;
	BBitmap	*fOffBitmap;
	BView *fOffView;
	BBitmap	*fCurSmall;
	BBitmap	*fCurLarge;
};

class TBox : public BBox {
public:
	TBox(BRect bounds, TSwitchMgr *manager, TSwitcherWindow *, TIconView *iconView);

	virtual void Draw(BRect update);
	virtual void AllAttached();
	virtual	void DrawIconScrollers(bool force);
	virtual	void DrawWindowScrollers(bool force);
	virtual	void MouseDown(BPoint where);

private:
	TSwitchMgr *fMgr;
	TSwitcherWindow	*fWindow;
	TIconView *fIconView;
	BRect fCenter;
	bool fLeftScroller;
	bool fRightScroller;
	bool fUpScroller;
	bool fDownScroller;
};

inline int32 TSwitcherWindow::SlotOf(int32 i)
	{ return fIconView->SlotOf(i); }

inline TIconView *TSwitcherWindow::IconView()
	{ return fIconView; }

inline TWindowView *TSwitcherWindow::WindowView()
	{ return fWindowView; }

inline void TSwitchMgr::Touch(bool zero)
	{ fLastActivity = zero ? 0 : system_time(); }

#endif
