/*
 * Copyright 2004 DarkWyrm <darkwyrm@earthlink.net>
 * Copyright 2013 FeemanLou
 * Copyright 2014-2015 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT license.
 *
 * Originally written by DarkWyrm <darkwyrm@earthlink.net>
 * Updated by FreemanLou as part of Google GCI 2013
 *
 * Authors:
 *		DarkWyrm, darkwyrm@earthlink.net
 *		FeemanLou
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _ABSTRACT_SPINNER_H
#define _ABSTRACT_SPINNER_H


#include <Control.h>


typedef enum {
	SPINNER_BUTTON_HORIZONTAL_ARROWS,
	SPINNER_BUTTON_VERTICAL_ARROWS,
	SPINNER_BUTTON_PLUS_MINUS
} spinner_button_style;


class BTextView;
class SpinnerButton;
class SpinnerTextView;


/*!	BAbstractSpinner provides an input whose value can be nudged up or down
	by way of two small buttons on the right.
*/
class BAbstractSpinner : public BControl {
public:
								BAbstractSpinner(BRect frame, const char* name,
									const char* label, BMessage* message,
									uint32 resizingMode = B_FOLLOW_LEFT_TOP,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
								BAbstractSpinner(const char* name, const char* label,
									BMessage* message,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
								BAbstractSpinner(BMessage* data);
	virtual						~BAbstractSpinner();

	static	BArchivable*		Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data, bool deep = true) const;

	virtual	status_t			GetSupportedSuites(BMessage* message);
	virtual	BHandler*			ResolveSpecifier(BMessage* message, int32 index,
									BMessage* specifier, int32 form,
									const char* property);

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);
	virtual	void				ValueChanged();

	virtual	void				Decrement() = 0;
	virtual	void				Increment() = 0;
	virtual	void				MakeFocus(bool focus = true);
	virtual	void				ResizeToPreferred();
	virtual	void				SetFlags(uint32 flags);
	virtual	void				WindowActivated(bool active);

			alignment			Alignment() const { return fAlignment; };
	virtual	void				SetAlignment(alignment align);

	spinner_button_style		ButtonStyle() const { return fButtonStyle; };
	virtual	void				SetButtonStyle(spinner_button_style buttonStyle);

			float				Divider() const { return fDivider; };
	virtual	void				SetDivider(float position);

	virtual	void				SetEnabled(bool enable);

	virtual	void				SetLabel(const char* label);

	virtual	void				SetValueFromText() = 0;

			bool				IsDecrementEnabled() const;
	virtual	void				SetDecrementEnabled(bool enable);

			bool				IsIncrementEnabled() const;
	virtual	void				SetIncrementEnabled(bool enable);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	BAlignment			LayoutAlignment();

			BLayoutItem*		CreateLabelLayoutItem();
			BLayoutItem*		CreateTextViewLayoutItem();

			BTextView*			TextView() const;

private:
	// FBC padding
	virtual	void				_ReservedAbstractSpinner20();
	virtual	void				_ReservedAbstractSpinner19();
	virtual	void				_ReservedAbstractSpinner18();
	virtual	void				_ReservedAbstractSpinner17();
	virtual	void				_ReservedAbstractSpinner16();
	virtual	void				_ReservedAbstractSpinner15();
	virtual	void				_ReservedAbstractSpinner14();
	virtual	void				_ReservedAbstractSpinner13();
	virtual	void				_ReservedAbstractSpinner12();
	virtual	void				_ReservedAbstractSpinner11();
	virtual	void				_ReservedAbstractSpinner10();
	virtual	void				_ReservedAbstractSpinner9();
	virtual	void				_ReservedAbstractSpinner8();
	virtual	void				_ReservedAbstractSpinner7();
	virtual	void				_ReservedAbstractSpinner6();
	virtual	void				_ReservedAbstractSpinner5();
	virtual	void				_ReservedAbstractSpinner4();
	virtual	void				_ReservedAbstractSpinner3();
	virtual	void				_ReservedAbstractSpinner2();
	virtual	void				_ReservedAbstractSpinner1();

protected:
	virtual	status_t			AllArchived(BMessage* into) const;
	virtual	status_t			AllUnarchived(const BMessage* from);

	virtual	void				LayoutInvalidated(bool descendants);
	virtual	void				DoLayout();

private:
	class LabelLayoutItem;
	class TextViewLayoutItem;
	struct LayoutData;

	friend class SpinnerButton;
	friend class SpinnerTextView;

	friend class LabelLayoutItem;
	friend class TextViewLayoutItem;
	friend struct LayoutData;

			void				_DrawLabel(BRect updateRect);
			void				_DrawTextView(BRect updateRect);
			void				_InitObject();
			void				_LayoutTextView();
			void				_UpdateFrame();
			void				_UpdateTextViewColors(bool enable);
			void				_ValidateLayoutData();

			BAbstractSpinner&	operator=(const BAbstractSpinner& other);

			alignment			fAlignment;
	spinner_button_style		fButtonStyle;
			float				fDivider;

			LayoutData*			fLayoutData;

			SpinnerTextView*	fTextView;
			SpinnerButton*		fIncrement;
			SpinnerButton*		fDecrement;

	// FBC padding
			uint32				_reserved[20];
};


#endif	// _ABSTRACT_SPINNER_H
