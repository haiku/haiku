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


// TipView.h
// * PURPOSE
//   Provide a basic, extensible 'ToolTip' view, designed
//   to be hosted by a floating window (TipWindow).
// * HISTORY
//   e.moon		20oct99		multi-line support
//   e.moon		17oct99		Begun.

#ifndef __TipView_H__
#define __TipView_H__

#include <Font.h>
#include <String.h>
#include <View.h>

#include <vector>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class TipWindow;

class TipView :
	public	BView {
	typedef	BView _inherited;
	
public:											// *** dtor/ctors
	virtual ~TipView();
	TipView();

public:											// *** operations

	// if attached to a BWindow, the window must be locked
	virtual void setText(
		const char*							text);
		
public:											// *** BView

	virtual void Draw(
		BRect										updateRect);

	virtual void FrameResized(
		float										width,
		float										height);
		
	virtual void GetPreferredSize(
		float*									outWidth,
		float*									outHeight);

private:										// implementation
	BString										m_text;
	BPoint										m_offset;

	BFont											m_font;
	font_height								m_fontHeight;
	
	rgb_color									m_textColor;
	rgb_color									m_borderLoColor;
	rgb_color									m_borderHiColor;
	rgb_color									m_viewColor;


	typedef std::vector<int32> line_set;
	line_set									m_lineSet;

private:
	static const float				s_xPad;
	static const float				s_yPad;

	void _initColors();
	void _initFont();
	void _updateLayout(
		float										width,
		float										height);
	
	void _setText(
		const char*							text);
		
	float _maxTextWidth();
	float _textHeight();
};

__END_CORTEX_NAMESPACE
#endif /*__TipView_H__*/
