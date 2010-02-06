/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _STATUS_BAR_H
#define _STATUS_BAR_H


#include <String.h>
#include <View.h>


class BStatusBar : public BView {
public:
								BStatusBar(BRect frame, const char* name,
									const char* label = NULL,
									const char* trailingLabel = NULL);
								BStatusBar(const char* name,
									const char* label = NULL,
									const char* trailingLabel = NULL);
								BStatusBar(BMessage* archive);
	virtual						~BStatusBar();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				AllAttached();
	virtual	void				AllDetached();

	virtual	void				WindowActivated(bool state);
	virtual	void				MakeFocus(bool focus = true);

	virtual	void				GetPreferredSize(float* _width,
									float* _height);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	void				ResizeToPreferred();
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);

	virtual	void				Draw(BRect updateRect);

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

	// BStatusBar
	virtual	void				SetBarColor(rgb_color color);
	virtual	void				SetBarHeight(float height);

	virtual	void				SetText(const char* string);
	virtual	void				SetTrailingText(const char* string);
	virtual	void				SetMaxValue(float max);

	virtual	void				Update(float delta, const char* text = NULL,
									const char* trailingText = NULL);
	virtual	void				Reset(const char* label = NULL,
									const char* trailingLabel = NULL);
	virtual	void				SetTo(float value, const char* text = NULL,
									const char* trailingText = NULL);

			float				CurrentValue() const;
			float				MaxValue() const;
			rgb_color			BarColor() const;
			float				BarHeight() const;

			const char*			Text() const;
			const char*			TrailingText() const;
			const char*			Label() const;
			const char*			TrailingLabel() const;

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

	virtual	status_t			Perform(perform_code d, void* arg);

private:
	// FBC memebers
	virtual	void				_ReservedStatusBar2();
	virtual	void				_ReservedStatusBar3();
	virtual	void				_ReservedStatusBar4();

			BStatusBar&			operator=(const BStatusBar& other);

			void				_InitObject();
			void				_SetTextData(BString& text, const char* string,
									const BString& combineWith,
									bool rightAligned);
			BRect				_BarFrame(const font_height* fontHeight
									= NULL) const;
			float				_BarPosition(const BRect& barFrame) const;
			bool				_HasText() const;

			BString				fLabel;
			BString				fTrailingLabel;
			BString				fText;
			BString				fTrailingText;
			float				fMax;
			float				fCurrent;
			float				fBarHeight;
			float				fTextDivider;
			rgb_color			fBarColor;
			bool				fCustomBarHeight;

			uint32				_reserved[5];
};

#endif	// _STATUS_BAR_H
