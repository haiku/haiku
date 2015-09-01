/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include <Spinner.h>

#include <stdint.h>
#include <stdlib.h>

#include <PropertyInfo.h>
#include <String.h>
#include <TextView.h>


static property_info sProperties[] = {
	{
		"MaxValue",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the maximum value of the spinner.",
		0,
		{ B_INT32_TYPE }
	},
	{
		"MaxValue",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the maximum value of the spinner.",
		0,
		{ B_INT32_TYPE }
	},

	{
		"MinValue",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the minimum value of the spinner.",
		0,
		{ B_INT32_TYPE }
	},
	{
		"MinValue",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the minimum value of the spinner.",
		0,
		{ B_INT32_TYPE }
	},

	{
		"Value",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the value of the spinner.",
		0,
		{ B_INT32_TYPE }
	},
	{
		"Value",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the value of the spinner.",
		0,
		{ B_INT32_TYPE }
	},

	{ 0 }
};


//	#pragma mark - BSpinner


BSpinner::BSpinner(BRect frame, const char* name, const char* label,
	BMessage* message, uint32 resizingMode, uint32 flags)
	:
	BAbstractSpinner(frame, name, label, message, resizingMode, flags)
{
	_InitObject();
}


BSpinner::BSpinner(const char* name, const char* label,
	BMessage* message, uint32 flags)
	:
	BAbstractSpinner(name, label, message, flags)
{
	_InitObject();
}


BSpinner::BSpinner(BMessage* data)
	:
	BAbstractSpinner(data)
{
	_InitObject();

	if (data->FindInt32("_max", &fMaxValue) != B_OK)
		fMinValue = INT32_MAX;

	if (data->FindInt32("_min", &fMinValue) != B_OK)
		fMinValue = INT32_MIN;

	if (data->FindInt32("_val", &fValue) != B_OK)
		fValue = 0;
}


BSpinner::~BSpinner()
{
}


BArchivable*
BSpinner::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "Spinner"))
		return new BSpinner(data);

	return NULL;
}


status_t
BSpinner::Archive(BMessage* data, bool deep) const
{
	status_t status = BAbstractSpinner::Archive(data, deep);
	data->AddString("class", "Spinner");

	if (status == B_OK)
		status = data->AddInt32("_max", fMaxValue);

	if (status == B_OK)
		status = data->AddInt32("_min", fMinValue);

	if (status == B_OK)
		status = data->AddInt32("_val", fValue);

	return status;
}


status_t
BSpinner::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/vnd.Haiku-intenger-spinner");

	BPropertyInfo prop_info(sProperties);
	message->AddFlat("messages", &prop_info);

	return BView::GetSupportedSuites(message);
}


void
BSpinner::AttachedToWindow()
{
	SetValue(fValue);

	BAbstractSpinner::AttachedToWindow();
}


void
BSpinner::Decrement()
{
	SetValue(Value() - 1);
}


void
BSpinner::Increment()
{
	SetValue(Value() + 1);
}


void
BSpinner::SetEnabled(bool enable)
{
	if (IsEnabled() == enable)
		return;

	SetIncrementEnabled(enable && Value() < fMaxValue);
	SetDecrementEnabled(enable && Value() > fMinValue);

	BAbstractSpinner::SetEnabled(enable);
}


void
BSpinner::SetMaxValue(int32 max)
{
	fMaxValue = max;
	if (fValue > fMaxValue)
		SetValue(fMaxValue);
}


void
BSpinner::SetMinValue(int32 min)
{
	fMinValue = min;
	if (fValue < fMinValue)
		SetValue(fMinValue);
}


void
BSpinner::Range(int32* min, int32* max)
{
	*min = fMinValue;
	*max = fMaxValue;
}


void
BSpinner::SetRange(int32 min, int32 max)
{
	SetMinValue(min);
	SetMaxValue(max);
}


void
BSpinner::SetValue(int32 value)
{
	// clip to range
	if (value < fMinValue)
		value = fMinValue;
	else if (value > fMaxValue)
		value = fMaxValue;

	// update the text view
	BString valueString;
	valueString << value;
	TextView()->SetText(valueString.String());

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
BSpinner::SetValueFromText()
{
	SetValue(atol(TextView()->Text()));
}


//	#pragma mark - BSpinner private methods


void
BSpinner::_InitObject()
{
	fMaxValue = INT32_MIN;
	fMinValue = INT32_MAX;
	fValue = 0;

	TextView()->SetAlignment(B_ALIGN_RIGHT);
	for (uint32 c = 0; c <= 42; c++)
		TextView()->DisallowChar(c);

	TextView()->DisallowChar(',');

	for (uint32 c = 46; c <= 47; c++)
		TextView()->DisallowChar(c);

	for (uint32 c = 58; c <= 127; c++)
		TextView()->DisallowChar(c);
}


// FBC padding

void BSpinner::_ReservedSpinner20() {}
void BSpinner::_ReservedSpinner19() {}
void BSpinner::_ReservedSpinner18() {}
void BSpinner::_ReservedSpinner17() {}
void BSpinner::_ReservedSpinner16() {}
void BSpinner::_ReservedSpinner15() {}
void BSpinner::_ReservedSpinner14() {}
void BSpinner::_ReservedSpinner13() {}
void BSpinner::_ReservedSpinner12() {}
void BSpinner::_ReservedSpinner11() {}
void BSpinner::_ReservedSpinner10() {}
void BSpinner::_ReservedSpinner9() {}
void BSpinner::_ReservedSpinner8() {}
void BSpinner::_ReservedSpinner7() {}
void BSpinner::_ReservedSpinner6() {}
void BSpinner::_ReservedSpinner5() {}
void BSpinner::_ReservedSpinner4() {}
void BSpinner::_ReservedSpinner3() {}
void BSpinner::_ReservedSpinner2() {}
void BSpinner::_ReservedSpinner1() {}
