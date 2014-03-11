/*
 * Copyright 2004 DarkWyrm <darkwyrm@earthlink.net>
 * Copyright 2013 FeemanLou
 * Copyright 2014 Haiku, Inc. All rights reserved.
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
#ifndef SPINNER_H
#define SPINNER_H


#include <Invoker.h>
#include <View.h>


class BTextView;
class SpinnerArrow;
class SpinnerTextView;


/*!	BSpinner provides a numeric input whose value can be nudged up or down
	by way of two small buttons on the right.
*/
class BSpinner : public BView, public BInvoker {
public:
								BSpinner(BRect frame, const char* name,
									const char* label, BMessage* message,
									uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
								BSpinner(const char* name, const char* label,
									BMessage* message,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
								BSpinner(BMessage* data);
	virtual						~BSpinner();

	static	BArchivable*		Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data, bool deep = true) const;

	virtual	status_t			GetSupportedSuites(BMessage* message);
	virtual	BHandler*			ResolveSpecifier(BMessage* message, int32 index,
									BMessage* specifier, int32 form,
									const char* property);

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);
	virtual	void				MakeFocus(bool focus = true);
	virtual	void				ResizeToPreferred();
	virtual	void				SetFlags(uint32 flags);
	virtual	void				ValueChanged();
	virtual	void				WindowActivated(bool active);

			alignment			Alignment() const { return fAlignment; };
	virtual	void				SetAlignment(alignment align);

			float				Divider() const { return fDivider; };
	virtual	void				SetDivider(float position);

			bool				IsEnabled() const { return fIsEnabled; };
	virtual	void				SetEnabled(bool enable);

			const char*			Label() const { return fLabel; };
	virtual	void				SetLabel(const char* text);

			uint32				Precision() const { return fPrecision; };
	virtual	void				SetPrecision(uint32 precision) { fPrecision = precision; };

			double				MaxValue() const { return fMaxValue; }
	virtual	void				SetMaxValue(double max);

			double				MinValue() const { return fMinValue; }
	virtual	void				SetMinValue(double min);

			void				Range(double* min, double* max);
	virtual	void				SetRange(double min, double max);

			double				Step() const { return fStep; }
	virtual	void				SetStep(double step) { fStep = step; };

			double				Value() const { return fValue; };
	virtual	void				SetValue(double value);

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
	virtual	void				_ReservedSpinner20();
	virtual	void				_ReservedSpinner19();
	virtual	void				_ReservedSpinner18();
	virtual	void				_ReservedSpinner17();
	virtual	void				_ReservedSpinner16();
	virtual	void				_ReservedSpinner15();
	virtual	void				_ReservedSpinner14();
	virtual	void				_ReservedSpinner13();
	virtual	void				_ReservedSpinner12();
	virtual	void				_ReservedSpinner11();
	virtual	void				_ReservedSpinner10();
	virtual	void				_ReservedSpinner9();
	virtual	void				_ReservedSpinner8();
	virtual	void				_ReservedSpinner7();
	virtual	void				_ReservedSpinner6();
	virtual	void				_ReservedSpinner5();
	virtual	void				_ReservedSpinner4();
	virtual	void				_ReservedSpinner3();
	virtual	void				_ReservedSpinner2();
	virtual	void				_ReservedSpinner1();

protected:
	virtual	status_t			AllArchived(BMessage* into) const;
	virtual	status_t			AllUnarchived(const BMessage* from);

	virtual	void				LayoutInvalidated(bool descendants);
	virtual	void				DoLayout();

private:
	class LabelLayoutItem;
	class TextViewLayoutItem;
	struct LayoutData;

	friend class SpinnerArrow;
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

			BSpinner&			operator=(const BSpinner& other);

			alignment			fAlignment;
			float				fDivider;
			bool				fIsEnabled;
			const char*			fLabel;
			double				fMinValue;
			double				fMaxValue;
			double				fStep;
			double				fValue;
			uint32				fPrecision;

			LayoutData*			fLayoutData;

			SpinnerTextView*	fTextView;
			SpinnerArrow*		fIncrement;
			SpinnerArrow*		fDecrement;

	// FBC padding
			uint32				_reserved[20];
};


#endif	// SPINNER_H
