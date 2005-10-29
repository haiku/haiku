//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		TabView.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BTab creates individual "tabs" that can be assigned
//                  to specific views.
//                  BTabView provides the framework for containing and
//                  managing groups of BTab objects.
//------------------------------------------------------------------------------

#ifndef _TAB_VIEW_H
#define _TAB_VIEW_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <View.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
enum tab_position {
	B_TAB_FIRST = 999,
	B_TAB_FRONT,
	B_TAB_ANY
};

// Globals ---------------------------------------------------------------------


// BTab class ------------------------------------------------------------------
class BTab : public BArchivable {

public:
						BTab(BView * tabView = NULL);
						virtual ~BTab();

						BTab(BMessage *archive);
static BArchivable		*Instantiate(BMessage *archive);

virtual	status_t		Archive(BMessage *archive, bool deep = true) const;
virtual status_t		Perform(uint32 d, void *arg);

		const char		*Label() const;
virtual	void			SetLabel(const char *label);

		bool			IsSelected() const;
virtual	void			Select(BView *owner);
virtual	void			Deselect();

virtual	void			SetEnabled(bool enabled);
		bool			IsEnabled() const;

		void			MakeFocus(bool inFocus = true);
		bool			IsFocus() const;

		//	sets/gets the view to be displayed for this tab
virtual	void			SetView(BView *view);
		BView			*View() const;

virtual void			DrawFocusMark(BView *owner, BRect frame);
virtual void			DrawLabel(BView *owner, BRect frame);
virtual void			DrawTab(BView *owner, BRect frame, tab_position position,
							bool full = true);

private:
virtual	void			_ReservedTab1();
virtual	void			_ReservedTab2();
virtual	void			_ReservedTab3();
virtual	void			_ReservedTab4();
virtual	void			_ReservedTab5();
virtual	void			_ReservedTab6();
virtual	void			_ReservedTab7();
virtual	void			_ReservedTab8();
virtual	void			_ReservedTab9();
virtual	void			_ReservedTab10();
virtual	void			_ReservedTab11();
virtual	void			_ReservedTab12();

	BTab				&operator=(const BTab &);
		
	bool 				fEnabled;
	bool				fSelected;
	bool				fFocus;
	BView				*fView;
	uint32				_reserved[12];
};
//------------------------------------------------------------------------------


// BTabView class --------------------------------------------------------------
class BTabView : public BView
{
public:
						BTabView(BRect frame, const char *name,
							button_width width = B_WIDTH_AS_USUAL, 
							uint32 resizingMode = B_FOLLOW_ALL,
							uint32 flags = B_FULL_UPDATE_ON_RESIZE |
								B_WILL_DRAW | B_NAVIGABLE_JUMP |
								B_FRAME_EVENTS | B_NAVIGABLE);
						~BTabView();

						BTabView(BMessage *archive);
static BArchivable		*Instantiate(BMessage *archive);
virtual	status_t		Archive(BMessage*, bool deep=true) const;
virtual status_t		Perform(perform_code d, void *arg);

virtual	void			WindowActivated(bool active);
virtual void 			AttachedToWindow();		
virtual	void			AllAttached();
virtual	void			AllDetached();
virtual	void			DetachedFromWindow();

virtual void 			MessageReceived(BMessage *message);
virtual void 			FrameMoved(BPoint newLocation);
virtual void			FrameResized(float width,float height);
virtual void			KeyDown(const char *bytes, int32 numBytes);
virtual void			MouseDown(BPoint point);
virtual void			MouseUp(BPoint point);
virtual void 			MouseMoved(BPoint point, uint32 transit,
								   const BMessage *message);
virtual	void			Pulse();

virtual	void			Select(int32 tab);
		int32			Selection() const;

virtual	void			MakeFocus(bool focused = true);
virtual	void			SetFocusTab(int32 tab, bool focused);
		int32			FocusTab() const;

virtual	void			Draw(BRect updateRect);
virtual	BRect			DrawTabs();
virtual	void			DrawBox(BRect selTabRect);
virtual	BRect			TabFrame(int32 tab_index) const;

virtual	void			SetFlags(uint32 flags);
virtual	void			SetResizingMode(uint32 mode);

virtual void 			GetPreferredSize(float *width, float *height);
virtual void 			ResizeToPreferred();

virtual BHandler		*ResolveSpecifier(BMessage *message, int32 index,
							BMessage *specifier, int32 what, const char *property);
virtual	status_t		GetSupportedSuites(BMessage *message);

virtual	void			AddTab(BView *target, BTab *tab = NULL);
#if !_PR3_COMPATIBLE_
virtual	BTab			*RemoveTab(int32 tabIndex);
#else
virtual	BTab			*RemoveTab(int32 tabIndex) const;
#endif

virtual	BTab			*TabAt ( int32 tab_index ) const;

virtual	void			SetTabWidth(button_width width);
		button_width	TabWidth() const;
		
virtual	void			SetTabHeight(float height);
		float			TabHeight() const;

		BView			*ContainerView() const;

		int32			CountTabs() const;
		BView			*ViewForTab(int32 tabIndex) const;

private:
		void			_InitObject();

virtual	void			_ReservedTabView1();
virtual	void			_ReservedTabView2();
virtual	void			_ReservedTabView3();
virtual	void			_ReservedTabView4();
virtual	void			_ReservedTabView5();
virtual	void			_ReservedTabView6();
virtual	void			_ReservedTabView7();
virtual	void			_ReservedTabView8();
virtual	void			_ReservedTabView9();
virtual	void			_ReservedTabView10();
virtual	void			_ReservedTabView11();
virtual	void			_ReservedTabView12();

						BTabView(const BTabView &);
		BTabView		&operator=(const BTabView &);
	
		BList			*fTabList;
		BView			*fContainerView;
		button_width	fTabWidthSetting;
		float 			fTabWidth;
		float			fTabHeight;
		int32			fSelection;
		int32			fInitialSelection;
		int32			fFocus;	
		uint32			_reserved[12];
};
//------------------------------------------------------------------------------

#endif // _TAB_VIEW_H

/*
 * $Log $
 *
 * $Id  $
 *
 */
