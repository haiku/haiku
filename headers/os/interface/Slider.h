/*
 * Copyright 2001-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SLIDER_H
#define _SLIDER_H


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


class BSlider : public BControl {
public:
								BSlider(BRect frame, const char* name,
									const char* label, BMessage* message,
									int32 minValue, int32 maxValue,
									thumb_style thumbType = B_BLOCK_THUMB,
									uint32 resizingMode
										= B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_NAVIGABLE | B_WILL_DRAW
										| B_FRAME_EVENTS);

								BSlider(BRect frame, const char* name,
									const char* label, BMessage* message,
									int32 minValue, int32 maxValue,
									orientation posture,
									thumb_style thumbType = B_BLOCK_THUMB,
									uint32 resizingMode
										= B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_NAVIGABLE | B_WILL_DRAW
										| B_FRAME_EVENTS);

								BSlider(const char* name, const char* label,
									BMessage* message, int32 minValue,
									int32 maxValue, orientation posture,
									thumb_style thumbType = B_BLOCK_THUMB,
									uint32 flags = B_NAVIGABLE | B_WILL_DRAW
										| B_FRAME_EVENTS);

								BSlider(BMessage* archive);
	virtual						~BSlider();

	static 	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t 			Archive(BMessage* archive,
									bool deep = true) const;
	virtual status_t			Perform(perform_code code, void* data);

	virtual void				WindowActivated(bool state);
	virtual	void				AttachedToWindow();
	virtual	void				AllAttached();
	virtual	void				AllDetached();
	virtual	void				DetachedFromWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual void				FrameMoved(BPoint newPosition);
	virtual void				FrameResized(float width, float height);
	virtual void				KeyDown(const char* bytes, int32 numBytes);
	virtual void				KeyUp(const char* bytes, int32 numBytes);
	virtual void				MouseDown(BPoint point);
	virtual void				MouseUp(BPoint point);
	virtual void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				Pulse();

	virtual void				SetLabel(const char* label);
	virtual	void				SetLimitLabels(const char* minLabel,
									const char* maxLabel);
			const char*			MinLimitLabel() const;
			const char*			MaxLimitLabel() const;
	virtual	void				SetValue(int32);
	virtual int32				ValueForPoint(BPoint) const;
	virtual void				SetPosition(float);
			float				Position() const;
	virtual void				SetEnabled(bool on);
			void				GetLimits(int32* minimum,
									int32* maximum) const;

	virtual	void				Draw(BRect);
	virtual void				DrawSlider();
	virtual void				DrawBar();
	virtual void				DrawHashMarks();
	virtual void				DrawThumb();
	virtual void				DrawFocusMark();
	virtual	void				DrawText();
	virtual const char*			UpdateText() const;
			void				UpdateTextChanged();

	virtual BRect				BarFrame() const;
	virtual BRect				HashMarksFrame() const;
	virtual BRect				ThumbFrame() const;

	virtual	void				SetFlags(uint32 flags);
	virtual	void				SetResizingMode(uint32 mode);

	virtual void				GetPreferredSize(float* _width,
									float* _height);
	virtual void				ResizeToPreferred();

	virtual status_t			Invoke(BMessage* message = NULL);
	virtual BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

	virtual	void				SetModificationMessage(BMessage* message);
			BMessage*			ModificationMessage() const;

	virtual void				SetSnoozeAmount(int32);
			int32				SnoozeAmount() const;

	virtual	void				SetKeyIncrementValue(int32 value);
			int32				KeyIncrementValue()	const;

	virtual	void				SetHashMarkCount(int32 count);
			int32				HashMarkCount() const;

	virtual	void				SetHashMarks(hash_mark_location where);
			hash_mark_location	HashMarks() const;

	virtual	void				SetStyle(thumb_style style);
			thumb_style			Style() const;

	virtual	void				SetBarColor(rgb_color color);
			rgb_color			BarColor() const;
	virtual	void				UseFillColor(bool useFill,
									const rgb_color* color = NULL);
			bool				FillColor(rgb_color* color) const;

			BView*				OffscreenView() const;

			orientation			Orientation() const;
	virtual void				SetOrientation(orientation);

			float				BarThickness() const;
	virtual void				SetBarThickness(float thickness);

	virtual void				SetFont(const BFont* font,
									uint32 properties = B_FONT_ALL);

	virtual void				SetLimits(int32 minimum, int32 maximum);

	virtual	float				MaxUpdateTextWidth();

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

protected:
	virtual	void				LayoutInvalidated(bool descendants);

private:
			void				_DrawBlockThumb();
			void				_DrawTriangleThumb();

			BPoint				_Location() const;
			void				_SetLocationForValue(int32 value);

			float				_MinPosition() const;
			float				_MaxPosition() const;
			bool				_ConstrainPoint(BPoint& point,
									BPoint compare) const;

			BSize				_ValidateMinSize();

			void				_InitBarColor();
			void				_InitObject();

private:
	// FBC padding and forbidden methods
	virtual	void				_ReservedSlider6();
	virtual	void				_ReservedSlider7();
	virtual	void				_ReservedSlider8();
	virtual	void				_ReservedSlider9();
	virtual	void				_ReservedSlider10();
	virtual	void				_ReservedSlider11();
	virtual	void				_ReservedSlider12();

			BSlider&			operator=(const BSlider& other);

private:
			BMessage*			fModificationMessage;
			int32				fSnoozeAmount;

			rgb_color 			fBarColor;
			rgb_color 			fFillColor;
			bool				fUseFillColor;

			char*				fMinLimitLabel;
			char*				fMaxLimitLabel;
			const char*			fUpdateText;

			int32 				fMinValue;
			int32 				fMaxValue;
			int32 				fKeyIncrementValue;

			int32 				fHashMarkCount;
			hash_mark_location 	fHashMarks;

			BBitmap*			fOffScreenBits;
			BView*				fOffScreenView;

			thumb_style			fStyle;

			BPoint 				fLocation;
			BPoint				fInitialLocation;

			orientation			fOrientation;
			float				fBarThickness;

			BSize				fMinSize;

			float				fMaxUpdateTextWidth;

			uint32				_reserved[4];
};

#endif	// _SLIDER_H
