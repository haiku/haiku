/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SCROLL_VIEW_H
#define _SCROLL_VIEW_H


#include <ScrollBar.h>


/*!	The BScrollView is a convenience class to add a scrolling
	mechanism to the target view.
*/
class BScrollView : public BView {
public:
								BScrollView(const char* name, BView* target,
									uint32 resizingMode = B_FOLLOW_LEFT_TOP,
									uint32 flags = 0, bool horizontal = false,
									bool vertical = false,
									border_style border = B_FANCY_BORDER);
								BScrollView(const char* name, BView* target,
									uint32 flags, bool horizontal,
									bool vertical, border_style border
										= B_FANCY_BORDER);
								BScrollView(BMessage* archive);
	virtual						~BScrollView();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive, bool deep = true) const;
	virtual status_t			AllUnarchived(const BMessage* archive);

	// Hook methods
	virtual	void				AllAttached();
	virtual	void				AllDetached();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();

	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 code,
									const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint where);

	virtual	void				WindowActivated(bool active);

	// Size
	virtual	void				GetPreferredSize(float* _width, float* _height);
	virtual	void				ResizeToPreferred();

	virtual	void				MakeFocus(bool focus = true);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	// BScrollBar
			BScrollBar*			ScrollBar(orientation direction) const;

	virtual	void				SetBorder(border_style border);
			border_style		Border() const;
			void				SetBorders(uint32 borders);
			uint32				Borders() const;

	virtual	status_t			SetBorderHighlighted(bool highlight);
			bool				IsBorderHighlighted() const;

			void				SetTarget(BView* target);
			BView*				Target() const;

	// Scripting
	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* message);

	virtual	status_t			Perform(perform_code d, void* arg);

protected:
	virtual	void				LayoutInvalidated(bool descendants = false);
	virtual	void				DoLayout();

private:
	// FBC padding and forbidden methods
	virtual	void				_ReservedScrollView1();
	virtual	void				_ReservedScrollView2();
	virtual	void				_ReservedScrollView3();
	virtual	void				_ReservedScrollView4();

			BScrollView&		operator=(const BScrollView& other);

private:
	friend class BView;

			void				_Init(bool horizontal, bool vertical);
			float				_BorderSize() const;
			BRect				_InnerFrame() const;
			BSize				_ComputeSize(BSize targetSize) const;
			BRect				_ComputeFrame(BRect targetRect) const;
			void				_AlignScrollBars(bool horizontal,
									bool vertical, BRect targetFrame);

	static	BRect				_ComputeFrame(BRect frame, bool horizontal,
									bool vertical, border_style border,
									uint32 borders);
	static	BRect				_ComputeFrame(BView* target, bool horizontal,
									bool vertical, border_style border,
									uint32 borders);
	static	float				_BorderSize(border_style border);
	static	uint32				_ModifyFlags(uint32 flags, BView* target,
									border_style border);
	static	void				_InsetBorders(BRect& frame, border_style border,
									uint32 borders, bool expand = false);
private:
			BView*				fTarget;
			BScrollBar*			fHorizontalScrollBar;
			BScrollBar*			fVerticalScrollBar;
			border_style		fBorder;
			uint16				fPreviousWidth;
			uint16				fPreviousHeight;
			bool				fHighlighted;
			uint32				fBorders;

			uint32				_reserved[2];
};

#endif // _SCROLL_VIEW_H
