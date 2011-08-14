/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// NumericValControl.cpp
// e.moon 30jan99


#include "NumericValControl.h"
#include "ValControlDigitSegment.h"
#include "ValCtrlLayoutEntry.h"

#include <Debug.h>
#include <MediaKit.h>
#include <ParameterWeb.h>

#include <cstdio>
#include <stdlib.h>
#include <string.h>

__USE_CORTEX_NAMESPACE


NumericValControl::NumericValControl(BRect frame, const char* name, BContinuousParameter* param,
	uint16 wholeDigits, uint16 fractionalDigits, align_mode alignMode, align_flags alignFlags)
	: ValControl(frame, name, 0, 0, alignMode, alignFlags, UPDATE_ASYNC, false),
	fParam(param),
	fWholeDigits(wholeDigits),
	fFractionalDigits(fractionalDigits)
{
	
	// ensure that the parameter represents a continuous value
	ASSERT(fParam->ValueType() == B_FLOAT_TYPE ||
		fParam->ValueType() == B_DOUBLE_TYPE 		
		/*||  unimplemented so far
		m_pParam->ValueType() == B_INT8_TYPE ||
		m_pParam->ValueType() == B_UINT8_TYPE ||
		m_pParam->ValueType() == B_INT16_TYPE ||
		m_pParam->ValueType() == B_UINT16_TYPE ||
		m_pParam->ValueType() == B_INT32_TYPE ||
		m_pParam->ValueType() == B_UINT32_TYPE ||
		m_pParam->ValueType() == B_INT64_TYPE ||
		m_pParam->ValueType() == B_UINT64_TYPE*/ );
	
	initConstraintsFromParam();
	initSegments();
	mediaParameterChanged();
}


NumericValControl::NumericValControl(BRect frame, const char* name, BMessage* message,
	uint16 wholeDigits, uint16 fractionalDigits, bool negativeVisible,
	align_mode alignMode, align_flags alignFlags)
	: ValControl(frame, name, 0, message, alignMode, alignFlags, UPDATE_ASYNC, false),
	fParam(0),
	fWholeDigits(wholeDigits),
	fFractionalDigits(fractionalDigits)
{
	_SetDefaultConstraints(negativeVisible);
	initSegments();
}


NumericValControl::~NumericValControl()
{
}


BContinuousParameter*
NumericValControl::param() const
{
	return fParam;
}


void
NumericValControl::initSegments()
{
	ASSERT(fWholeDigits);
	
	bool negativeVisible = fMinFixed < 0.0;

	// *** SEGMENT DIVISION NEEDS TO BE CONFIGURABLE +++++
	// GOOD 23aug99
	// init segments:
	_Add(new ValControlDigitSegment(fWholeDigits, 0, negativeVisible), RIGHT_MOST);

	if (fFractionalDigits)
		_Add(ValCtrlLayoutEntry::decimalPoint, RIGHT_MOST);

	for (int n = 0; n < fFractionalDigits; ++n)
		_Add(new ValControlDigitSegment(1, (-1)-n, false, ValControlDigitSegment::ZERO_FILL),
			RIGHT_MOST);
//		add(new ValControlDigitSegment(fFractionalDigits, -fFractionalDigits,
//			false, ValControlDigitSegment::ZERO_FILL),
//			RIGHT_MOST);
//	}

//	// +++++ individual-segment test
//	
//	for(int n = 0; n < fWholeDigits; ++n)
//		add(
//			new ValControlDigitSegment(1, fWholeDigits-n, bNegativeCapable),
//			RIGHT_MOST);
//
//	if(fFractionalDigits)
//		add(ValCtrlLayoutEntry::decimalPoint, RIGHT_MOST);
//		
//	for(int n = 0; n < fFractionalDigits; ++n)
//		add(
//			new ValControlDigitSegment(1, (-1)-n, false, ValControlDigitSegment::ZERO_FILL),
//			RIGHT_MOST);
}


