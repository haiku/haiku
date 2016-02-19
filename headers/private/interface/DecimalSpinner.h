/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _DECIMAL_SPINNER_H
#define _DECIMAL_SPINNER_H


#include <Spinner.h>


class BDecimalSpinner : public BAbstractSpinner {
public:
								BDecimalSpinner(BRect frame, const char* name,
									const char* label, BMessage* message,
									uint32 resizingMode = B_FOLLOW_LEFT_TOP,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
								BDecimalSpinner(const char* name, const char* label,
									BMessage* message,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
								BDecimalSpinner(BMessage* data);
	virtual						~BDecimalSpinner();

	static	BArchivable*		Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data, bool deep = true) const;

	virtual	status_t			GetSupportedSuites(BMessage* message);

	virtual	void				AttachedToWindow();

	virtual	void				Increment();
	virtual	void				Decrement();

	virtual	void				SetEnabled(bool enable);

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
	virtual	void				SetValue(int32 value);
	virtual	void				SetValue(double value);
	virtual	void				SetValueFromText();

private:
	// FBC padding
	virtual	void				_ReservedDecimalSpinner20();
	virtual	void				_ReservedDecimalSpinner19();
	virtual	void				_ReservedDecimalSpinner18();
	virtual	void				_ReservedDecimalSpinner17();
	virtual	void				_ReservedDecimalSpinner16();
	virtual	void				_ReservedDecimalSpinner15();
	virtual	void				_ReservedDecimalSpinner14();
	virtual	void				_ReservedDecimalSpinner13();
	virtual	void				_ReservedDecimalSpinner12();
	virtual	void				_ReservedDecimalSpinner11();
	virtual	void				_ReservedDecimalSpinner10();
	virtual	void				_ReservedDecimalSpinner9();
	virtual	void				_ReservedDecimalSpinner8();
	virtual	void				_ReservedDecimalSpinner7();
	virtual	void				_ReservedDecimalSpinner6();
	virtual	void				_ReservedDecimalSpinner5();
	virtual	void				_ReservedDecimalSpinner4();
	virtual	void				_ReservedDecimalSpinner3();
	virtual	void				_ReservedDecimalSpinner2();
	virtual	void				_ReservedDecimalSpinner1();

private:
			void				_InitObject();

			double				fMinValue;
			double				fMaxValue;
			double				fStep;
			double				fValue;
			uint32				fPrecision;

	// FBC padding
			uint32				_reserved[20];
};


#endif	// _DECIMAL_SPINNER_H
