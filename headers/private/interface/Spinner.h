/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _SPINNER_H
#define _SPINNER_H


#include <AbstractSpinner.h>


class BSpinner : public BAbstractSpinner {
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

	virtual	void				Increment();
	virtual	void				Decrement();

	virtual	status_t			GetSupportedSuites(BMessage* message);

	virtual	void				AttachedToWindow();

	virtual	void				SetEnabled(bool enable);

			int32				MaxValue() const { return fMaxValue; }
	virtual	void				SetMaxValue(int32 max);

			int32				MinValue() const { return fMinValue; }
	virtual	void				SetMinValue(int32 min);

			void				Range(int32* min, int32* max);
	virtual	void				SetRange(int32 min, int32 max);

			int32				Value() const { return fValue; };
	virtual	void				SetValue(int32 value);
	virtual	void				SetValueFromText();

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

private:
			void				_InitObject();

			int32				fMinValue;
			int32				fMaxValue;
			int32				fValue;

	// FBC padding
			uint32				_reserved[20];
};


#endif	// _SPINNER_H
