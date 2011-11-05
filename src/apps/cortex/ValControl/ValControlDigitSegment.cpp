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


// ValControlDigitSegment.cpp

#include "ValControlDigitSegment.h"
#include "ValControl.h"

#include "NumericValControl.h"

#include <Debug.h>

#include <math.h>
#include <stdlib.h>
#include <cstdio>

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// constants/static stuff
// -------------------------------------------------------- //

const float ValControlDigitSegment::s_widthTrim			= -2;

const BFont* ValControlDigitSegment::s_cachedFont 	= 0;
float ValControlDigitSegment::s_cachedDigitWidth 		= 0.0;

// -------------------------------------------------------- //
// ctor/dtor/accessors
// -------------------------------------------------------- //

ValControlDigitSegment::ValControlDigitSegment(
	uint16											digitCount,
	int16												scaleFactor,
	bool												negativeVisible,
	display_flags								flags) :
	
	ValControlSegment(SOLID_UNDERLINE),

	m_digitCount(digitCount),
	m_value(0),
	m_negative(false),
	m_scaleFactor(scaleFactor),
	m_font(0),
	m_yOffset(0.0),
	m_minusSignWidth(0.0),
	m_digitPadding(0.0),
	m_flags(flags),
	m_negativeVisible(negativeVisible) {}
		
ValControlDigitSegment::~ValControlDigitSegment() {}

uint16 ValControlDigitSegment::digitCount() const {
	return m_digitCount;
}

int16 ValControlDigitSegment::scaleFactor() const {
	return m_scaleFactor;
}

int64 ValControlDigitSegment::value() const {
	return m_value;
}

// -------------------------------------------------------- //
// operations
// -------------------------------------------------------- //

// revised setValue() 18sep99: now sets the
// value of the displayed digits ONLY.
//
// a tad simpler. the old setValue() is provided for giggles.
//
void ValControlDigitSegment::setValue(
	int64												value,
	bool												negative) {
	
	if(
		value == m_value &&
		m_negative == negative)
		return;
	
	m_value = value;
	m_negative = negative;
	Invalidate();
}

//// +++++
//void ValControlDigitSegment::setValue(double dfValue) {
//
//	printf("seg[%d]::setValue(%.12f)\n", m_digitCount, dfValue);
//	
//	// convert possibly-negative value into absolute value and 
//	// negative flag
//	bool m_bWasNegative = m_negative;
//	m_negative = (m_negativeVisible && dfValue < 0.0);
//	dfValue = fabs(dfValue);
//
//	// prepare to scale the value to fit the digits this segment
//	// represents	
//	bool bMult = m_scaleFactor < 0;	
//	int64 nLowPow = m_scaleFactor ? (int64)pow(10.0, abs(m_scaleFactor)) : 1;
//	int64 nHighPow = (int64)pow(10.0, m_digitCount);
//	
////	printf("  lowPow %Ld, highPow %Ld\n", nLowPow, nHighPow);
//			
//	double dfTemp = bMult ? dfValue * nLowPow : dfValue / nLowPow;
////	printf("  -> %.8lf\n", dfTemp);
//	
//	int64 nLocal;
//	if(m_scaleFactor < 0) {
//		// really ugly rounding business: there must be a cleaner
//		// way to do this...
//		double dfC = ceil(dfTemp);
//		double dfCDelta = dfC-dfTemp;
//		double dfF = floor(dfTemp);
//		double dfFDelta = dfTemp - dfF;
//
//		nLocal = (int64)((dfCDelta < dfFDelta) ? dfC : dfF);
//	}
//	else
//		nLocal = (int64)dfTemp;
//
////	printf("  -> %Ld\n", nLocal);
//	nLocal %= nHighPow;
////	printf("  -> %Ld\n", nLocal);
//	
//	if(nLocal != m_value || m_negative != m_bWasNegative) {
//		m_value = nLocal;
//		Invalidate();
//	}
//}

// -------------------------------------------------------- //
// ValControlSegment impl.
// -------------------------------------------------------- //

