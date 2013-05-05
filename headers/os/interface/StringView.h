/*
 * Copyright 2001-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _STRING_VIEW_H
#define _STRING_VIEW_H


#include <View.h>


class BStringView : public BView {
public:
								BStringView(BRect bounds, const char* name,
									const char* text, uint32 resizeFlags
										= B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW);
								BStringView(const char* name, const char* text,
									uint32 flags = B_WILL_DRAW);
								BStringView(BMessage* archive);
	virtual 					~BStringView();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

			void				SetText(const char* text);
			const char*			Text() const;
			void				SetAlignment(alignment flag);
			alignment			Alignment() const;

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				AllAttached();
	virtual	void				AllDetached();

	virtual	void				MakeFocus(bool state = true);

	virtual void				GetPreferredSize(float* _width,
									float* _height);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	void				ResizeToPreferred();
	virtual	BAlignment			LayoutAlignment();
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);

	virtual	void				Draw(BRect bounds);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint point);
	virtual	void				MouseUp(BPoint point);
	virtual	void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* dragMessage);

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

	virtual	void				SetFont(const BFont* font,
									uint32 mask = B_FONT_ALL);

protected:
	virtual	void				LayoutInvalidated(bool descendants = false);

private:
	// FBC padding and forbidden methods
	virtual	status_t			Perform(perform_code code, void* data);

	virtual	void				_ReservedStringView1();
	virtual	void				_ReservedStringView2();
	virtual	void				_ReservedStringView3();

			BStringView&		operator=(const BStringView& other);

private:
			BSize				_ValidatePreferredSize();

private:
			char*				fText;
			float				fStringWidth;
			alignment			fAlign;
			BSize				fPreferredSize;
};

#endif // _STRING_VIEW_H