void
NumericValControl::initConstraintsFromParam()
{
	ASSERT(fParam);
	
	printf("NumericValControl::initConstraintsFromParam():\n  ");
	int r;
	float fFactor;
	float fOffset;
	fParam->GetResponse(&r, &fFactor, &fOffset);
	switch (r) {
		case BContinuousParameter::B_LINEAR:
			printf("Linear");
			break;
			
		case BContinuousParameter::B_POLYNOMIAL:
			printf("Polynomial");
			break;
			
		case BContinuousParameter::B_EXPONENTIAL:
			printf("Exponential");
			break;
			
		case BContinuousParameter::B_LOGARITHMIC:
			printf("Logarithmic");
			break;
		
		default:
			printf("unknown (?)");
	}
	printf(" response; factor %.2f, offset %.2f\n", fFactor, fOffset);

	setConstraints(fParam->MinValue(), fParam->MaxValue());

	// step not yet supported +++++ 19sep99
	//		
	float fStep = fParam->ValueStep();
	
	printf("  min value: %f\n", fParam->MinValue());
	printf("  max value: %f\n", fParam->MaxValue());
	printf("  value step: %f\n\n", fStep);
}


//! Value constraints (by default, the min/max allowed by the
// setting of nWholeDigits, nFractionalDigits, and bNegativeCapable)
void
NumericValControl::getConstraints(double* outMinValue, double* outMaxValue)
{
	double factor = pow(10, -fFractionalDigits);

	*outMinValue = (double)fMinFixed * factor;
	*outMaxValue = (double)fMaxFixed * factor;
}

	
status_t
NumericValControl::setConstraints(double minValue, double maxValue)
{	
	if (maxValue < minValue)
		return B_BAD_VALUE;

	double factor = pow(10, fFractionalDigits);

	fMinFixed = (minValue < 0.0) ?
		(int64)floor(minValue * factor) :
		(int64)ceil(minValue * factor);
	
	fMaxFixed = (maxValue < 0.0) ?
		(int64)floor(maxValue * factor) :
		(int64)ceil(maxValue * factor);

	return B_OK;
}


//! Fetches the current value (calculated on the spot from each
// segment.) 
double
NumericValControl::value() const
{
//	double acc = 0.0;
//
//	// walk segments, adding the value of each
//	for(int n = CountEntries(); n > 0; --n) {
//		const ValCtrlLayoutEntry& e = entryAt(n-1);
//		if(e.type == ValCtrlLayoutEntry::SEGMENT_ENTRY) {
//			const ValControlDigitSegment* digitSegment =
//				dynamic_cast<ValControlDigitSegment*>(e.pView);
//			ASSERT(digitSegment);
//
//			PRINT((
//				"\t...segment %d: %d digits at %d: %Ld\n",
//				n-1,
//				digitSegment->digitCount(),
//				digitSegment->scaleFactor(),
//				digitSegment->value()));
//
//			acc += ((double)digitSegment->value() *
//				pow(
//					10,
//					digitSegment->scaleFactor()));
//				
//			PRINT((
//				"\t-> %.12f\n\n", acc));
//		}
//	}
//	
//	return acc;

	double ret = (double)_ValueFixed() / pow(10, fFractionalDigits);

//	PRINT((
//		"### NumericValControl::value(): %.12f\n", ret));

	return ret;
}

	
//! Set the displayed value (and, if setParam is true, the
// linked parameter.)  The value will be constrained if necessary.
void
NumericValControl::setValue(double value, bool setParam)
{

//	PRINT((
//		"### NumericValControl::setValue(%.12f)\n", value));

	// round to displayed precision
	double scaleFactor = pow(10, fFractionalDigits);

	int64 fixed = (int64)(value * scaleFactor);
	double junk = (value * scaleFactor) - (double)fixed;
	
//	PRINT((
//		" :  junk == %.12f\n", junk));
	
	if (value < 0.0) {
		if (junk * scaleFactor < 0.5)
			fixed--;
	} else {
		if (junk * scaleFactor >= 0.5)
			fixed++;
	}
	
	value = (double)fixed / scaleFactor;
	
//	PRINT((
//		" -> %.12f, %Ld\n", value, fixed));

	_SetValueFixed(fixed);
		
	// notify target 
	Invoke();

	// set parameter
	if (setParam && fParam)
		updateParameter(value);
	
	// +++++ redraw?
}

