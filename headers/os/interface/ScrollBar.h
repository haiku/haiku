/*
 * Copyright 2001-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_SCROLL_BAR_H
#define	_SCROLL_BAR_H


#include <View.h>


#define B_V_SCROLL_BAR_WIDTH	14.0f
#define B_H_SCROLL_BAR_HEIGHT	14.0f

// TODO: shouldn't these be moved into the implementation?
#define SCROLL_BAR_MAXIMUM_KNOB_SIZE	50
#define SCROLL_BAR_MINIMUM_KNOB_SIZE	9

#define DISABLES_ON_WINDOW_DEACTIVATION 1


class BScrollBar : public BView {
public:
								BScrollBar(BRect frame, const char* name,
									BView* target, float min, float max,
									orientation direction);
								BScrollBar(const char* name, BView* target,
									float min, float max,
									enum orientation orientation);
								BScrollBar(BMessage* archive);
	virtual						~BScrollBar();
	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				AttachedToWindow();
			void				SetValue(float value);
			float				Value() const;
			void				SetProportion(float);
			float				Proportion() const;
	virtual	void				ValueChanged(float newValue);

			void				SetRange(float min, float max);
			void				GetRange(float* _min, float* _max) const;
			void				SetSteps(float smallStep, float largeStep);
			void				GetSteps(float* _smallStep,
									float* _largeStep) const;
			void				SetTarget(BView *target);
			void				SetTarget(const char* targetName);
			BView*				Target() const;
			void				SetOrientation(enum orientation orientation);
			orientation			Orientation() const;

			// TODO: Make this a virtual method, it should be one,
			// but it's not important right now. This is supposed
			// to be used in case the BScrollBar should draw part of
			// the focus indication of the target view for aesthetical
			// reasons. BScrollView will forward this method.
			status_t			SetBorderHighlighted(bool state);

	virtual void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint pt);
	virtual	void				MouseUp(BPoint pt);
	virtual	void				MouseMoved(BPoint pt, uint32 code,
									const BMessage* dragMessage);
	virtual	void				DetachedFromWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameMoved(BPoint new_position);
	virtual	void				FrameResized(float newWidth, float newHeight);

	virtual BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);

	virtual void				ResizeToPreferred();
	virtual void				GetPreferredSize(float* _width,
									float* _height);
	virtual void				MakeFocus(bool state = true);
	virtual void				AllAttached();
	virtual void				AllDetached();
	virtual status_t			GetSupportedSuites(BMessage* data);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual status_t			Perform(perform_code d, void* arg);

#if DISABLES_ON_WINDOW_DEACTIVATION
	virtual	void				WindowActivated(bool active);
#endif

private:
	class Private;
	friend class Private;

	friend status_t control_scrollbar(scroll_bar_info* info,
		BScrollBar *bar);
		// for use within the preflet

	virtual	void				_ReservedScrollBar1();
	virtual	void				_ReservedScrollBar2();
	virtual	void				_ReservedScrollBar3();
	virtual	void				_ReservedScrollBar4();

	// disabled
			BScrollBar&			operator=(const BScrollBar& other);

			bool				_DoubleArrows() const;
			void				_UpdateThumbFrame();
			float				_ValueFor(BPoint where) const;
			int32				_ButtonFor(BPoint where) const;
			BRect				_ButtonRectFor(int32 button) const;
			void				_UpdateTargetValue(BPoint where);
			void				_UpdateArrowButtons();
			void				_DrawDisabledBackground(BRect area,
									const rgb_color& light,
									const rgb_color& dark,
									const rgb_color& fill);
			void				_DrawArrowButton(int32 direction,
									bool doubleArrows, BRect frame,
									const BRect& updateRect,
									bool enabled, bool down);

			BSize				_MinSize() const;

private:
			float				fMin;
			float				fMax;
			float				fSmallStep;
			float				fLargeStep;
			float				fValue;
			float				fProportion;
			BView*				fTarget;
			orientation			fOrientation;
			char*				fTargetName;

			Private*			fPrivateData;

			uint32				_reserved[3];
};

#endif	// _SCROLL_BAR_H
