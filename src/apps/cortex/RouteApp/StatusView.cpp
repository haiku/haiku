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


// StatusView.cpp

#include "StatusView.h"
#include "cortex_ui.h"
#include "RouteAppNodeManager.h"
#include "TipManager.h"

// Application Kit
#include <Message.h>
#include <MessageRunner.h>
// Interface Kit
#include <Bitmap.h>
#include <Font.h>
#include <ScrollBar.h>
#include <Window.h>
// Support Kit
#include <Beep.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_ALLOC(x) //PRINT(x)
#define D_HOOK(x) //PRINT(x)
#define D_MESSAGE(x) //PRINT(x)
#define D_OPERATION(x) //PRINT(x)

// -------------------------------------------------------- //
// *** constants
// -------------------------------------------------------- //

const bigtime_t TICK_PERIOD = 50000;
const bigtime_t TEXT_DECAY_DELAY = 10 * 1000 * 1000;
const bigtime_t TEXT_DECAY_TIME = 3 * 1000 * 1000;

// width: 8, height:12, color_space: B_CMAP8
const unsigned char ERROR_ICON_BITS [] = {
	0xff,0xff,0x00,0x00,0xff,0xff,0xff,0xff,
	0xff,0x00,0xfa,0xfa,0x00,0xff,0xff,0xff,
	0x00,0x3f,0x3f,0xfa,0xfa,0x00,0xff,0xff,
	0x00,0xf9,0xf9,0x3f,0x5d,0x00,0xff,0xff,
	0x00,0xf9,0xf9,0x5d,0x5d,0x00,0xff,0xff,
	0x00,0xf9,0xf9,0x5d,0x5d,0x00,0xff,0xff,
	0x00,0x00,0xf9,0x5d,0x00,0x00,0xff,0xff,
	0xff,0x00,0x00,0x00,0x00,0xff,0xff,0xff,
	0x00,0xf9,0x00,0x00,0x5d,0x00,0xff,0xff,
	0x00,0xf9,0xf9,0x5d,0x5d,0x00,0xff,0xff,
	0x00,0x00,0xf9,0x5d,0x00,0x00,0x0f,0xff,
	0xff,0xff,0x00,0x00,0x00,0x0f,0x0f,0x0f,
};

// width: 8, height:12, color_space: B_CMAP8
const unsigned char INFO_ICON_BITS [] = {
	0xff,0xff,0x00,0x00,0x00,0xff,0xff,0xff,
	0xff,0x00,0x21,0x21,0x21,0x00,0xff,0xff,
	0xff,0x00,0x21,0x92,0x25,0x00,0xff,0xff,
	0xff,0x00,0x21,0x25,0x25,0x00,0xff,0xff,
	0xff,0xff,0x00,0x00,0x00,0xff,0xff,0xff,
	0xff,0x00,0x92,0x92,0x25,0x00,0xff,0xff,
	0xff,0x00,0x21,0x21,0x25,0x00,0xff,0xff,
	0xff,0x00,0x21,0x21,0x25,0x00,0xff,0xff,
	0x00,0x00,0x21,0x21,0x25,0x00,0x00,0xff,
	0x00,0xbe,0x21,0x21,0x92,0xbe,0x25,0x00,
	0x00,0x00,0x21,0x21,0x21,0x25,0x00,0x0f,
	0xff,0xff,0x00,0x00,0x00,0x00,0x0f,0x0f,
};

// -------------------------------------------------------- //
// *** ctor/dtor
// -------------------------------------------------------- //

StatusView::StatusView(
	BRect frame,
	RouteAppNodeManager *manager,
	BScrollBar *scrollBar)
	:	BStringView(frame, "StatusView", "", B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
					B_FRAME_EVENTS | B_WILL_DRAW),
		m_scrollBar(scrollBar),
		m_icon(0),
		m_opacity(1.0),
		m_clock(0),
		m_dragging(false),
		m_backBitmap(0),
		m_backView(0),
		m_dirty(true),
		m_manager(manager) {
	D_ALLOC(("StatusView::StatusView()\n"));

	SetViewColor(B_TRANSPARENT_COLOR);
	SetFont(be_plain_font);
	
	allocBackBitmap(frame.Width(), frame.Height());
}

