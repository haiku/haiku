/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_TEXT_CONTROL_H
#define	_TEXT_CONTROL_H


#include <Control.h>
#include <TextView.h>

class _BTextInput_;


class BTextControl : public BControl {
	public:
							BTextControl(BRect frame, const char* name,
								const char* label, const char* initialText,
								BMessage* message,
								uint32 resizeMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
								uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 
		virtual				~BTextControl();

							BTextControl(BMessage* archive);
		static BArchivable*	Instantiate(BMessage* archive);
		virtual	status_t	Archive(BMessage* archive, bool deep = true) const;

		virtual	void		SetText(const char* text);
		const char*			Text() const;

		virtual	void		SetValue(int32 value);
		virtual	status_t	Invoke(BMessage* message = NULL);

		BTextView*			TextView() const;

		virtual	void		SetModificationMessage(BMessage* message);
		BMessage*			ModificationMessage() const;

		virtual	void		SetAlignment(alignment label, alignment text);
		void				GetAlignment(alignment* _label, alignment* _text) const;
		virtual	void		SetDivider(float position);
		float				Divider() const;

		virtual	void		Draw(BRect updateRect);
		virtual	void		MouseDown(BPoint where);
		virtual	void		AttachedToWindow();
		virtual	void		MakeFocus(bool focus = true);
		virtual	void		SetEnabled(bool enabled);
		virtual	void		FrameMoved(BPoint newPosition);
		virtual	void		FrameResized(float newWidth, float newHeight);
		virtual	void		WindowActivated(bool active);

		virtual	void		GetPreferredSize(float* _width, float* _height);
		virtual	void		ResizeToPreferred();

		virtual void		MessageReceived(BMessage* message);
		virtual BHandler*	ResolveSpecifier(BMessage* message, int32 index,
								BMessage* specifier, int32 what,
								const char* property);

		virtual	void		MouseUp(BPoint point);
		virtual	void		MouseMoved(BPoint point, uint32 transit,
								const BMessage* dragMessage);
		virtual	void		DetachedFromWindow();

		virtual void		AllAttached();
		virtual void		AllDetached();
		virtual status_t	GetSupportedSuites(BMessage* data);
		virtual void		SetFlags(uint32 flags);

	private:
		friend class _BTextInput_;

		virtual status_t	Perform(perform_code d, void* arg);

		virtual	void		_ReservedTextControl1();
		virtual	void		_ReservedTextControl2();
		virtual	void		_ReservedTextControl3();
		virtual	void		_ReservedTextControl4();

		BTextControl&		operator=(const BTextControl& other);

		void				_CommitValue();
		void				_InitData(const char* label, const char* initialText,
								BMessage* archive = NULL);

	private:
		_BTextInput_*		fText;
		char*				fLabel;
		BMessage*			fModificationMessage;
		alignment			fLabelAlign;
		float				fDivider;
		float				fPreviousWidth;
		float				fPreviousHeight;

		uint32				_reserved[6];

		bool				fClean;
		bool				fSkipSetFlags;

		bool				_reserved1;
		bool				_reserved2;
};

#endif	// _TEXT_CONTROL_H
