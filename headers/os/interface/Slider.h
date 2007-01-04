/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */
#ifndef _SLIDER_H
#define _SLIDER_H


#include <BeBuild.h>
#include <Control.h>


enum hash_mark_location {
	B_HASH_MARKS_NONE = 0,
	B_HASH_MARKS_TOP = 1,
	B_HASH_MARKS_LEFT = 1,
	B_HASH_MARKS_BOTTOM = 2,
	B_HASH_MARKS_RIGHT = 2,
	B_HASH_MARKS_BOTH = 3
};

enum thumb_style {
	B_BLOCK_THUMB,
	B_TRIANGLE_THUMB
};


#define USE_OFF_SCREEN_VIEW 0


class BSlider : public BControl {
	public:
		BSlider(BRect frame, const char *name, const char *label,
			BMessage *message, int32 minValue, int32 maxValue,
			thumb_style thumbType = B_BLOCK_THUMB,
			uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_NAVIGABLE | B_WILL_DRAW | B_FRAME_EVENTS);

		BSlider(BRect frame, const char *name, const char *label,
			BMessage *message, int32 minValue, int32 maxValue,
			orientation posture, thumb_style thumbType = B_BLOCK_THUMB,
			uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_NAVIGABLE | B_WILL_DRAW | B_FRAME_EVENTS);

		BSlider(BMessage *data);
		virtual	~BSlider();

		static 	BArchivable	*Instantiate(BMessage *data);
		virtual	status_t 	Archive(BMessage *data, bool deep = true) const;
		virtual status_t	Perform(perform_code d, void *arg);

		virtual void		WindowActivated(bool state);
		virtual	void		AttachedToWindow();
		virtual	void		AllAttached();
		virtual	void		AllDetached();
		virtual	void		DetachedFromWindow();

		virtual	void		MessageReceived(BMessage *msg);
		virtual void		FrameMoved(BPoint new_position);
		virtual void		FrameResized(float w,float h);
		virtual void		KeyDown(const char * bytes, int32 n);
		virtual void		MouseDown(BPoint);
		virtual void		MouseUp(BPoint pt);
		virtual void		MouseMoved(BPoint pt, uint32 c, const BMessage *m);
		virtual	void		Pulse();

		virtual void		SetLabel(const char *label);
		virtual	void		SetLimitLabels(const char *minLabel,
								const char *maxLabel);
		const char*			MinLimitLabel() const;
		const char*			MaxLimitLabel() const;							
		virtual	void		SetValue(int32);
		virtual int32		ValueForPoint(BPoint) const;
		virtual void		SetPosition(float);
				float		Position() const;
		virtual void		SetEnabled(bool on); 
				void		GetLimits(int32 * minimum, int32 * maximum);	

		virtual	void		Draw(BRect);
		virtual void		DrawSlider();
		virtual void		DrawBar();
		virtual void		DrawHashMarks();
		virtual void		DrawThumb();
		virtual void		DrawFocusMark();
		virtual	void		DrawText();
		virtual char*		UpdateText() const;			

		virtual BRect		BarFrame() const;
		virtual BRect		HashMarksFrame() const;
		virtual BRect		ThumbFrame() const;

		virtual	void		SetFlags(uint32 flags);
		virtual	void		SetResizingMode(uint32 mode);

		virtual void		GetPreferredSize( float *width, float *height);
		virtual void		ResizeToPreferred();

		virtual status_t	Invoke(BMessage *msg=NULL);
		virtual BHandler*	ResolveSpecifier(BMessage *msg, int32 index,
								BMessage *specifier, int32 form,
								const char *property);
		virtual	status_t	GetSupportedSuites(BMessage* data);

		virtual	void		SetModificationMessage(BMessage *message);
				BMessage*	ModificationMessage() const;

		virtual void		SetSnoozeAmount(int32);
				int32		SnoozeAmount() const;

		virtual	void		SetKeyIncrementValue(int32 value);
				int32		KeyIncrementValue()	const;

		virtual	void		SetHashMarkCount(int32 count);
				int32		HashMarkCount() const;

		virtual	void		SetHashMarks(hash_mark_location where);
		hash_mark_location	HashMarks() const;

		virtual	void		SetStyle(thumb_style s);
				thumb_style	Style() const;

		virtual	void		SetBarColor(rgb_color);
				rgb_color	BarColor() const;
		virtual	void		UseFillColor(bool, const rgb_color* c=NULL);
				bool		FillColor(rgb_color*) const;

				BView*		OffscreenView() const;

				orientation	Orientation() const;
		virtual void		SetOrientation(orientation);

				float		BarThickness() const;
		virtual void		SetBarThickness(float thickness);

		virtual void		SetFont(const BFont *font, uint32 properties = B_FONT_ALL);

		virtual void		SetLimits(int32 minimum, int32 maximum);

	private:
				void		_DrawBlockThumb();
				void		_DrawTriangleThumb();

				BPoint		_Location() const;
				void		_SetLocation(BPoint p);

				float		_MinPosition() const;
				float		_MaxPosition() const;
				bool		_ConstrainPoint(BPoint& point, BPoint compare) const;

		virtual	void		_ReservedSlider5();
		virtual	void		_ReservedSlider6();
		virtual	void		_ReservedSlider7();
		virtual	void		_ReservedSlider8();
		virtual	void		_ReservedSlider9();
		virtual	void		_ReservedSlider10();
		virtual	void		_ReservedSlider11();
		virtual	void		_ReservedSlider12();

				BSlider&	operator=(const BSlider &);

				void		_InitObject();

	private:
		BMessage*			fModificationMessage;
		int32				fSnoozeAmount;

		rgb_color 			fBarColor;
		rgb_color 			fFillColor;
		bool				fUseFillColor;

		char*				fMinLimitLabel;
		char*				fMaxLimitLabel;
		char*				fUpdateText;

		int32 				fMinValue;
		int32 				fMaxValue;
		int32 				fKeyIncrementValue;

		int32 				fHashMarkCount;
		hash_mark_location 	fHashMarks;

#if USE_OFF_SCREEN_VIEW
		BBitmap*			fOffScreenBits;
		BView*				fOffScreenView;
#endif

		thumb_style			fStyle;

		BPoint 				fLocation;
		BPoint				fInitialLocation;

		orientation			fOrientation;
		float				fBarThickness;

#if USE_OFF_SCREEN_VIEW
		uint32				_reserved[7];
#else
		uint32				_reserved[9];
#endif
};

#endif	// _SLIDER_H