StatusView::~StatusView() {
	D_ALLOC(("StatusView::~ParameterContainerView()\n"));

	// get the tip manager instance and reset
	TipManager *manager = TipManager::Instance();
	manager->removeAll(this);
	
	delete m_clock;
	
	freeBackBitmap();
}

// -------------------------------------------------------- //
// *** BScrollView impl
// -------------------------------------------------------- //

void StatusView::AttachedToWindow() {
	D_HOOK(("StatusView::AttachedToWindow()\n"));

	if (m_manager) {
		m_manager->setLogTarget(BMessenger(this, Window()));
	}
	
	allocBackBitmap(Bounds().Width(), Bounds().Height());
}

void StatusView::Draw(
	BRect updateRect) {
	D_HOOK(("StatusView::Draw()\n"));
	
	if(!m_backView) {
		drawInto(this, updateRect);
	} else {
		if(m_dirty) {
			m_backBitmap->Lock();
			drawInto(m_backView, updateRect);
			m_backView->Sync();
			m_backBitmap->Unlock();
			m_dirty = false;
		}

		SetDrawingMode(B_OP_COPY);
		DrawBitmap(m_backBitmap, updateRect, updateRect);
	}
}

void StatusView::FrameResized(
	float width,
	float height) {
	D_HOOK(("StatusView::FrameResized()\n"));

	allocBackBitmap(width, height);

	// get the tip manager instance and reset
	TipManager *manager = TipManager::Instance();
	manager->removeAll(this);

	// re-truncate the string if necessary
	BString text = m_fullText;
	if (be_plain_font->StringWidth(text.String()) > Bounds().Width() - 25.0) {
		be_plain_font->TruncateString(&text, B_TRUNCATE_END,
									  Bounds().Width() - 25.0);
		manager->setTip(m_fullText.String(), this);
	}
	BStringView::SetText(text.String());

	float minWidth, maxWidth, minHeight, maxHeight;
	Window()->GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	minWidth = width + 6 * B_V_SCROLL_BAR_WIDTH;
	Window()->SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
}

void StatusView::MessageReceived(
	BMessage *message) {
	D_MESSAGE(("StatusView::MessageReceived()\n"));

	switch (message->what) {
		case 'Tick':
			fadeTick();
			break;
			
		case RouteAppNodeManager::M_LOG: {
			D_MESSAGE((" -> RouteAppNodeManager::M_LOG\n"));

			BString title;
			if (message->FindString("title", &title) != B_OK) {
				return;
			}
			BString details, line;
			for (int32 i = 0; message->FindString("line", i, &line) == B_OK; i++) {
				if (details.CountChars() > 0) {
					details << "\n";
				}
				details << line;
			}
			status_t error = B_OK;
			message->FindInt32("error", &error);
			setMessage(title, details, error);
			break;
		}
		default: {
			BStringView::MessageReceived(message);
		}
	}
}

