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


// TipView.cpp

#include "TipView.h"

#include <Debug.h>
#include <Window.h>
#include <cmath>
#include <cstring>

using namespace std;

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// constants
// -------------------------------------------------------- //

const float TipView::s_xPad = 5.0;
const float TipView::s_yPad = 2.0;

// -------------------------------------------------------- //
// *** dtor/ctors
// -------------------------------------------------------- //

TipView::~TipView() {}
TipView::TipView() :
	BView(
		BRect(0,0,0,0),
		"TipView",
		B_FOLLOW_NONE,
		B_WILL_DRAW|B_FRAME_EVENTS),
	m_font(be_plain_font) {
	
	_initColors();
	_initFont();
}

// -------------------------------------------------------- //
// *** operations
// -------------------------------------------------------- //

void TipView::setText(
	const char*							text) {

	_setText(text);
}
		
// -------------------------------------------------------- //
// *** BView
// -------------------------------------------------------- //

// Adapted from c.lenz' ToolTipView::Draw()
void TipView::Draw(
	BRect										updateRect) {

	BRect r = Bounds();

	// Draw border and fill
	SetDrawingMode(B_OP_ALPHA);
	SetHighColor(m_borderLoColor);
	StrokeLine(r.LeftBottom(), r.RightBottom());
	StrokeLine(r.RightTop(), r.RightBottom());
	SetHighColor(m_borderHiColor);
	StrokeLine(r.LeftTop(), r.RightTop());
	StrokeLine(r.LeftTop(), r.LeftBottom());
	SetHighColor(m_viewColor);
	SetDrawingMode(B_OP_ALPHA);
	r.InsetBy(1.0, 1.0);
	FillRect(r);

	// Draw text
	SetDrawingMode(B_OP_OVER);
	SetHighColor(m_textColor);
	
	BPoint p = m_offset;
	for(uint32 n = 0; n < m_lineSet.size(); ++n) {
		
		uint32 from = m_lineSet[n];
		uint32 to = (n < m_lineSet.size()-1) ? m_lineSet[n+1]-1 :
			m_text.Length();
		
		if(to > from)
			DrawString(
				m_text.String() + from,
				to - from,
				p);
			
		p.y += (m_fontHeight.ascent + m_fontHeight.descent + m_fontHeight.leading);
	}
}

void TipView::FrameResized(
	float										width,
	float										height) {
	
	_inherited::FrameResized(width, height);
	_updateLayout(width, height);
	Invalidate();
}
		
void TipView::GetPreferredSize(
	float*									outWidth,
	float*									outHeight) {

//	*outWidth = ceil(m_font.StringWidth(m_text.String()) + s_xPad*2);
	*outWidth = ceil(_maxTextWidth() + s_xPad*2);
	*outHeight = ceil(_textHeight() + s_yPad*2);
}

// -------------------------------------------------------- //
// implementation
// -------------------------------------------------------- //
	
void _make_color(
	rgb_color*							outColor,
	uint8										red,
	uint8										green,
	uint8										blue,
	uint8										alpha=255);
void _make_color(
	rgb_color*							outColor,
	uint8										red,
	uint8										green,
	uint8										blue,
	uint8										alpha) {
	outColor->red = red;
	outColor->green = green;
	outColor->blue = blue;
	outColor->alpha = alpha;
}
	
void TipView::_initColors() {

	SetViewColor(B_TRANSPARENT_COLOR);

	_make_color(&m_textColor, 0, 0, 0, 255);
	_make_color(&m_borderHiColor, 0, 0, 0, 255 /*196*/);
	_make_color(&m_borderLoColor, 0, 0, 0, 255 /*196*/);
	_make_color(&m_viewColor, 255, 255, 240, 255 /*216*/);
}

void TipView::_initFont() {

	SetFont(&m_font);
	m_font.GetHeight(&m_fontHeight);
}

void TipView::_updateLayout(
	float										width,
	float										height) {

	// center text
	m_offset.x = (width - _maxTextWidth()) / 2;

	m_offset.y = (height - _textHeight()) / 2;
	m_offset.y += m_fontHeight.ascent;
}

void TipView::_setText(
	const char*							text) {

	ASSERT(text);
	
	m_lineSet.clear();
	m_text = text;
	
	// skip leading newlines
	int32 n = 0;
	while(n < m_text.Length() && m_text[n] == '\n')
		++n;
	
	// mark first line
	m_lineSet.push_back(n);
	
	// break following lines
	while(n < m_text.Length()) {
		int32 nextBreak = m_text.FindFirst('\n', n);
		// no more line breaks?
		if(nextBreak < n)
			break;
		n = nextBreak + 1;
		m_lineSet.push_back(n);
	}
}

float TipView::_maxTextWidth() {
	float max = 0.0;
	for(uint32 n = 0; n < m_lineSet.size(); ++n) {
		uint32 from = m_lineSet[n];
		uint32 to = (n < m_lineSet.size()-1) ? m_lineSet[n+1]-1 :
			m_text.Length();
		float lineWidth = m_font.StringWidth(
			m_text.String() + from,
			to - from);
		if(lineWidth > max)
			max = lineWidth;
	}
	return max;
}

float TipView::_textHeight() {
	float height = m_fontHeight.ascent + m_fontHeight.descent;
	if(m_lineSet.size() > 1)
		height += (m_lineSet.size()-1) *
			(m_fontHeight.ascent + m_fontHeight.descent + m_fontHeight.leading);
	
	return height;
}

// END -- TipView.cpp --
