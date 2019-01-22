/*
 * Copyright 2001-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TAB_VIEW_H
#define _TAB_VIEW_H


#include <View.h>


class BTabView;


enum tab_position {
	B_TAB_FIRST = 999,
	B_TAB_FRONT,
	B_TAB_ANY
};


class BTab : public BArchivable {
public:
								BTab(BView* contentsView = NULL);
	virtual						~BTab();

								BTab(BMessage* archive);
	static	BArchivable*		Instantiate(BMessage* archive);

	virtual	status_t			Archive(BMessage* data, bool deep = true) const;
	virtual	status_t			Perform(uint32 d, void* arg);

			const char*			Label() const;
	virtual	void				SetLabel(const char* label);

			bool				IsSelected() const;
	virtual	void				Select(BView* owner);
	virtual	void				Deselect();

	virtual	void				SetEnabled(bool enable);
			bool				IsEnabled() const;

			void				MakeFocus(bool focus = true);
			bool				IsFocus() const;

	//	sets/gets the view to be displayed for this tab
	virtual	void				SetView(BView* view);
			BView*				View() const;

	virtual	void				DrawFocusMark(BView* owner, BRect frame);
	virtual	void				DrawLabel(BView* owner, BRect frame);
	virtual	void				DrawTab(BView* owner, BRect frame,
									tab_position position, bool full = true);

	class Private;

private:
	friend class Private;

	// FBC padding and forbidden methods
	virtual	void				_ReservedTab1();
	virtual	void				_ReservedTab2();
	virtual	void				_ReservedTab3();
	virtual	void				_ReservedTab4();
	virtual	void				_ReservedTab5();
	virtual	void				_ReservedTab6();
	virtual	void				_ReservedTab7();
	virtual	void				_ReservedTab8();
	virtual	void				_ReservedTab9();
	virtual	void				_ReservedTab10();
	virtual	void				_ReservedTab11();
	virtual	void				_ReservedTab12();

			BTab&				operator=(const BTab&);

private:
			bool 				fEnabled;
			bool				fSelected;
			bool				fFocus;
			BView*				fView;
			BTabView*			fTabView;

			uint32				_reserved[11];
};


class BTabView : public BView {
public:
			enum tab_side {
				kLeftSide	= 1 << 0,
				kRightSide	= 1 << 1,
				kTopSide	= 1 << 2,
				kBottomSide	= 1 << 3
			};

								BTabView(const char* name,
									button_width width = B_WIDTH_FROM_WIDEST,
									uint32 flags = B_FULL_UPDATE_ON_RESIZE
										| B_WILL_DRAW | B_NAVIGABLE_JUMP
										| B_FRAME_EVENTS | B_NAVIGABLE);
								BTabView(BRect frame, const char* name,
									button_width width = B_WIDTH_AS_USUAL,
									uint32 resizeMask = B_FOLLOW_ALL,
									uint32 flags = B_FULL_UPDATE_ON_RESIZE
										| B_WILL_DRAW | B_NAVIGABLE_JUMP
										| B_FRAME_EVENTS | B_NAVIGABLE);
	virtual						~BTabView();

								BTabView(BMessage* archive);
	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;
	virtual status_t			AllUnarchived(const BMessage* from);
	virtual	status_t			Perform(perform_code d, void* arg);

	virtual	void 				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				AllAttached();
	virtual	void				AllDetached();

	virtual	void 				MessageReceived(BMessage* message);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void 				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				Pulse();

	virtual	void				Select(int32 index);
			int32				Selection() const;

	virtual	void				WindowActivated(bool active);
	virtual	void				MakeFocus(bool focused = true);
	virtual	void				SetFocusTab(int32 tab, bool focused);
			int32				FocusTab() const;

	virtual	void				Draw(BRect updateRect);
	virtual	BRect				DrawTabs();
	virtual	void				DrawBox(BRect selectedTabRect);
	virtual	BRect				TabFrame(int32 index) const;

	virtual	void				SetFlags(uint32 flags);
	virtual	void				SetResizingMode(uint32 mode);

	virtual	void				ResizeToPreferred();
	virtual	void				GetPreferredSize(float* _width, float* _height);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual	void 				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* message);

	// BTabView
	virtual	void				AddTab(BView* target, BTab* tab = NULL);
	virtual	BTab*				RemoveTab(int32 tabIndex);

	virtual	BTab*				TabAt(int32 index) const;

	virtual	void				SetTabWidth(button_width width);
			button_width		TabWidth() const;

	virtual	void				SetTabHeight(float height);
			float				TabHeight() const;

	virtual	void				SetBorder(border_style borderStyle);
			border_style		Border() const;

	virtual	void				SetTabSide(tab_side tabSide);
			tab_side			TabSide() const;

			BView*				ContainerView() const;

			int32				CountTabs() const;
			BView*				ViewForTab(int32 tabIndex) const;

private:
	// FBC padding and forbidden methods
	virtual	void				_ReservedTabView3();
	virtual	void				_ReservedTabView4();
	virtual	void				_ReservedTabView5();
	virtual	void				_ReservedTabView6();
	virtual	void				_ReservedTabView7();
	virtual	void				_ReservedTabView8();
	virtual	void				_ReservedTabView9();
	virtual	void				_ReservedTabView10();
	virtual	void				_ReservedTabView11();
	virtual	void				_ReservedTabView12();

								BTabView(const BTabView&);
			BTabView&			operator=(const BTabView&);

private:
			void				_InitObject(bool layouted, button_width width);
			void				_InitContainerView(bool layouted);
			BSize				_TabsMinSize() const;
			float				_BorderWidth() const;
			void				_LayoutContainerView(bool layouted);

private:
			BList*				fTabList;
			BView*				fContainerView;
			button_width		fTabWidthSetting;
			float 				fTabWidth;
			float				fTabHeight;
			int32				fSelection;
			int32				fInitialSelection;
			int32				fFocus;
			float				fTabOffset;
			border_style		fBorderStyle;
			tab_side			fTabSide;

			uint32				_reserved[9];
};

#endif // _TAB_VIEW_H