//double NumericValControl::value() const {
//	return m_dfValue;
//	/*
//	double dfCur = 0.0;
//	
//	// sum the values of all segments
//	for(int nIndex = CountEntries()-1; nIndex >= 0; nIndex--) {
//		if(entryAt(nIndex).type == ValCtrlLayoutEntry::SEGMENT_ENTRY)	{
//			const ValControlDigitSegment* pSeg =
//				dynamic_cast<ValControlDigitSegment*>(entryAt(nIndex).pView);
//			ASSERT(pSeg);
//			dfCur += pSeg->value();
//		}
//	}
//	
//	return dfCur;*/
//}
//
//void NumericValControl::setValue(double dfValue, bool bSetParam) {
//
//	printf("setValue(%.12f)\n", dfValue);
//
//
//	// constrain
//	if(dfValue > m_maxValue)
//		dfValue = m_maxValue;
//	else if(dfValue < m_minValue)
//		dfValue = m_minValue;
//
//	// +++++ round to displayed precision
//
//	// set value
//	m_dfValue = dfValue;
//
//	// set parameter
//	if(bSetParam && fParam)
//		updateParameter();
//	
//	// notify target (ugh.  what if the target called this? +++++)
//	Invoke();
//	
//	// hand value to each segment
//	for(int nIndex = 0; nIndex < CountEntries(); nIndex++) {
//		if(entryAt(nIndex).type == ValCtrlLayoutEntry::SEGMENT_ENTRY)	{
//			const ValControlDigitSegment* pSeg =
//				dynamic_cast<ValControlDigitSegment*>(entryAt(nIndex).pView);
//			ASSERT(pSeg);
//			pSeg->setValue(!nIndex ? m_dfValue : fabs(m_dfValue));
//		}
//	}
//}

// ---------------------------------------------------------------- //
// segment interface
// ---------------------------------------------------------------- //

//void NumericValControl::offsetValue(double dfDelta) {
////	printf("offset: %lf\t", dfDelta);
//	setValue(value() + dfDelta, true);	
////	printf("%lf\n", value());
//}

// 18sep99: new segment interface.  'offset' is given
// in the segment's units.

void
NumericValControl::offsetSegmentValue(ValControlDigitSegment* segment,
	int64 offset)
{

//	PRINT((
//		"### offsetSegmentValue(): %Ld\n",
//		offset));
		
	int64 segmentFactor = (int64)pow(10, fFractionalDigits + segment->scaleFactor());

	int64 value = _ValueFixed();
	
	// cut values below domain of the changed segment
	value /= segmentFactor;
	
	// add offset
	value += offset;
	
	// restore
	value *= segmentFactor;
	
	_SetValueFixed(value);

	// notify target 
	Invoke();

	if (fParam)
		updateParameter((double)value * (double)segmentFactor);
}


void
NumericValControl::mediaParameterChanged()
{	
	// fetch value
	size_t nSize;
	bigtime_t tLastChanged;
	status_t err;
	switch (fParam->ValueType()) {
		case B_FLOAT_TYPE:
		{	// +++++ left-channel hack
			float fParamValue[4];
			nSize = sizeof(float) * 4;
			// +++++ broken
			err = fParam->GetValue((void*)&fParamValue, &nSize, &tLastChanged);
			// if (err != B_OK)
			// break;

			setValue(fParamValue[0]);
			break;
		}

		case B_DOUBLE_TYPE:
		{
			double fParamValue;
			nSize = sizeof(double);
			err = fParam->GetValue((void*)&fParamValue, &nSize, &tLastChanged);
			if (err != B_OK)
				break;
		
			setValue(fParamValue);
			break;
		}
	}
}


void
NumericValControl::updateParameter(double value)
{
	ASSERT(fParam);
	
//	// is this kosher? +++++
//	// ++++++ 18sep99: no.
//	bigtime_t tpNow = system_time();

	// store value
	status_t err;
	switch (fParam->ValueType()) {
		case B_FLOAT_TYPE:
		{	// +++++ left-channel hack
			float fValue[2];
			fValue[0] = value;
			fValue[1] = value;
			err = fParam->SetValue((void*)&fValue, sizeof(float)*2, 0LL);
			break;
		}
		
		case B_DOUBLE_TYPE: {
			double fValue = value;
			err = fParam->SetValue((void*)&fValue, sizeof(double), 0LL);
			break;
		}
	}			
}


void
NumericValControl::setValue(const void* data, size_t size)
{
}


