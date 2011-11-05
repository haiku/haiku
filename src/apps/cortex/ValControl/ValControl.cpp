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


// ValControl.cpp

#include "ValControl.h"
#include "ValControlSegment.h"

#include "TextControlFloater.h"

#include <Debug.h>
#include <String.h>
#include <Window.h>

#include <algorithm>
#include <functional>
#include <cstdio>

using namespace std;

__USE_CORTEX_NAMESPACE


const float ValControl::fSegmentPadding = 2.0;

// the decimal point covers one more pixel x and y-ward:
const float ValControl::fDecimalPointWidth = 2.0;
const float ValControl::fDecimalPointHeight = 2.0;


/*protected*/
ValControl::ValControl(BRect frame, const char* name, const char* label,
		BMessage* message, align_mode alignMode, align_flags alignFlags,
		update_mode updateMode, bool backBuffer)
	: BControl(frame, name, label, message, B_FOLLOW_TOP|B_FOLLOW_LEFT,
		B_WILL_DRAW|B_FRAME_EVENTS),
	fDirty(true),
	fUpdateMode(updateMode),
	fLabelFont(be_bold_font),
	fValueFont(be_bold_font),
	fAlignMode(alignMode),
	fAlignFlags(alignFlags),
	fOrigBounds(Bounds()),
	fHaveBackBuffer(backBuffer),
	fBackBuffer(NULL),
	fBackBufferView(NULL)
{
	if (fHaveBackBuffer)
		_AllocBackBuffer(frame.Width(), frame.Height());

//	m_font.SetSize(13.0);	
//	rgb_color red = {255,0,0,255};
//	SetViewColor(red);
}


ValControl::~ValControl()
{
	delete fBackBuffer;
}


ValControl::update_mode
ValControl::updateMode() const
{
	return fUpdateMode;
}


void
ValControl::setUpdateMode(update_mode mode)
{
	fUpdateMode = mode;
}


const BFont*
ValControl::labelFont() const
{
	return &fLabelFont;
}


void
ValControl::setLabelFont(const BFont* labelFont)
{
	fLabelFont = labelFont;
	// inform label segments
	_InvalidateAll();
}	


const BFont*
ValControl::valueFont() const
{
	return &fValueFont;
}


void
ValControl::setValueFont(const BFont* valueFont)
{
	fValueFont = valueFont;
	
	// inform value segments
	for (int n = CountEntries(); n > 0; --n) {
		const ValCtrlLayoutEntry& e = _EntryAt(n-1);
		if (e.type != ValCtrlLayoutEntry::SEGMENT_ENTRY)
			continue;
			
		ValControlSegment* s = dynamic_cast<ValControlSegment*>(e.pView);
		ASSERT(s);
		s->SetFont(&fValueFont);
		s->fontChanged(&fValueFont);
	}
}


float
ValControl::baselineOffset() const
{
	font_height h;
	be_plain_font->GetHeight(&h);
	return ceil(h.ascent);
}


float
ValControl::segmentPadding() const
{
	return fSegmentPadding;
}


BView*
ValControl::backBufferView() const
{
	return fBackBufferView;
}


BBitmap*
ValControl::backBuffer() const
{
	return fBackBuffer;
}


