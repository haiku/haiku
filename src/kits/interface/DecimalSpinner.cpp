/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include <DecimalSpinner.h>

#include <stdio.h>
#include <stdlib.h>

#include <PropertyInfo.h>
#include <TextView.h>


static double
roundTo(double value, uint32 n)
{
	return floor(value * pow(10.0, n) + 0.5) / pow(10.0, n);
}


static property_info sProperties[] = {
	{
		"MaxValue",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the maximum value of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},
	{
		"MaxValue",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the maximum value of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},

	{
		"MinValue",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the minimum value of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},
	{
		"MinValue",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the minimum value of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},

	{
		"Precision",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the number of decimal places of precision of the spinner.",
		0,
		{ B_UINT32_TYPE }
	},
	{
		"Precision",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the number of decimal places of precision of the spinner.",
		0,
		{ B_UINT32_TYPE }
	},

	{
		"Step",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the step size of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},
	{
		"Step",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the step size of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},

	{
		"Value",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the value of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},
	{
		"Value",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the value of the spinner.",
		0,
		{ B_DOUBLE_TYPE }
	},

	{ 0 }
};


//	#pragma mark - BDecimalSpinner


BDecimalSpinner::BDecimalSpinner(BRect frame, const char* name,
	const char* label, BMessage* message, uint32 resizingMode, uint32 flags)
	:
	BAbstractSpinner(frame, name, label, message, resizingMode, flags)
{
	_InitObject();
}


BDecimalSpinner::BDecimalSpinner(const char* name, const char* label,
	BMessage* message, uint32 flags)
	:
	BAbstractSpinner(name, label, message, flags)
{
	_InitObject();
}


BDecimalSpinner::BDecimalSpinner(BMessage* data)
	:
	BAbstractSpinner(data)
{
	_InitObject();

	if (data->FindDouble("_max", &fMaxValue) != B_OK)
		fMinValue = 100.0;

	if (data->FindDouble("_min", &fMinValue) != B_OK)
		fMinValue = 0.0;

	if (data->FindUInt32("_precision", &fPrecision) != B_OK)
		fPrecision = 2;

	if (data->FindDouble("_step", &fStep) != B_OK)
		fStep = 1.0;

	if (data->FindDouble("_val", &fValue) != B_OK)
		fValue = 0.0;
}


BDecimalSpinner::~BDecimalSpinner()
{
}


BArchivable*
BDecimalSpinner::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "DecimalSpinner"))
		return new BDecimalSpinner(data);

	return NULL;
}


status_t
BDecimalSpinner::Archive(BMessage* data, bool deep) const
{
	status_t status = BAbstractSpinner::Archive(data, deep);
	data->AddString("class", "DecimalSpinner");

	if (status == B_OK)
		status = data->AddDouble("_max", fMaxValue);

	if (status == B_OK)
		status = data->AddDouble("_min", fMinValue);

	if (status == B_OK)
		status = data->AddUInt32("_precision", fPrecision);

	if (status == B_OK)
		status = data->AddDouble("_step", fStep);

	if (status == B_OK)
		status = data->AddDouble("_val", fValue);

	return status;
}


status_t
BDecimalSpinner::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/vnd.Haiku-decimal-spinner");

	BPropertyInfo prop_info(sProperties);
	message->AddFlat("messages", &prop_info);

	return BView::GetSupportedSuites(message);
}


void
BDecimalSpinner::AttachedToWindow()
{
	SetValue(fValue);

	BAbstractSpinner::AttachedToWindow();
}


void
BDecimalSpinner::Decrement()
{
	SetValue(Value() - Step());
}


void
BDecimalSpinner::Increment()
{
	SetValue(Value() + Step());
}


void
BDecimalSpinner::SetEnabled(bool enable)
{
	if (IsEnabled() == enable)
		return;

	SetIncrementEnabled(enable && Value() < fMaxValue);
	SetDecrementEnabled(enable && Value() > fMinValue);

	BAbstractSpinner::SetEnabled(enable);
}


void
BDecimalSpinner::SetMaxValue(double max)
{
	fMaxValue = max;
	if (fValue > fMaxValue)
		SetValue(fMaxValue);
}


void
BDecimalSpinner::SetMinValue(double min)
{
	fMinValue = min;
	if (fValue < fMinValue)
		SetValue(fMinValue);
}


void
BDecimalSpinner::Range(double* min, double* max)
{
	*min = fMinValue;
	*max = fMaxValue;
}


void
BDecimalSpinner::SetRange(double min, double max)
{
	SetMinValue(min);
	SetMaxValue(max);
}


void
BDecimalSpinner::SetValue(int32 value)
{
	SetValue((double)value);
}


void
BDecimalSpinner::SetValue(double value)
{
	// clip to range
	if (value < fMinValue)
		value = fMinValue;
	else if (value > fMaxValue)
		value = fMaxValue;

	// update the text view
	char* format;
	asprintf(&format, "%%.%" B_PRId32 "f", fPrecision);
	char* valueString;
	asprintf(&valueString, format, value);
	TextView()->SetText(valueString);
	free(format);
	free(valueString);

	// update the up and down arrows
	SetIncrementEnabled(IsEnabled() && value < fMaxValue);
	SetDecrementEnabled(IsEnabled() && value > fMinValue);

	if (value == fValue)
		return;

	fValue = value;
	ValueChanged();

	Invoke();
	Invalidate();
}


void
BDecimalSpinner::SetValueFromText()
{
	SetValue(roundTo(atof(TextView()->Text()), Precision()));
}


//	#pragma mark - BDecimalSpinner private methods


void
BDecimalSpinner::_InitObject()
{
	fMaxValue = 100.0;
	fMinValue = 0.0;
	fPrecision = 2;
	fStep = 1.0;
	fValue = 0.0;

	TextView()->SetAlignment(B_ALIGN_RIGHT);
	for (uint32 c = 0; c <= 42; c++)
		TextView()->DisallowChar(c);

	TextView()->DisallowChar('/');
	for (uint32 c = 58; c <= 127; c++)
		TextView()->DisallowChar(c);
}


// FBC padding

void BDecimalSpinner::_ReservedDecimalSpinner20() {}
void BDecimalSpinner::_ReservedDecimalSpinner19() {}
void BDecimalSpinner::_ReservedDecimalSpinner18() {}
void BDecimalSpinner::_ReservedDecimalSpinner17() {}
void BDecimalSpinner::_ReservedDecimalSpinner16() {}
void BDecimalSpinner::_ReservedDecimalSpinner15() {}
void BDecimalSpinner::_ReservedDecimalSpinner14() {}
void BDecimalSpinner::_ReservedDecimalSpinner13() {}
void BDecimalSpinner::_ReservedDecimalSpinner12() {}
void BDecimalSpinner::_ReservedDecimalSpinner11() {}
void BDecimalSpinner::_ReservedDecimalSpinner10() {}
void BDecimalSpinner::_ReservedDecimalSpinner9() {}
void BDecimalSpinner::_ReservedDecimalSpinner8() {}
void BDecimalSpinner::_ReservedDecimalSpinner7() {}
void BDecimalSpinner::_ReservedDecimalSpinner6() {}
void BDecimalSpinner::_ReservedDecimalSpinner5() {}
void BDecimalSpinner::_ReservedDecimalSpinner4() {}
void BDecimalSpinner::_ReservedDecimalSpinner3() {}
void BDecimalSpinner::_ReservedDecimalSpinner2() {}
void BDecimalSpinner::_ReservedDecimalSpinner1() {}
