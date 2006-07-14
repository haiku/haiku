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

// -------------------------------------------------------- //
// constants
// -------------------------------------------------------- //

const float ValControl::s_segmentPadding				= 2.0;

// the decimal point covers one more pixel x and y-ward:
const float ValControl::s_decimalPointWidth		= 2.0;
const float ValControl::s_decimalPointHeight		= 2.0;


// -------------------------------------------------------- //
// ctor/dtor/accessors
// -------------------------------------------------------- //

/*protected*/
ValControl::ValControl(
	BRect												frame,
	const char*									name,
	const char*									label, 
	BMessage*										message,
	align_mode									alignMode,
	align_flags									alignFlags,
	update_mode									updateMode,
	bool												backBuffer) :
		
	BControl(frame, name, label, message, B_FOLLOW_TOP|B_FOLLOW_LEFT,
		B_WILL_DRAW|B_FRAME_EVENTS),
	m_dirty(true),
	m_updateMode(updateMode),
	m_labelFont(be_bold_font),
	m_valueFont(be_bold_font),
	m_alignMode(alignMode),
	m_alignFlags(alignFlags),
	m_origBounds(Bounds()),
	m_haveBackBuffer(backBuffer),
	m_backBuffer(0),
	m_backBufferView(0) {

	if(m_haveBackBuffer)
		_allocBackBuffer(frame.Width(), frame.Height());

//	m_font.SetSize(13.0);	
//	rgb_color red = {255,0,0,255};
//	SetViewColor(red);
}

ValControl::~ValControl() {
	if(m_backBuffer)
		delete m_backBuffer;
}

// +++++ sync
ValControl::update_mode ValControl::updateMode() const {
	return m_updateMode;
}

// +++++ sync
void ValControl::setUpdateMode(
	update_mode									mode) {
	m_updateMode = mode;
}

//const BFont* ValControl::font() const {
//	return &m_font;
//}

const BFont* ValControl::labelFont() const { return &m_labelFont; }
void ValControl::setLabelFont(
	const BFont*								labelFont) {

	m_labelFont = labelFont;
	// +++++ inform label segments
	_invalidateAll();
}	

const BFont* ValControl::valueFont() const { return &m_valueFont; }
void ValControl::setValueFont(
	const BFont*								valueFont) {

	m_valueFont = valueFont;
	
	// inform value segments
	for(int n = countEntries(); n > 0; --n) {

		const ValCtrlLayoutEntry& e = entryAt(n-1);
		if(e.type != ValCtrlLayoutEntry::SEGMENT_ENTRY)
			continue;
			
		ValControlSegment* s = dynamic_cast<ValControlSegment*>(e.pView);
		ASSERT(s);
		s->SetFont(&m_valueFont);
		s->fontChanged(&m_valueFont);
	}
}

float ValControl::baselineOffset() const {
	font_height h;
	be_plain_font->GetHeight(&h);
	return ceil(h.ascent);
}

float ValControl::segmentPadding() const {
	return s_segmentPadding;
}

BView* ValControl::backBufferView() const {
	return m_backBufferView;
}
BBitmap* ValControl::backBuffer() const {
	return m_backBuffer;
}

// -------------------------------------------------------- //
// debugging [23aug99]
// -------------------------------------------------------- //