void
ValControl::dump()
{
#if defined(DEBUG)
	BRect f = Frame();

	PRINT((
		"*** ValControl::dump():\n"
		"    FRAME    (%.1f,%.1f)-(%.1f,%.1f)\n"
		"    ENTRIES:\n",
		f.left, f.top, f.right, f.bottom));

	for (layout_set::const_iterator it = fLayoutSet.begin();
		it != fLayoutSet.end(); ++it) {
		const ValCtrlLayoutEntry& e = *it;
		switch (e.type) {
			case ValCtrlLayoutEntry::SEGMENT_ENTRY:
				PRINT(("    Segment       "));
				break;

			case ValCtrlLayoutEntry::VIEW_ENTRY:
				PRINT(("    View          "));
				break;

			case ValCtrlLayoutEntry::DECIMAL_POINT_ENTRY:
				PRINT(("    Decimal Point "));
				break;

			default:
				PRINT(("    ???           "));
				break;
		}

		PRINT(("\n      cached frame (%.1f,%.1f)-(%.1f,%.1f) + pad(%.1f)\n",
			e.frame.left, e.frame.top, e.frame.right, e.frame.bottom,
			e.fPadding));

		if (e.type == ValCtrlLayoutEntry::SEGMENT_ENTRY
			|| e.type == ValCtrlLayoutEntry::VIEW_ENTRY) {
			if (e.pView) {
				PRINT(("      real frame   (%.1f,%.1f)-(%.1f,%.1f)\n\n",
					e.pView->Frame().left, e.pView->Frame().top,
					e.pView->Frame().right, e.pView->Frame().bottom));
			} else
				PRINT(("      (no view!)\n\n"));
		}
	}
	PRINT(("\n"));
#endif
}


void
ValControl::SetEnabled(bool enabled)
{
	// redraw if enabled-state changes
	_Inherited::SetEnabled(enabled);

	_InvalidateAll();
}


void
ValControl::_InvalidateAll()
{
	Invalidate();
	int c = CountChildren();
	for (int n = 0; n < c; ++n)
		ChildAt(n)->Invalidate();
}


