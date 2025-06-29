/*
 * Copyright 2003-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MENU_BAR_H
#define _MENU_BAR_H


#include <InterfaceDefs.h>
#include <Menu.h>
#include <OS.h>


enum menu_bar_border {
	B_BORDER_FRAME,
	B_BORDER_CONTENTS,
	B_BORDER_EACH_ITEM
};

class BMenu;
class BWindow;
class BMenuItem;
class BMenuField;


class BMenuBar : public BMenu {
public:
								BMenuBar(BRect frame, const char* name,
									uint32 resizingMode = B_FOLLOW_LEFT_RIGHT
										| B_FOLLOW_TOP,
									menu_layout layout = B_ITEMS_IN_ROW,
									bool resizeToFit = true);
								BMenuBar(const char* name,
									menu_layout layout = B_ITEMS_IN_ROW,
									uint32 flags = B_WILL_DRAW
										| B_FRAME_EVENTS);
								BMenuBar(BMessage* archive);
	virtual						~BMenuBar();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual void				AllAttached();
	virtual void				AllDetached();

	virtual	void				WindowActivated(bool state);
	virtual void				MakeFocus(bool state = true);

	virtual void				ResizeToPreferred();
	virtual void				GetPreferredSize(float* _width,
									float* _height);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);

	virtual	void				Show();
	virtual	void				Hide();

	virtual	void				Draw(BRect updateRect);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual status_t			GetSupportedSuites(BMessage* data);

	virtual	void				SetBorder(menu_bar_border border);
			menu_bar_border		Border() const;
			void				SetBorders(uint32 borders);
			uint32				Borders() const;

	virtual status_t			Perform(perform_code code, void* data);

protected:
			void				StartMenuBar(int32 menuIndex,
									bool sticky = true, bool showMenu = false,
									BRect* special_rect = NULL);

private:
	friend class BWindow;
	friend class BMenuField;
	friend class BMenu;

	virtual	void				_ReservedMenuBar1();
	virtual	void				_ReservedMenuBar2();
	virtual	void				_ReservedMenuBar3();
	virtual	void				_ReservedMenuBar4();

			BMenuBar			&operator=(const BMenuBar &);

	static	int32				_TrackTask(void *arg);
			BMenuItem*			_Track(int32 *action, int32 startIndex = -1,
									bool showMenu = false);
			void				_StealFocus();
			void				_RestoreFocus();
			void				_InitData(menu_layout layout);

			menu_bar_border		fBorder;
			thread_id			fTrackingPID;
			int32				fPrevFocusToken;
			sem_id				fMenuSem;
			BRect*				fLastBounds;
			uint32				fBorders;
			uint32				_reserved[1];

			bool				fTracking;
};


#endif /* _MENU_BAR_H */