void ValControl::dump() {
	BRect f = Frame();
	PRINT((
		"*** ValControl::dump():\n"
		"    FRAME    (%.1f,%.1f)-(%.1f,%.1f)\n"
		"    ENTRIES:\n",
		f.left, f.top, f.right, f.bottom));
	for(layout_set::const_iterator it = m_layoutSet.begin();
		it != m_layoutSet.end(); ++it) {
		const ValCtrlLayoutEntry& e = *it;
		switch(e.type) {
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
		PRINT((
			"\n      cached frame (%.1f,%.1f)-(%.1f,%.1f) + pad(%.1f)\n",
			e.frame.left, e.frame.top, e.frame.right, e.frame.bottom,
			e.fPadding));
		if(
			e.type == ValCtrlLayoutEntry::SEGMENT_ENTRY ||
			e.type == ValCtrlLayoutEntry::VIEW_ENTRY) {
			if(e.pView) {
				PRINT((
					"      real frame   (%.1f,%.1f)-(%.1f,%.1f)\n\n",
					e.pView->Frame().left, e.pView->Frame().top,
					e.pView->Frame().right, e.pView->Frame().bottom));
			} else {
				PRINT((
					"      (no view!)\n\n"));
			}
		}
	}
	PRINT(("\n"));
}

// -------------------------------------------------------- //
// BControl impl.
// -------------------------------------------------------- //

void ValControl::SetEnabled(
	bool												enabled) {
//	PRINT((
//		"### SetEnabled(%s)\n"
//		"    (%.1f,%.1f)-(%.1f,%.1f)\n",
//		bEnabled?"true":"false",
//		Frame().left, Frame().top, Frame().right, Frame().bottom));
	
	// redraw if enabled-state changes
	_inherited::SetEnabled(enabled);

	_invalidateAll();
}

void ValControl::_invalidateAll() {
	Invalidate();
	int c = CountChildren();
	for(int n = 0; n < c; ++n) {
		ChildAt(n)->Invalidate();
	}
}

// -------------------------------------------------------- //
// BView impl
// -------------------------------------------------------- //

// handle initial layout stuff:
void ValControl::AttachedToWindow() {
	// adopt parent view's color
	if(Parent())
		SetViewColor(Parent()->ViewColor());
}

void ValControl::AllAttached() {
	// move children to requested positions
	BWindow* pWnd = Window();
	pWnd->BeginViewTransaction();
	
	for_each(m_layoutSet.begin(), m_layoutSet.end(),
		ptr_fun(&ValCtrlLayoutEntry::InitChildFrame)); // +++++?
	
	pWnd->EndViewTransaction();
}


// paint decorations (& decimal point)
void ValControl::Draw(
	BRect												updateRect) {
	
	// draw lightweight entries:
	for(unsigned int n = 0; n < m_layoutSet.size(); n++)
		if(m_layoutSet[n].type == ValCtrlLayoutEntry::DECIMAL_POINT_ENTRY)
			drawDecimalPoint(m_layoutSet[n]);
}

void ValControl::drawDecimalPoint(
	ValCtrlLayoutEntry&					e) {
	
	rgb_color dark = {0,0,0,255};
	rgb_color med = {200,200,200,255};
//	rgb_color light = {244,244,244,255};
	
	BPoint center;
	center.x = e.frame.left + 1;
	center.y = baselineOffset() - 1;
	
	SetHighColor(dark);
	StrokeLine(center, center);
	SetHighColor(med);
	StrokeLine(center-BPoint(0,1), center+BPoint(1,0));
	StrokeLine(center-BPoint(1,0), center+BPoint(0,1));

//	SetHighColor(light);
//	StrokeLine(center+BPoint(-1,1), center+BPoint(-1,1));
//	StrokeLine(center+BPoint(1,1), center+BPoint(1,1));
//	StrokeLine(center+BPoint(-1,-1), center+BPoint(-1,-1));
//	StrokeLine(center+BPoint(1,-1), center+BPoint(1,-1));
}

void ValControl::FrameResized(
	float												width,
	float												height) {
	
	_inherited::FrameResized(width,height);
	if(m_haveBackBuffer)
		_allocBackBuffer(width, height);
//
//	PRINT((
//		"# ValControl::FrameResized(): %.1f, %.1f\n",
//		width, height));
}

// calculate minimum size
void ValControl::GetPreferredSize(
	float*											outWidth,
	float*											outHeight) {
	ASSERT(m_layoutSet.size() > 0);
	
	*outWidth =
		m_layoutSet.back().frame.right -
		m_layoutSet.front().frame.left;
		
	*outHeight = 0;
	for(layout_set::const_iterator it = m_layoutSet.begin();
		it != m_layoutSet.end(); ++it) {
		if((*it).frame.Height() > *outHeight)
			*outHeight = (*it).frame.Height();
	}
//
//	PRINT((
//		"# ValControl::GetPreferredSize(): %.1f, %.1f\n",
//		*outWidth, *outHeight));
}

void ValControl::MakeFocus(
	bool												focused) {

	_inherited::MakeFocus(focused);

	// +++++ only the underline needs to be redrawn	
	_invalidateAll();
}

void ValControl::MouseDown(
	BPoint											where) {
	
	MakeFocus(true);
}

// -------------------------------------------------------- //
// BHandler impl.
// -------------------------------------------------------- //

void ValControl::MessageReceived(
	BMessage*										message) {
	status_t err;
	const char* stringValue;
	
//	PRINT((
//		"ValControl::MessageReceived():\n"));
//	message->PrintToStream();
	
	switch(message->what) {

		case M_SET_VALUE:
			err = message->FindString("_value", &stringValue);
			if(err < B_OK) {
				PRINT((
					"! ValControl::MessageReceived(): no _value found!\n"));
				break;
			}
				
			// set from string
			err = setValueFrom(stringValue);
			if(err < B_OK) {
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
			_inherited::MessageReceived(message);
	}
}

// -------------------------------------------------------- //
// archiving/instantiation
// -------------------------------------------------------- //

ValControl::ValControl(
	BMessage*										archive) :
	BControl(archive),
	m_dirty(true) {

	status_t err;
	
	// fetch parameters
	err = archive->FindInt32("updateMode", (int32*)&m_updateMode);
	ASSERT(err == B_OK);

	err = archive->FindInt32("alignMode", (int32*)&m_alignMode);
	ASSERT(err == B_OK);
	
	err = archive->FindInt32("alignFlags", (int32*)&m_alignFlags);
	ASSERT(err == B_OK);
	
	// original bounds
	err = archive->FindRect("origBounds", &m_origBounds);
	ASSERT(err == B_OK);
}

status_t ValControl::Archive(
	BMessage*										archive,
	bool												deep) const {
	_inherited::Archive(archive, deep);

	status_t err;
	
	// write parameters
	archive->AddInt32("updateMode", (int32)m_updateMode);
	archive->AddInt32("alignMode", (int32)m_alignMode);
	archive->AddInt32("alignFlags", (int32)m_alignFlags);

	archive->AddRect("origBounds", m_origBounds);

	// write layout set?
	if(!deep)
		return B_OK;
	
	// yes; spew it:
	for(layout_set::const_iterator it = m_layoutSet.begin();
		it != m_layoutSet.end(); it++) {

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
void ValControl::add(
	ValControlSegment*					segment,
	entry_location							from,
	uint16											distance) {

	BWindow* pWnd = Window();
	if(pWnd)
		pWnd->BeginViewTransaction();
	
	AddChild(segment);
	
	segment->SetFont(&m_valueFont);
	segment->fontChanged(&m_valueFont);
	
	uint16 nIndex = _locationToIndex(from, distance);
	ValCtrlLayoutEntry entry = segment->makeLayoutEntry();
	_insertEntry(entry, nIndex);
//	linkSegment(segment, nIndex);
	
	if(pWnd)
		pWnd->EndViewTransaction();
}

// add general view (manipulator, label, etc.)
// the entry's frame rectangle will be filled in
void ValControl::add(
	ValCtrlLayoutEntry&					entry,
	entry_location							from) {

	BWindow* pWnd = Window();
	if(pWnd)
		pWnd->BeginViewTransaction();

	if(entry.pView)	
		AddChild(entry.pView);
		
	uint16 nIndex = _locationToIndex(from, 0);
	_insertEntry(entry, nIndex);
	
	if(pWnd)
		pWnd->EndViewTransaction();
}

// access child-view ValCtrlLayoutEntry
// (indexOf returns index from left)
const ValCtrlLayoutEntry& ValControl::entryAt(
	entry_location							from,
	uint16											distance) const {

	uint16 nIndex = _locationToIndex(from, distance);
	ASSERT(nIndex < m_layoutSet.size());
	return m_layoutSet[nIndex];
}

const ValCtrlLayoutEntry& ValControl::entryAt(
	uint16											offset) const {
	
	uint16 nIndex = _locationToIndex(FROM_LEFT, offset);
	ASSERT(nIndex < m_layoutSet.size());
	return m_layoutSet[nIndex];
}
	
uint16 ValControl::indexOf(
	BView*											child) const {

	for(uint16 n = 0; n < m_layoutSet.size(); n++)
		if(m_layoutSet[n].pView == child)
			return n;
			
	ASSERT(!"shouldn't be here");
	return 0;
}

uint16 ValControl::countEntries() const {
	return m_layoutSet.size();
}


// pop up keyboard input field +++++
void ValControl::showEditField() {

	BString valueString;
	status_t err = getString(valueString);
	ASSERT(err == B_OK);
	
	BRect f = Bounds().OffsetByCopy(4.0, -4.0);
	ConvertToScreen(&f);
//	PRINT((
//		"# ValControl::showEditField(): base bounds (%.1f, %.1f)-(%.1f,%.1f)\n",
//		f.left, f.top, f.right, f.bottom));
		
	new TextControlFloater(
		f,
		B_ALIGN_RIGHT,
		&m_valueFont,
		valueString.String(),
		BMessenger(this),
		new BMessage(M_SET_VALUE)); // TextControlFloater embeds new value
																// in message: _value (string) +++++ DO NOT HARDCODE
}

// -------------------------------------------------------- //
// internal operation guts
// -------------------------------------------------------- //

// (re-)initialize backbuffer
void ValControl::_allocBackBuffer(
	float												width,
	float												height) {

	ASSERT(m_haveBackBuffer);
	if(m_backBuffer && m_backBuffer->Bounds().Width() >= width &&
		m_backBuffer->Bounds().Height() >= height)
		return;
		
	if(m_backBuffer) {
		delete m_backBuffer;
		m_backBuffer = 0;
		m_backBufferView = 0;
	}
	
	BRect bounds(0,0,width,height);
	m_backBuffer = new BBitmap(bounds, B_RGB32, true);
	ASSERT(m_backBuffer);
	m_backBufferView = new BView(bounds, "back", B_FOLLOW_NONE, B_WILL_DRAW);
	ASSERT(m_backBufferView);
	m_backBuffer->AddChild(m_backBufferView);
}


// ref'd view must already be a child +++++
// (due to GetPreferredSize implementation in segment views)
void ValControl::_insertEntry(
	ValCtrlLayoutEntry&					entry,
	uint16											index) {

	// view ptr must be 0, or a ValControlSegment that's already a child
	ValControlSegment* pSeg = dynamic_cast<ValControlSegment*>(entry.pView);
	if(entry.pView)
		ASSERT(pSeg);
	if(pSeg)
		ASSERT(this == pSeg->Parent());

	// entry must be at one side or the other:
	ASSERT(!index || index == m_layoutSet.size());
	
	// figure padding

	bool bNeedsPadding =
		!(entry.flags & ValCtrlLayoutEntry::LAYOUT_NO_PADDING ||
		 ((index-1 >= 0 &&
		  m_layoutSet[index-1].flags & ValCtrlLayoutEntry::LAYOUT_NO_PADDING)) ||
		 ((index+1 < m_layoutSet.size() &&
		  m_layoutSet[index+1].flags & ValCtrlLayoutEntry::LAYOUT_NO_PADDING)));		  

	entry.fPadding = (bNeedsPadding) ? s_segmentPadding : 0.0;		

	// fetch (and grant) requested frame size
	BRect frame(0,0,0,0);
	if(pSeg)
		pSeg->GetPreferredSize(&frame.right, &frame.bottom);
	else
		_getDefaultEntrySize(entry.type, &frame.right, &frame.bottom);

	// figure amount this entry will displace:
	float fDisplacement = frame.Width() + entry.fPadding + 1;

	// set entry's top-left position:
	if(!m_layoutSet.size()) {
		// sole entry:
		if(m_alignMode == ALIGN_FLUSH_RIGHT)
			frame.OffsetBy(Bounds().right - frame.Width(), 0.0);
	}
	else if(index) {
		// insert at right side
		if(m_alignMode == ALIGN_FLUSH_LEFT)
			frame.OffsetBy(m_layoutSet.back().frame.right + 1 + entry.fPadding, 0.0);
		else
			frame.OffsetBy(m_layoutSet.back().frame.right - frame.Width(), 0.0); //+++++
	}
	else {
		// insert at left side
		if(m_alignMode == ALIGN_FLUSH_RIGHT)
			frame.OffsetBy(m_layoutSet.front().frame.left - fDisplacement, 0.0);
	}
	
	// add to layout set
	entry.frame = frame;
	m_layoutSet.insert(
		index ? m_layoutSet.end() : m_layoutSet.begin(),
		entry);

	// slide following or preceding entries (depending on align mode)
	// to make room:
	switch(m_alignMode) {
		case ALIGN_FLUSH_LEFT:
			// following entries are shifted to the right
			for(int n = index+1; n < m_layoutSet.size(); n++)
				_slideEntry(n, fDisplacement);
			break;
			
		case ALIGN_FLUSH_RIGHT: {
			// preceding entries are shifted to the left
			for(int n = index-1; n >= 0; n--)
				_slideEntry(n, -fDisplacement);

			break;
		}
	}
//	
//	PRINT((
//		"### added entry: (%.1f,%.1f)-(%.1f,%.1f)\n",
//		frame.left, frame.top, frame.right, frame.bottom));
}

void ValControl::_slideEntry(
	int													index,
	float												delta) {
	
	ValCtrlLayoutEntry& e = m_layoutSet[index];
	e.frame.OffsetBy(delta, 0.0);

	// move & possibly resize view:
	if(e.pView) {
		e.pView->MoveTo(e.frame.LeftTop());
		
		BRect curFrame = e.pView->Frame();
		float fWidth = e.frame.Width();
		float fHeight = e.frame.Height();
		if(curFrame.Width() != fWidth ||
			curFrame.Height() != fHeight)
			e.pView->ResizeTo(fWidth + 5.0, fHeight);
	}
}


uint16 ValControl::_locationToIndex(
	entry_location							from,
	uint16											distance) const {

	uint16 nResult = 0;
	
	switch(from) {
		case FROM_LEFT:
			nResult = distance;
			break;
			
		case FROM_RIGHT:
			nResult = m_layoutSet.size() - distance;
			break;
	}
	
	ASSERT(nResult <= m_layoutSet.size());
	
	return nResult;
}

void ValControl::_getDefaultEntrySize(
	ValCtrlLayoutEntry::entry_type		type,
	float*											outWidth,
	float*											outHeight) {

	switch(type) {
		case ValCtrlLayoutEntry::SEGMENT_ENTRY:
		case ValCtrlLayoutEntry::VIEW_ENTRY:
			*outWidth = 1.0;
			*outHeight = 1.0;
			break;

		case ValCtrlLayoutEntry::DECIMAL_POINT_ENTRY:
			*outWidth = s_decimalPointWidth;
			*outHeight = s_decimalPointHeight;
			break;
	}
}

// END -- ValControl.cpp --
