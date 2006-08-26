/*
 * Copyright 2001-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen (xlr8@tref.nl)
 */
#ifndef _STRING_VIEW_H
#define _STRING_VIEW_H


#include <BeBuild.h>
#include <View.h>


class BStringView : public BView{
	public:
							BStringView(BRect bounds, const char* name,
								const char* text,
								uint32 resizeFlags = B_FOLLOW_LEFT | B_FOLLOW_TOP,
								uint32 flags = B_WILL_DRAW);
							BStringView(BMessage* data);
		virtual 			~BStringView();

		static BArchivable*	Instantiate(BMessage* data);
		virtual	status_t	Archive(BMessage* data, bool deep = true) const;

		void				SetText(const char* text);
		const char*			Text() const;
		void				SetAlignment(alignment flag);
		alignment			Alignment() const;

		virtual	void		AttachedToWindow();
		virtual	void		Draw(BRect bounds);

		virtual void		MessageReceived(BMessage* message);
		virtual	void		MouseDown(BPoint point);
		virtual	void		MouseUp(BPoint point);
		virtual	void		MouseMoved(BPoint point, uint32 transit,
								const BMessage* dragMessage);
		virtual	void		DetachedFromWindow();
		virtual	void		FrameMoved(BPoint newPosition);
		virtual	void		FrameResized(float newWidth, float newHeight);

		virtual BHandler*	ResolveSpecifier(BMessage* msg, int32 index,
								BMessage* specifier, int32 form,
								const char* property);

		virtual void		ResizeToPreferred();
		virtual void		GetPreferredSize(float* _width, float* _height);
		virtual void		MakeFocus(bool state = true);
		virtual void		AllAttached();
		virtual void		AllDetached();
		virtual status_t	GetSupportedSuites(BMessage* data);

		virtual	BSize		MaxSize();

	private:
		virtual status_t	Perform(perform_code d, void* arg);
		virtual	void		_ReservedStringView1();
		virtual	void		_ReservedStringView2();
		virtual	void		_ReservedStringView3();

		BStringView	&operator=(const BStringView&);

		char*		fText;
		alignment	fAlign;
		uint32		_reserved[3];
};

#endif	// _STRING_VIEW_H