ValCtrlLayoutEntry ValControlDigitSegment::makeLayoutEntry() {
	return ValCtrlLayoutEntry(this, ValCtrlLayoutEntry::SEGMENT_ENTRY);
}

float ValControlDigitSegment::handleDragUpdate(
		float												distance) {

	int64 units = (int64)(distance / dragScaleFactor());
	float remaining = distance;
	
	if(units) {
		remaining = fmod(distance, dragScaleFactor());
		
		// +++++ echk [23aug99] -- is this the only way?
		NumericValControl* numericParent = dynamic_cast<NumericValControl*>(parent());
		ASSERT(numericParent);

		// adjust value for parent:
//		dfUnits = floor(dfUnits);
//		dfUnits *= pow(10.0, m_scaleFactor);
//	
//		// ++++++ 17sep99
//		PRINT((
//			"offset: %.8f\n", dfUnits));
//			
//		numericParent->offsetValue(dfUnits);

		numericParent->offsetSegmentValue(this, units);
	}
	
	// return 'unused pixels'
	return remaining;
}

void ValControlDigitSegment::mouseReleased() {
	// +++++
}

// -------------------------------------------------------- //
// BView impl.
// -------------------------------------------------------- //

void ValControlDigitSegment::Draw(BRect updateRect) {

//	PRINT((
//		"### ValControlDigitSegment::Draw()\n"));
//

	ASSERT(m_font);

	BBitmap* pBufferBitmap = parent()->backBuffer();
	BView* pView = pBufferBitmap ? parent()->backBufferView() : this;
	if(pBufferBitmap)
		pBufferBitmap->Lock();

//	rgb_color white = {255,255,255,255};
	rgb_color black = {0,0,0,255};
	rgb_color disabled = tint_color(black, B_LIGHTEN_2_TINT);
	rgb_color viewColor = ViewColor();
	
	// +++++
	
	BRect b = Bounds();
//	PRINT((
//		"# ValControlDigitSegment::Draw(%.1f,%.1f,%.1f,%.1f) %s\n"
//		"  frame(%.1f,%.1f,%.1f,%.1f)\n\n",
//		updateRect.left, updateRect.top, updateRect.right, updateRect.bottom,
//		pBufferBitmap ? "(BUFFERED)" : "(DIRECT)",
//		Frame().left, Frame().top, Frame().right, Frame().bottom));
		
	float digitWidth = MaxDigitWidth(m_font);
	BPoint p;
	p.x = b.right - digitWidth;
	p.y = m_yOffset;
	
//	// clear background
//	pView->SetHighColor(white);
//	pView->FillRect(b);

	// draw a digit at a time, right to left (low->high)
	pView->SetFont(m_font);
	if(parent()->IsEnabled()) {
			
		pView->SetHighColor(black);
	} else {
			
		pView->SetHighColor(disabled);
	}
	
	pView->SetLowColor(viewColor);
	int16 digit;
	int64 cur = abs(m_value);

	for(digit = 0;
		digit < m_digitCount;
		digit++, cur /= 10, p.x -= (digitWidth+m_digitPadding)) {

		uint8 digitValue = (uint8)(cur % 10);
		if(digit && !(m_flags & ZERO_FILL) && !cur)
			break;
		pView->DrawChar('0' + digitValue, p);
//		PRINT(("ch(%.1f,%.1f): %c\n", p.x, p.y, '0' + digitValue));
	}

	if(m_negative) {
		// draw minus sign
		p.x += (digitWidth-m_minusSignWidth);
		pView->DrawChar('-', p);
	}

	// paint buffer?
	if(pBufferBitmap) {
		pView->Sync();
		DrawBitmap(parent()->backBuffer(), b, b);
		pBufferBitmap->Unlock();
	}

	_inherited::Draw(updateRect);	
}

// must have parent at this point +++++
void ValControlDigitSegment::GetPreferredSize(float* pWidth, float* pHeight) {

//	// font initialized?
//	if(!m_font) {
//		initFont();
//	}

	*pWidth = prefWidth();
	*pHeight = prefHeight();
}