void
ValControl::AttachedToWindow()
{
	// adopt parent view's color
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
ValControl::AllAttached()
{
	// move children to requested positions
	BWindow* pWnd = Window();
	pWnd->BeginViewTransaction();
	
	for_each(fLayoutSet.begin(), fLayoutSet.end(),
		ptr_fun(&ValCtrlLayoutEntry::InitChildFrame)); // +++++?
	
	pWnd->EndViewTransaction();
}


//! Paint decorations (& decimal point)
void
ValControl::Draw(BRect updateRect)
{	
	// draw lightweight entries:
	for (unsigned int n = 0; n < fLayoutSet.size(); n++) {
		if (fLayoutSet[n].type == ValCtrlLayoutEntry::DECIMAL_POINT_ENTRY)
			drawDecimalPoint(fLayoutSet[n]);
	}
}


void
ValControl::drawDecimalPoint(ValCtrlLayoutEntry& e)
{	
	rgb_color dark = {0, 0, 0, 255};
	rgb_color med = {200, 200, 200, 255};
//	rgb_color light = {244,244,244,255};
	
	BPoint center;
	center.x = e.frame.left + 1;
	center.y = baselineOffset() - 1;
	
	SetHighColor(dark);
	StrokeLine(center, center);
	SetHighColor(med);
	StrokeLine(center - BPoint(0, 1), center + BPoint(1, 0));
	StrokeLine(center - BPoint(1, 0), center + BPoint(0, 1));

//	SetHighColor(light);
//	StrokeLine(center+BPoint(-1,1), center+BPoint(-1,1));
//	StrokeLine(center+BPoint(1,1), center+BPoint(1,1));
//	StrokeLine(center+BPoint(-1,-1), center+BPoint(-1,-1));
//	StrokeLine(center+BPoint(1,-1), center+BPoint(1,-1));
}


void
ValControl::FrameResized(float width, float height)
{	
	_Inherited::FrameResized(width,height);
	if (fHaveBackBuffer)
		_AllocBackBuffer(width, height);
//
//	PRINT((
//		"# ValControl::FrameResized(): %.1f, %.1f\n",
//		width, height));
}


void
ValControl::GetPreferredSize(float* outWidth, float* outHeight)
{
	ASSERT(fLayoutSet.size() > 0);
	
	*outWidth =
		fLayoutSet.back().frame.right -
		fLayoutSet.front().frame.left;
		
	*outHeight = 0;
	for(layout_set::const_iterator it = fLayoutSet.begin();
		it != fLayoutSet.end(); ++it) {
		if((*it).frame.Height() > *outHeight)
			*outHeight = (*it).frame.Height();
	}
//
//	PRINT((
//		"# ValControl::GetPreferredSize(): %.1f, %.1f\n",
//		*outWidth, *outHeight));
}


void
ValControl::MakeFocus(bool focused)
{
	_Inherited::MakeFocus(focused);

	// +++++ only the underline needs to be redrawn	
	_InvalidateAll();
}


void
ValControl::MouseDown(BPoint where)
{
	MakeFocus(true);
}


void
ValControl::MessageReceived(BMessage* message)
{
	status_t err;
	const char* stringValue;

//	PRINT((
//		"ValControl::MessageReceived():\n"));
//	message->PrintToStream();

	switch (message->what) {
		case M_SET_VALUE:
			err = message->FindString("_value", &stringValue);
			if(err < B_OK) {
				PRINT((
					"! ValControl::MessageReceived(): no _value found!\n"));
				break;
			}
				
			// set from string
			err = setValueFrom(stringValue);
			if (err < B_OK) {
				PRINT((
					"! ValControl::MessageReceived(): setValueFrom('%s'):\n"
					"  %s\n",
					stringValue,
					strerror(err)));
			}
			
			// +++++ broadcast new value +++++ [23aug99]
			break;
			
		case M_GET_VALUE: // +++++
			break;
			
		default:
			_Inherited::MessageReceived(message);
	}
}


// -------------------------------------------------------- //
// archiving/instantiation
// -------------------------------------------------------- //

ValControl::ValControl(BMessage* archive)
	: BControl(archive),
	fDirty(true)
{
	// fetch parameters
	archive->FindInt32("updateMode", (int32*)&fUpdateMode);
	archive->FindInt32("alignMode", (int32*)&fAlignMode);
	archive->FindInt32("alignFlags", (int32*)&fAlignFlags);

	// original bounds
	archive->FindRect("origBounds", &fOrigBounds);
}


status_t
ValControl::Archive(BMessage* archive, bool deep) const
{
	status_t err = _Inherited::Archive(archive, deep);
	
	// write parameters
	if (err == B_OK)
		err = archive->AddInt32("updateMode", (int32)fUpdateMode);
	if (err == B_OK)
		err = archive->AddInt32("alignMode", (int32)fAlignMode);
	if (err == B_OK)
		err = archive->AddInt32("alignFlags", (int32)fAlignFlags);
	if (err == B_OK)
		err = archive->AddRect("origBounds", fOrigBounds);
	if (err < B_OK)
		return err;

	// write layout set?
	if (!deep)
		return B_OK;

	// yes; spew it:
	for (layout_set::const_iterator it = fLayoutSet.begin();
		it != fLayoutSet.end(); it++) {

		// archive entry
		BMessage layoutSet;
		ASSERT((*it).pView);
		err = (*it).pView->Archive(&layoutSet, true);
		ASSERT(err == B_OK);
		
		// write it
		archive->AddMessage("layoutSet", &layoutSet);
	}

	return B_OK;
}


// -------------------------------------------------------- //
// internal operations
// -------------------------------------------------------- //

// add segment view (which is responsible for generating its
// own ValCtrlLayoutEntry)
void
ValControl::_Add(ValControlSegment* segment, entry_location from,
	uint16 distance)
{
	BWindow* pWnd = Window();
	if(pWnd)
		pWnd->BeginViewTransaction();
	
	AddChild(segment);
	
	segment->SetFont(&fValueFont);
	segment->fontChanged(&fValueFont);
	
	uint16 nIndex = _LocationToIndex(from, distance);
	ValCtrlLayoutEntry entry = segment->makeLayoutEntry();
	_InsertEntry(entry, nIndex);
//	linkSegment(segment, nIndex);
	
	if (pWnd)
		pWnd->EndViewTransaction();
}


// add general view (manipulator, label, etc.)
// the entry's frame rectangle will be filled in
void
ValControl::_Add(ValCtrlLayoutEntry& entry, entry_location from)
{
	BWindow* window = Window();
	if (window)
		window->BeginViewTransaction();

	if (entry.pView)	
		AddChild(entry.pView);

	uint16 index = _LocationToIndex(from, 0);
	_InsertEntry(entry, index);

	if (window)
		window->EndViewTransaction();
}


// access child-view ValCtrlLayoutEntry
// (_IndexOf returns index from left)
const ValCtrlLayoutEntry&
ValControl::_EntryAt(entry_location from, uint16 distance) const
{
	uint16 nIndex = _LocationToIndex(from, distance);
	ASSERT(nIndex < fLayoutSet.size());
	return fLayoutSet[nIndex];
}


const ValCtrlLayoutEntry&
ValControl::_EntryAt(uint16 offset) const
{
	uint16 nIndex = _LocationToIndex(FROM_LEFT, offset);
	ASSERT(nIndex < fLayoutSet.size());
	return fLayoutSet[nIndex];
}


uint16
ValControl::_IndexOf(BView* child) const
{
	for (uint16 n = 0; n < fLayoutSet.size(); n++) {
		if (fLayoutSet[n].pView == child)
			return n;
	}

	ASSERT(!"shouldn't be here");
	return 0;
}


uint16
ValControl::CountEntries() const
{
	return fLayoutSet.size();
}


// pop up keyboard input field +++++
void
ValControl::showEditField()
{
	BString valueString;

#if defined(DEBUG)
	status_t err = getString(valueString);
	ASSERT(err == B_OK);
#endif	// DEBUG
	
	BRect f = Bounds().OffsetByCopy(4.0, -4.0);
	ConvertToScreen(&f);
	//PRINT((
	//"# ValControl::showEditField(): base bounds (%.1f, %.1f)-(%.1f,%.1f)\n",
	//f.left, f.top, f.right, f.bottom));
	
	new TextControlFloater(f, B_ALIGN_RIGHT, &fValueFont, valueString.String(),
		BMessenger(this), new BMessage(M_SET_VALUE));
		// TextControlFloater embeds new value
		// in message: _value (string) +++++ DO NOT HARDCODE
}


//! (Re-)initialize backbuffer
void
ValControl::_AllocBackBuffer(float width, float height)
{
	ASSERT(fHaveBackBuffer);
	if (fBackBuffer && fBackBuffer->Bounds().Width() >= width
		&& fBackBuffer->Bounds().Height() >= height)
		return;

	if (fBackBuffer) {
		delete fBackBuffer;
		fBackBuffer = NULL;
		fBackBufferView = NULL;
	}

	BRect bounds(0, 0, width, height);
	fBackBuffer = new BBitmap(bounds, B_RGB32, true);
	fBackBufferView = new BView(bounds, "back", B_FOLLOW_NONE, B_WILL_DRAW);
	fBackBuffer->AddChild(fBackBufferView);
}


// ref'd view must already be a child +++++
// (due to GetPreferredSize implementation in segment views)
void
ValControl::_InsertEntry(ValCtrlLayoutEntry& entry, uint16 index)
{
	// view ptr must be 0, or a ValControlSegment that's already a child
	ValControlSegment* pSeg = dynamic_cast<ValControlSegment*>(entry.pView);
	if (entry.pView)
		ASSERT(pSeg);
	if (pSeg)
		ASSERT(this == pSeg->Parent());

	// entry must be at one side or the other:
	ASSERT(!index || index == fLayoutSet.size());

	// figure padding
	bool bNeedsPadding =
		!(entry.flags & ValCtrlLayoutEntry::LAYOUT_NO_PADDING ||
		 ((index - 1 >= 0 &&
		  fLayoutSet[index - 1].flags & ValCtrlLayoutEntry::LAYOUT_NO_PADDING)) ||
		 ((index + 1 < static_cast<uint16>(fLayoutSet.size()) &&
		  fLayoutSet[index + 1].flags & ValCtrlLayoutEntry::LAYOUT_NO_PADDING)));		  

	entry.fPadding = (bNeedsPadding) ? fSegmentPadding : 0.0;		

	// fetch (and grant) requested frame size
	BRect frame(0, 0, 0, 0);
	if (pSeg)
		pSeg->GetPreferredSize(&frame.right, &frame.bottom);
	else
		_GetDefaultEntrySize(entry.type, &frame.right, &frame.bottom);

	// figure amount this entry will displace:
	float fDisplacement = frame.Width() + entry.fPadding + 1;

	// set entry's top-left position:
	if (!fLayoutSet.size()) {
		// sole entry:
		if (fAlignMode == ALIGN_FLUSH_RIGHT)
			frame.OffsetBy(Bounds().right - frame.Width(), 0.0);
	} else if (index) {
		// insert at right side
		if (fAlignMode == ALIGN_FLUSH_LEFT)
			frame.OffsetBy(fLayoutSet.back().frame.right + 1 + entry.fPadding, 0.0);
		else
			frame.OffsetBy(fLayoutSet.back().frame.right - frame.Width(), 0.0); //+++++
	} else {
		// insert at left side
		if (fAlignMode == ALIGN_FLUSH_RIGHT)
			frame.OffsetBy(fLayoutSet.front().frame.left - fDisplacement, 0.0);
	}

	// add to layout set
	entry.frame = frame;
	fLayoutSet.insert(
		index ? fLayoutSet.end() : fLayoutSet.begin(),
		entry);

	// slide following or preceding entries (depending on align mode)
	// to make room:
	switch (fAlignMode) {
		case ALIGN_FLUSH_LEFT:
			// following entries are shifted to the right
			for(uint32 n = index+1; n < fLayoutSet.size(); n++)
				_SlideEntry(n, fDisplacement);
			break;
			
		case ALIGN_FLUSH_RIGHT: {
			// preceding entries are shifted to the left
			for(int n = index-1; n >= 0; n--)
				_SlideEntry(n, -fDisplacement);

			break;
		}
	}
//	
//	PRINT((
//		"### added entry: (%.1f,%.1f)-(%.1f,%.1f)\n",
//		frame.left, frame.top, frame.right, frame.bottom));
}


void
ValControl::_SlideEntry(int index, float delta)
{
	ValCtrlLayoutEntry& e = fLayoutSet[index];
	e.frame.OffsetBy(delta, 0.0);

	// move & possibly resize view:
	if (e.pView) {
		e.pView->MoveTo(e.frame.LeftTop());

		BRect curFrame = e.pView->Frame();
		float fWidth = e.frame.Width();
		float fHeight = e.frame.Height();
		if (curFrame.Width() != fWidth
			|| curFrame.Height() != fHeight)
			e.pView->ResizeTo(fWidth + 5.0, fHeight);
	}
}


uint16
ValControl::_LocationToIndex(entry_location from, uint16 distance) const
{
	uint16 nResult = 0;

	switch (from) {
		case FROM_LEFT:
			nResult = distance;
			break;

		case FROM_RIGHT:
			nResult = fLayoutSet.size() - distance;
			break;
	}

	ASSERT(nResult <= fLayoutSet.size());
	return nResult;
}


void
ValControl::_GetDefaultEntrySize(ValCtrlLayoutEntry::entry_type type,
	float* outWidth, float* outHeight)
{
	switch (type) {
		case ValCtrlLayoutEntry::SEGMENT_ENTRY:
		case ValCtrlLayoutEntry::VIEW_ENTRY:
			*outWidth = 1.0;
			*outHeight = 1.0;
			break;

		case ValCtrlLayoutEntry::DECIMAL_POINT_ENTRY:
			*outWidth = fDecimalPointWidth;
			*outHeight = fDecimalPointHeight;
			break;
	}
}