void
NumericValControl::getValue(void* data, size_t* ioSize)
{
}


status_t
NumericValControl::setValueFrom(const char* text)
{
	double d = atof(text);
	setValue(d, true);

	return B_OK;
}


status_t
NumericValControl::getString(BString& buffer)
{
	// should provide the same # of digits as the control! +++++
	BString format = "%.";
	format << (int32)fFractionalDigits << 'f';
	char cbuf[120];
	sprintf(cbuf, format.String(), value());
	buffer = cbuf;

	return B_OK;
}


void
NumericValControl::MessageReceived(BMessage* pMsg)
{
	status_t err;
	double dfValue;

	switch (pMsg->what) {
		case M_SET_VALUE:
			err = pMsg->FindDouble("value", &dfValue);
			if (err < B_OK) {
				_inherited::MessageReceived(pMsg);
				break;
			}
			
			setValue(dfValue);
			break;

		case B_MEDIA_PARAMETER_CHANGED:
		{
			int32 id;
			if (pMsg->FindInt32("be:parameter", &id) != B_OK)
				break;

			ASSERT(id == fParam->ID());
			mediaParameterChanged();
			break;
		}
			
		default:
			_inherited::MessageReceived(pMsg);
			break;
	}
}


void
NumericValControl::_SetDefaultConstraints(bool negativeVisible)
{
	double max = pow(10, fWholeDigits) - pow(10, -fFractionalDigits);
	double min = (negativeVisible) ? -max : 0.0;
	
	setConstraints(min, max);
}


//! calculates the current value as an int64
int64
NumericValControl::_ValueFixed() const {

//	PRINT((
//		"### NumericValControl::_ValueFixed()\n", value));

	int64 acc = 0LL;

	int64 scaleBase = fFractionalDigits;
	
	// walk segments, adding the value of each
	for (int n = CountEntries(); n > 0; --n) {
		const ValCtrlLayoutEntry& entry = _EntryAt(n-1);
		if (entry.type == ValCtrlLayoutEntry::SEGMENT_ENTRY) {
			const ValControlDigitSegment* digitSegment =
				dynamic_cast<ValControlDigitSegment*>(entry.pView);
			ASSERT(digitSegment);

//			PRINT((
//				"\t...segment %d: %d digits at %d: %Ld\n",
//				n-1,
//				digitSegment->digitCount(),
//				digitSegment->scaleFactor(),
//				digitSegment->value()));
//
			acc += digitSegment->value() * (int64)pow(10,
				scaleBase + digitSegment->scaleFactor());
//				
//			PRINT((
//				"\t-> %Ld\n\n", acc));
		}
	}
	
	return acc;
}


//! sets the value of each segment based on an int64 value;
// does not constrain the value
void
NumericValControl::_SetValueFixed(int64 fixed)
{
//	PRINT((
//		"### NumericValControl::_SetValueFixed(%Ld)\n", fixed));

	// constrain
	if (fixed > fMaxFixed)
		fixed = fMaxFixed;

	if (fixed < fMinFixed)
		fixed = fMinFixed;

	int64 scaleBase = fFractionalDigits;

	// set segments
	for (int n = CountEntries(); n > 0; --n) {
		const ValCtrlLayoutEntry& entry = _EntryAt(n-1);
		
		if (entry.type == ValCtrlLayoutEntry::SEGMENT_ENTRY) {
			ValControlDigitSegment* digitSegment =
				dynamic_cast<ValControlDigitSegment*>(entry.pView);
			ASSERT(digitSegment);

//			PRINT((
//				"\tsegment %d: %d digits at %d:\n",
//				n-1,
//				digitSegment->digitCount(),
//				digitSegment->scaleFactor()));
				
			// subtract higher-magnitude segments' value
			int64 hiCut = fixed % (int64)pow(10,
				scaleBase + digitSegment->scaleFactor() + digitSegment->digitCount());
					
//			PRINT((
//				"\t  [] %Ld\n", hiCut));

			// shift value
			int64 segmentValue = hiCut / (int64)pow(10,
				scaleBase + digitSegment->scaleFactor());
		
//			PRINT((
//				"\t  -> %Ld\n\n", segmentValue));

			digitSegment->setValue(segmentValue, fixed < 0);
		}
	}
}