void StatusView::MouseDown(
	BPoint point) {
	D_HOOK(("StatusView::MouseDown()\n"));

	int32 buttons;
	if (Window()->CurrentMessage()->FindInt32("buttons", &buttons) != B_OK) {
		buttons = B_PRIMARY_MOUSE_BUTTON;
	}

	if (buttons == B_PRIMARY_MOUSE_BUTTON) {
		// drag rect
		BRect dragRect(Bounds());
		dragRect.left = dragRect.right - 10.0;
		if (dragRect.Contains(point)) {
			// resize
			m_dragging = true;
			SetMouseEventMask(B_POINTER_EVENTS,
							  B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
		}
	}
}

void StatusView::MouseMoved(
	BPoint point,
	uint32 transit,
	const BMessage *message) {
	D_HOOK(("StatusView::MouseMoved()\n"));

	if (m_dragging) {
		float x = point.x - (Bounds().right - 5.0);
		if ((Bounds().Width() + x) <= 16.0) {
			return;
		}
		if (m_scrollBar
		 && ((m_scrollBar->Bounds().Width() - x) <= (6 * B_V_SCROLL_BAR_WIDTH))) {
			return;
		}
		ResizeBy(x, 0.0);
		BRect r(Bounds());
		r.left = r.right - 5.0;
		if (x > 0)
			r.left -= x;
		m_dirty = true;
		Invalidate(r);
		if (m_scrollBar) {
			m_scrollBar->ResizeBy(-x, 0.0);
			m_scrollBar->MoveBy(x, 0.0);
		}
	}
}

void StatusView::MouseUp(
	BPoint point) {
	D_HOOK(("StatusView::MouseUp()\n"));

	m_dragging = false;
}

// -------------------------------------------------------- //
// *** internal operations
// -------------------------------------------------------- //

void 
StatusView::drawInto(BView *v, BRect updateRect)
{
	BRect r(Bounds());
	D_OPERATION(("StatusView::drawInto(%.1f, %.1f)\n", r.Width(), r.Height()));

	// draw border (minus right edge, which the scrollbar draws)
	v->SetDrawingMode(B_OP_COPY);
	v->BeginLineArray(8);
	v->AddLine(r.LeftTop(), r.RightTop(), M_MED_GRAY_COLOR);
	BPoint rtop = r.RightTop();
	rtop.y++;
	v->AddLine(rtop, r.RightBottom(), tint_color(M_MED_GRAY_COLOR, B_LIGHTEN_1_TINT));
	v->AddLine(r.RightBottom(), r.LeftBottom(), M_MED_GRAY_COLOR);
	v->AddLine(r.LeftBottom(), r.LeftTop(), M_MED_GRAY_COLOR);
	r.InsetBy(1.0, 1.0);
	v->AddLine(r.LeftTop(), r.RightTop(), M_LIGHT_GRAY_COLOR);
	rtop.y++;
	rtop.x--;
	v->AddLine(rtop, r.RightBottom(), M_GRAY_COLOR);
	v->AddLine(r.RightBottom(), r.LeftBottom(), tint_color(M_MED_GRAY_COLOR, B_LIGHTEN_1_TINT));
	v->AddLine(r.LeftBottom(), r.LeftTop(), M_LIGHT_GRAY_COLOR);
	v->EndLineArray();
	r.InsetBy(1.0, 1.0);
	v->SetLowColor(M_GRAY_COLOR);
	v->FillRect(r, B_SOLID_LOW);

	r.InsetBy(2.0, 0.0);
	v->SetDrawingMode(B_OP_ALPHA);
	v->SetHighColor(0, 0, 0, uchar(255 * m_opacity));

	// draw icon
	if (m_icon) {
		v->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		BPoint p = r.LeftTop();
		p.y--;
		v->DrawBitmap(m_icon, p);
	}

	// draw text
	r.left += 10.0;
	font_height fh;
	be_plain_font->GetHeight(&fh);
	r.bottom = Bounds().bottom - fh.descent
		- (Bounds().Height() - fh.ascent - fh.descent) / 2;
	v->MovePenTo(r.LeftBottom());
	v->DrawString(Text());

	// draw resize dragger
	v->SetDrawingMode(B_OP_OVER);
	r = Bounds();
	r.right -= 2.0;
	r.left = r.right - 2.0;
	r.InsetBy(0.0, 3.0);
	r.top += 1.0;
	for (int32 i = 0; i < r.IntegerHeight(); i += 3) {
		BPoint p = r.LeftTop() + BPoint(0.0, i);
		v->SetHighColor(M_MED_GRAY_COLOR);
		v->StrokeLine(p, p, B_SOLID_HIGH);
		p += BPoint(1.0, 1.0);
		v->SetHighColor(M_WHITE_COLOR);
		v->StrokeLine(p, p, B_SOLID_HIGH);
	}
}



void StatusView::setMessage(
	BString &title,
	BString &details,
	status_t error) {
	D_OPERATION(("StatusView::setMessage(%s)\n", title.String()));

	// get the tip manager instance and reset
	TipManager *manager = TipManager::Instance();
	manager->removeAll(this);

	// append error string
	if (error) {
		title << " (" << strerror(error) << ")";
	}

	// truncate if necessary
	bool truncated = false;
	m_fullText = title;
	if (be_plain_font->StringWidth(title.String()) > Bounds().Width() - 25.0) {
		be_plain_font->TruncateString(&title, B_TRUNCATE_END,
									  Bounds().Width() - 25.0);
		truncated = true;
	}
	BStringView::SetText(title.String());

	if (truncated || details.CountChars() > 0) {
		BString tip = m_fullText;
		if (details.CountChars() > 0) {
			tip << "\n" << details;
		}
		manager->setTip(tip.String(), this);
	}

	if (error) {
		beep();
		// set icon
		if (m_icon) {
			delete m_icon;
			m_icon = 0;
		}
		BRect iconRect(0.0, 0.0, 7.0, 11.0);
		m_icon = new BBitmap(iconRect, B_CMAP8);
		m_icon->SetBits(ERROR_ICON_BITS, 96, 0, B_CMAP8);
	}
	else {
		// set icon
		if (m_icon) {
			delete m_icon;
			m_icon = 0;
		}
		BRect iconRect(0.0, 0.0, 7.0, 11.0);
		m_icon = new BBitmap(iconRect, B_CMAP8);
		m_icon->SetBits(INFO_ICON_BITS, 96, 0, B_CMAP8);
	}
	m_dirty = true;
	startFade();
	Invalidate();
}

void StatusView::setErrorMessage(
	BString text,
	bool error) {
	D_OPERATION(("StatusView::setErrorMessage(%s)\n",
				 text.String()));

	// get the tip manager instance and reset
	TipManager *manager = TipManager::Instance();
	manager->removeAll(this);

	// truncate if necessary
	m_fullText = text;
	if (be_plain_font->StringWidth(text.String()) > Bounds().Width() - 25.0) {
		be_plain_font->TruncateString(&text, B_TRUNCATE_END,
									  Bounds().Width() - 25.0);
		manager->setTip(m_fullText.String(), this);
	}
	BStringView::SetText(text.String());

	if (error) {
		beep();
		// set icon
		if (m_icon) {
			delete m_icon;
			m_icon = 0;
		}
		BRect iconRect(0.0, 0.0, 7.0, 11.0);
		m_icon = new BBitmap(iconRect, B_CMAP8);
		m_icon->SetBits(ERROR_ICON_BITS, 96, 0, B_CMAP8);
	}
	else {
		// set icon
		if (m_icon) {
			delete m_icon;
			m_icon = 0;
		}
		BRect iconRect(0.0, 0.0, 7.0, 11.0);
		m_icon = new BBitmap(iconRect, B_CMAP8);
		m_icon->SetBits(INFO_ICON_BITS, 96, 0, B_CMAP8);
	}
	m_dirty = true;
	startFade();
	Invalidate();
}

void StatusView::startFade() {
	D_OPERATION(("StatusView::startFade()\n"));
	
	m_opacity = 1.0;
	m_decayDelay = TEXT_DECAY_DELAY;
	if(!m_clock) {
		m_clock = new BMessageRunner(
			BMessenger(this),
			new BMessage('Tick'),
			TICK_PERIOD);
	}
}

void StatusView::fadeTick() {
	D_HOOK(("StatusView::fadeTick()\n"));

	if (m_opacity > 0.0) {
		if(m_decayDelay > 0) {
			m_decayDelay -= TICK_PERIOD;
			return;
		}
		
		float steps = static_cast<float>(TEXT_DECAY_TIME)
					  / static_cast<float>(TICK_PERIOD);
		m_opacity -= (1.0 / steps);
		if (m_opacity < 0.001) {
			m_opacity = 0.0;
		}
		m_dirty = true;
		Invalidate();
	}
	else if (m_clock) {
		delete m_clock;
		m_clock = 0;
		
		// get the tip manager instance and reset
		TipManager *manager = TipManager::Instance();
		manager->removeAll(this);
	}
}

void StatusView::allocBackBitmap(float width, float height) {
	D_OPERATION(("StatusView::allocBackBitmap(%.1f, %.1f)\n", width, height));

	// sanity check
	if(width <= 0.0 || height <= 0.0)
		return;

	if(m_backBitmap) {
		// see if the bitmap needs to be expanded
		BRect b = m_backBitmap->Bounds();
		if(b.Width() >= width && b.Height() >= height)
			return;

		// it does; clean up:
		freeBackBitmap();
	}

	BRect b(0.0, 0.0, width, height);
	m_backBitmap = new BBitmap(b, B_RGB32, true);
	if(!m_backBitmap) {
		D_OPERATION(("StatusView::allocBackBitmap(): failed to allocate\n"));
		return;
	}
	
	m_backView = new BView(b, 0, B_FOLLOW_NONE, B_WILL_DRAW);
	m_backBitmap->AddChild(m_backView);
	m_dirty = true;
}

void StatusView::freeBackBitmap() {
	if(m_backBitmap) {
		delete m_backBitmap;
		m_backBitmap = 0;
		m_backView = 0;
	}
}


// END -- ParameterContainerView.cpp --
