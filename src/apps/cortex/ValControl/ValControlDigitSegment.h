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


// ValControlDigitSegment.h
// * PURPOSE
//   Represents a fixed number of digits in a numeric value-
//   selection control.
// * HISTORY
//   e.moon			18sep99		moving to a fixed-point approach
//   e.moon			20jan99		Begun


#ifndef __VALCONTROLDIGITSEGMENT_H__
#define __VALCONTROLDIGITSEGMENT_H__

#include "ValControlSegment.h"

#include <Font.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class ValControl;

class ValControlDigitSegment : /*extends*/ public ValControlSegment {
	typedef ValControlSegment _inherited;

public:													// flags/types
	enum display_flags {
		NONE			=0,
		ZERO_FILL	=1
	};
	
public:													// ctor/dtor/accessors
	// scaleFactor is the power of ten corresponding to the
	// rightmost digit: for a whole-number segment, this is 0;
	// for a 2-digit fractional segment, this would be -2.
	// +++++ is this still accurate? 18sep99

	ValControlDigitSegment(
		uint16											digitCount,
		int16												scaleFactor,
		bool												negativeVisible,
		display_flags								flags=NONE);
	~ValControlDigitSegment();
	
	uint16 digitCount() const;
	int16 scaleFactor() const;
	int64 value() const;

	float prefWidth() const;
	float prefHeight() const;
	
public:								// operations
	// revised setValue() 18sep99: now sets the
	// value of the displayed digits ONLY.
	void setValue(
		int64												value,
		bool												negative);

//	// operates on a real value; based on scaling factor,
//	// retrieves the proper digits +++++nyi
//	void setValue(double dfValue);	// made public 30jan99

public:						// ValControlSegment impl.

	// do any font-related layout work
	virtual void fontChanged(
		const BFont*								font);
		
	virtual ValCtrlLayoutEntry makeLayoutEntry();
	
	virtual float handleDragUpdate(
		float												distance);
	virtual void mouseReleased(); //nyi

public:						// BView impl.
	virtual void Draw(BRect updateRect);
	virtual void GetPreferredSize(float* pWidth, float* pHeight);
	
public:						// BHandler impl.
	virtual void MessageReceived(BMessage* pMsg); //nyi

public:						// archiving/instantiation
	ValControlDigitSegment(BMessage* pArchive);
	virtual status_t Archive(BMessage* pArchive, bool bDeep) const;
	_EXPORT static BArchivable* Instantiate(BMessage* pArchive);

//private:					// impl. helpers
//	void initFont();
//	
protected:					// members
	// * internal value representation
	uint16					m_digitCount;
	int64						m_value;
	bool						m_negative;
	int16						m_scaleFactor;
	
	// * font
	const BFont*		m_font;
	font_height			m_fontHeight;
	float						m_yOffset;
	float						m_minusSignWidth;

	float						m_digitPadding;
	
	// * display settings
	display_flags		m_flags;
	bool						m_negativeVisible;
	
public:						// constants
	static const float		s_widthTrim;

protected:					// static stuff
	static float MaxDigitWidth(const BFont* pFont);

	static const BFont*		s_cachedFont;
	static float					s_cachedDigitWidth;
};

__END_CORTEX_NAMESPACE
#endif /* __VALCONTROLDIGITSEGMENT_H__ */