// +++++ need a way to return an overlap amount?
//       -> underline should extend a pixel to the right.
float ValControlDigitSegment::prefWidth() const {
	ASSERT(m_font);

	float width = (m_digitCount*MaxDigitWidth(m_font)) +
		((m_digitCount - 1)*m_digitPadding);
	if(m_negativeVisible)
		width += (m_minusSignWidth + m_digitPadding);

	return width;
}

float ValControlDigitSegment::prefHeight() const {
	ASSERT(m_font);
	return m_fontHeight.ascent + m_fontHeight.descent + m_fontHeight.leading;
}

// do any font-related layout work
void ValControlDigitSegment::fontChanged(
	const BFont*								font) {
//	PRINT((
//		"* ValControlDigitSegment::fontChanged()\n"));
		
	m_font = font;

	m_font->GetHeight(&m_fontHeight);
	
	ASSERT(parent());
	m_yOffset = parent()->baselineOffset();
	char c = '-';
	m_minusSignWidth = m_font->StringWidth(&c, 1) + s_widthTrim;

	// space between digits should be the same as space between
	// segments, for consistent look:	
	m_digitPadding = parent()->segmentPadding();
}
	
// -------------------------------------------------------- //
// BHandler impl.
// -------------------------------------------------------- //

void ValControlDigitSegment::MessageReceived(BMessage* pMsg) {
	
	double fVal;
	status_t err;
	
	switch(pMsg->what) {
				
		case ValControl::M_SET_VALUE:
			err = pMsg->FindDouble("value", &fVal);
			ASSERT(err == B_OK);
			setValue((int64)fVal, fVal < 0);
			break;
			
		case ValControl::M_GET_VALUE: {
			BMessage reply(ValControl::M_VALUE);
			reply.AddDouble("value", value());
			pMsg->SendReply(&reply);	
			break;
		}
	}
}

// -------------------------------------------------------- //
// archiving/instantiation
// -------------------------------------------------------- //

ValControlDigitSegment::ValControlDigitSegment(BMessage* pArchive) :
	ValControlSegment(pArchive),
	m_font(0),
	m_digitPadding(0.0) {

	// #/digits
	status_t err = pArchive->FindInt16("digits", (int16*)&m_digitCount);
	ASSERT(err == B_OK);
	
	// current value		
	err = pArchive->FindInt64("value", &m_value);
	ASSERT(err == B_OK);
	
	// scaling
	err = pArchive->FindInt16("scaleFactor", &m_scaleFactor);
	ASSERT(err == B_OK);
}

status_t ValControlDigitSegment::Archive(BMessage* pArchive, bool bDeep) const{
	_inherited::Archive(pArchive, bDeep);

	pArchive->AddInt16("digits", m_digitCount);
	pArchive->AddInt64("value", m_value);
	pArchive->AddInt16("scaleFactor", m_scaleFactor);
	
	return B_OK;
}

/* static */
BArchivable* ValControlDigitSegment::Instantiate(BMessage* pArchive) {
	if(validate_instantiation(pArchive, "ValControlDigitSegment"))
		return new ValControlDigitSegment(pArchive);
	else
		return 0;
}

// -------------------------------------------------------- //
// helpers
// -------------------------------------------------------- //

/*static*/
float ValControlDigitSegment::MaxDigitWidth(const BFont* pFont) {
	ASSERT(pFont);
	if(s_cachedFont == pFont)
		return s_cachedDigitWidth;
	
	s_cachedFont = pFont;
	float fMax = 0.0;
	for(char c = '0'; c <= '9'; c++) {
		float fWidth = pFont->StringWidth(&c, 1);
		if(fWidth > fMax)
			fMax = fWidth;
	}
	
	s_cachedDigitWidth = ceil(fMax + s_widthTrim);
	return s_cachedDigitWidth;
}

// END -- ValControlDigitSegment.cpp --
