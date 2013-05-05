/*
 * Copyright 2005-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BOX_H
#define _BOX_H


#include <View.h>


class BBox : public BView {
	public:
							BBox(BRect frame, const char *name = NULL,
								uint32 resizingMode = B_FOLLOW_LEFT
									| B_FOLLOW_TOP,
								uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS
									| B_NAVIGABLE_JUMP,
								border_style border = B_FANCY_BORDER);
							BBox(const char* name,
								uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS
									| B_NAVIGABLE_JUMP,
								border_style border = B_FANCY_BORDER,
								BView* child = NULL);
							BBox(border_style border, BView* child);
		virtual				~BBox();

		/* Archiving */
							BBox(BMessage* archive);
		static BArchivable*	Instantiate(BMessage* archive);
		virtual	status_t	Archive(BMessage* archive, bool deep = true) const;

		virtual	void		SetBorder(border_style border);
		border_style		Border() const;

		float				TopBorderOffset();
		BRect				InnerFrame();

		void				SetLabel(const char* string);
		status_t			SetLabel(BView* viewLabel);

		const char*			Label() const;
		BView*				LabelView() const;

		virtual	void		Draw(BRect updateRect);
		virtual	void		AttachedToWindow();
		virtual	void		DetachedFromWindow();
		virtual	void		AllAttached();
		virtual	void		AllDetached();
		virtual void		FrameResized(float width, float height);
		virtual void		MessageReceived(BMessage* message);
		virtual	void		MouseDown(BPoint point);
		virtual	void		MouseUp(BPoint point);
		virtual	void		WindowActivated(bool active);
		virtual	void		MouseMoved(BPoint point, uint32 transit,
								const BMessage* dragMessage);
		virtual	void		FrameMoved(BPoint newLocation);

		virtual BHandler*	ResolveSpecifier(BMessage* message,
								int32 index, BMessage* specifier,
								int32 what, const char* property);

		virtual void		ResizeToPreferred();
		virtual void		GetPreferredSize(float* _width, float* _height);
		virtual void		MakeFocus(bool focused = true);
		virtual status_t	GetSupportedSuites(BMessage* message);

		virtual status_t	Perform(perform_code d, void* arg);

		virtual	BSize		MinSize();
		virtual	BSize		MaxSize();
		virtual	BSize		PreferredSize();

	protected:
		virtual	void		LayoutInvalidated(bool descendants = false);
		virtual	void		DoLayout();

	private:
		struct LayoutData;

		virtual	void		_ReservedBox1();
		virtual	void		_ReservedBox2();

		BBox				&operator=(const BBox &);

		void				_InitObject(BMessage* data = NULL);
		void				_DrawPlain(BRect labelBox);
		void				_DrawFancy(BRect labelBox);
		void				_ClearLabel();

		BView*				_Child() const;
		void				_ValidateLayoutData();

		char*				fLabel;
		BRect				fBounds;
		border_style		fStyle;
		BView*				fLabelView;
		LayoutData*			fLayoutData;
};

#endif	// _BOX_H
