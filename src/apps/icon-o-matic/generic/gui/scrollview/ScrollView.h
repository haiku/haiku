/*
 * Copyright 2006-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#ifndef SCROLL_VIEW_H
#define SCROLL_VIEW_H

#include <View.h>

#include "Scroller.h"

class Scrollable;
class InternalScrollBar;
class ScrollCorner;

enum {
	SCROLL_HORIZONTAL						= 0x01,
	SCROLL_VERTICAL							= 0x02,
	SCROLL_HORIZONTAL_MAGIC					= 0x04,
	SCROLL_VERTICAL_MAGIC					= 0x08,
	SCROLL_VISIBLE_RECT_IS_CHILD_BOUNDS		= 0x10,
	SCROLL_NO_FRAME							= 0x20,
	SCROLL_LIST_FRAME						= 0x40,
};

enum {
	BORDER_LEFT		= 0x01,
	BORDER_TOP		= 0x02,
	BORDER_RIGHT	= 0x04,
	BORDER_BOTTOM	= 0x08,
	BORDER_ALL		= BORDER_LEFT | BORDER_TOP | BORDER_RIGHT | BORDER_BOTTOM
};


class ScrollView : public BView, public Scroller {
public:
								ScrollView(BView* child, uint32 scrollingFlags,
									BRect frame, const char *name,
									uint32 resizingMode, uint32 viewFlags,
									uint32 borderStyle = B_FANCY_BORDER,
									uint32 borderFlags = BORDER_ALL);
#ifdef __HAIKU__
								ScrollView(BView* child, uint32 scrollingFlags,
									const char *name, uint32 viewFlags,
									uint32 borderStyle = B_FANCY_BORDER,
									uint32 borderFlags = BORDER_ALL);
#endif // __HAIKU__
	virtual						~ScrollView();

	virtual	void				AllAttached();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);
	virtual	void				WindowActivated(bool activated);

#ifdef __HAIKU__

	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();

#endif // __HAIKU__

			uint32				ScrollingFlags() const;
			void				SetVisibleRectIsChildBounds(bool flag);
			bool				VisibleRectIsChildBounds() const;

			BView*				Child() const;
			void				ChildFocusChanged(bool focused);

			BScrollBar*			HScrollBar() const;
			BScrollBar*			VScrollBar() const;
			BView*				HVScrollCorner() const;

			void				SetHSmallStep(float hStep);
			void				SetVSmallStep(float vStep);
			void				SetSmallSteps(float hStep, float vStep);
			void				GetSmallSteps(float* hStep,
											  float* vStep) const;
			float				HSmallStep() const;
			float				VSmallStep() const;

	virtual	bool				IsScrolling() const;
	virtual	void				SetScrollingEnabled(bool enabled);

 protected:
	virtual	void				DataRectChanged(BRect oldDataRect,
												BRect newDataRect);
	virtual	void				ScrollOffsetChanged(BPoint oldOffset,
													BPoint newOffset);
	virtual	void				VisibleSizeChanged(float oldWidth,
												   float oldHeight,
												   float newWidth,
												   float newHeight);
	virtual	void				ScrollTargetChanged(Scrollable* oldTarget,
													Scrollable* newTarget);

 private:
			BView*				fChild;			// child view
			uint32				fScrollingFlags;
			InternalScrollBar*	fHScrollBar;	// horizontal scroll bar
			InternalScrollBar*	fVScrollBar;	// vertical scroll bar
			ScrollCorner*		fScrollCorner;	// scroll corner
			bool				fHVisible;		// horizontal/vertical scroll
			bool				fVVisible;		// bar visible flag
			bool				fCornerVisible;	// scroll corner visible flag
			bool				fWindowActive;
			bool				fChildFocused;
			bool				fScrolling;
			float				fHSmallStep;
			float				fVSmallStep;

			uint32				fBorderStyle;
			uint32				fBorderFlags;

			void				_Init(BView* child, uint32 scrollingFlags,
									uint32 borderStyle, uint32 borderFlags);

			void				_ScrollValueChanged(
										InternalScrollBar* scrollBar,
										float value);
			void				_ScrollCornerValueChanged(BPoint offset);

protected:
	virtual	void				_Layout(uint32 flags);

private:
			void				_UpdateScrollBars();
			uint32				_UpdateScrollBarVisibility();

			BRect				_InnerRect() const;
			BRect				_ChildRect() const;
			BRect				_ChildRect(bool hbar, bool vbar) const;
			BRect				_GuessVisibleRect(bool hbar, bool vbar) const;
			BRect				_MaxVisibleRect() const;
#ifdef __HAIKU__
	virtual	BSize				_Size(BSize childSize);
#endif

			void				_SetScrolling(bool scrolling);

	friend class InternalScrollBar;
	friend class ScrollCorner;
};



#endif	// SCROLL_VIEW_H
