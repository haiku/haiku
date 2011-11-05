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


// ParameterContainerView.cpp

#include "ParameterContainerView.h"

// Interface Kit
#include <ScrollBar.h>
#include <View.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_ALLOC(x) //PRINT (x)
#define D_HOOK(x) //PRINT (x)
#define D_INTERNAL(x) //PRINT (x)

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

ParameterContainerView::ParameterContainerView(
	BRect dataRect,
	BView *target) :
		BView(
			target->Frame(),
			"ParameterContainerView",
			B_FOLLOW_ALL_SIDES,
			B_FRAME_EVENTS|B_WILL_DRAW),
		m_target(target),
	  m_dataRect(dataRect),
	  m_boundsRect(Bounds()),
	  m_hScroll(0),
	  m_vScroll(0) {
	D_ALLOC(("ParameterContainerView::ParameterContainerView()\n"));

	BView* wrapper = new BView(
		m_target->Bounds(), 0, B_FOLLOW_ALL_SIDES, B_WILL_DRAW|B_FRAME_EVENTS);
	m_target->SetResizingMode(B_FOLLOW_LEFT|B_FOLLOW_TOP);
	m_target->MoveTo(B_ORIGIN);
	wrapper->AddChild(m_target);
	AddChild(wrapper);

	BRect b = wrapper->Frame();

	ResizeTo(
		b.Width() + B_V_SCROLL_BAR_WIDTH,
		b.Height() + B_H_SCROLL_BAR_HEIGHT);

	BRect hsBounds = b;
	hsBounds.left--;
	hsBounds.top = hsBounds.bottom + 1;
	hsBounds.right++;
	hsBounds.bottom = hsBounds.top + B_H_SCROLL_BAR_HEIGHT + 1;
	m_hScroll = new BScrollBar(
		hsBounds,
		"hScrollBar",
		m_target,
		0, 0, B_HORIZONTAL);
	AddChild(m_hScroll);

	BRect vsBounds = b;
	vsBounds.left = vsBounds.right + 1;
	vsBounds.top--;
	vsBounds.right = vsBounds.right + B_V_SCROLL_BAR_WIDTH + 1;
	vsBounds.bottom++;
	m_vScroll = new BScrollBar(
		vsBounds,
		"vScrollBar",
		m_target,
		0, 0, B_VERTICAL);
	AddChild(m_vScroll);


	SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_1_TINT));
	_updateScrollBars();	
}

ParameterContainerView::~ParameterContainerView() {
	D_ALLOC(("ParameterContainerView::~ParameterContainerView()\n"));

}

// -------------------------------------------------------- //
// *** BScrollView impl
// -------------------------------------------------------- //

void ParameterContainerView::FrameResized(
	float width,
	float height) {
	D_HOOK(("ParameterContainerView::FrameResized()\n"));

	BView::FrameResized(width, height);
//	BRect b = ChildAt(0)->Frame();
//	printf("param view:\n");
//	b.PrintToStream();
//	printf("hScroll:\n");
//	m_target->ScrollBar(B_HORIZONTAL)->Frame().PrintToStream();
//	printf("vScroll:\n");
//	m_target->ScrollBar(B_VERTICAL)->Frame().PrintToStream();
//	fprintf(stderr, "m: %.1f,%.1f c: %.1f,%.1f)\n", width, height, b.Width(),b.Height());

	if(height > m_boundsRect.Height()) {
		Invalidate(BRect(
			m_boundsRect.left, m_boundsRect.bottom, m_boundsRect.right, m_boundsRect.top+height));
	}
	
	if(width > m_boundsRect.Width()) {
		Invalidate(BRect(
			m_boundsRect.right, m_boundsRect.top, m_boundsRect.left+width, m_boundsRect.bottom));
	}

	m_boundsRect = Bounds();
	_updateScrollBars();
}

// -------------------------------------------------------- //
// *** internal operations
// -------------------------------------------------------- //

void ParameterContainerView::_updateScrollBars() {
	D_INTERNAL(("ParameterContainerView::_updateScrollBars()\n"));

	// fetch the vertical ScrollBar
	if (m_vScroll) {
		float height = Bounds().Height() - B_H_SCROLL_BAR_HEIGHT;
		// Disable the ScrollBar if the window is large enough to display
		// the entire parameter view
		D_INTERNAL((" -> dataRect.Height() = %f   scrollView.Height() = %f\n", m_dataRect.Height(), height));
		if (height > m_dataRect.Height()) {
			D_INTERNAL((" -> disable vertical scroll bar\n"));
			m_vScroll->SetRange(0.0, 0.0);
			m_vScroll->SetProportion(1.0);
		}
		else {
			D_INTERNAL((" -> enable vertical scroll bar\n"));
			m_vScroll->SetRange(m_dataRect.top, m_dataRect.bottom - height);
			m_vScroll->SetProportion(height / m_dataRect.Height());
		}
	}
	if (m_hScroll) {
		float width = Bounds().Width() - B_V_SCROLL_BAR_WIDTH;
		// Disable the ScrollBar if the view is large enough to display
		// the entire data-rect
		D_INTERNAL((" -> dataRect.Width() = %f   scrollView.Width() = %f\n", m_dataRect.Width(), width));
		if (width > m_dataRect.Width()) {
			D_INTERNAL((" -> disable horizontal scroll bar\n"));
			m_hScroll->SetRange(0.0, 0.0);
			m_hScroll->SetProportion(1.0);
		}
		else {
			D_INTERNAL((" -> enable horizontal scroll bar\n"));
			m_hScroll->SetRange(m_dataRect.left, m_dataRect.right - width);
			m_hScroll->SetProportion(width / m_dataRect.Width());
		}
	}
}

// END -- ParameterContainerView.cpp --
