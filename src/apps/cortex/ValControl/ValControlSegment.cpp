// ValControlSegment.cpp
// e.moon 20jan99


#include "ValControlSegment.h"
#include "ValControl.h"

#include <Debug.h>

#include <cstdio>

__USE_CORTEX_NAMESPACE

// -------------------------------------------------------- //
// static stuff
// -------------------------------------------------------- //

const double ValControlSegment::s_defDragScaleFactor = 4;

// -------------------------------------------------------- //
// dtor/accessors
// -------------------------------------------------------- //

ValControlSegment::~ValControlSegment() {}
	
// -------------------------------------------------------- //
// ctor/internal operations
// -------------------------------------------------------- //

ValControlSegment::ValControlSegment(
		underline_style							underlineStyle) :

	BView(
		BRect(),
		"ValControlSegment",
		B_FOLLOW_LEFT|B_FOLLOW_TOP,
		B_WILL_DRAW|B_FRAME_EVENTS),
	m_underlineStyle(underlineStyle),
	m_dragScaleFactor(s_defDragScaleFactor),
	m_lastClickTime(0LL),
	m_xUnderlineLeft(0.0),
	m_xUnderlineRight(0.0) {
	
	init();
}

void ValControlSegment::init() {
//	m_pLeft = 0;
}

// get parent
ValControl* ValControlSegment::parent() const {
	return dynamic_cast<ValControl*>(Parent());
}
//
//// send message -> segment to my left
//void ValControlSegment::sendMessageLeft(BMessage* pMsg) {
//	if(!m_pLeft)
//		return;
//	BMessenger m(m_pLeft);
//	if(!m.IsValid())
//		return;
//	m.SendMessage(pMsg);
//	delete pMsg;
//}
	
// init mouse tracking
void ValControlSegment::trackMouse(
	BPoint											point,
	track_flags									flags) {

	m_trackFlags = flags;
	m_prevPoint = point;
	setTracking(true);
	SetMouseEventMask(
		B_POINTER_EVENTS,
		B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
}

void ValControlSegment::setTracking(
	bool												tracking) {

	m_tracking = tracking;
}

bool ValControlSegment::isTracking() const {
	return m_tracking;
}

double ValControlSegment::dragScaleFactor() const {
	return m_dragScaleFactor;
}

// -------------------------------------------------------- //
// default hook implementations
// -------------------------------------------------------- //

void ValControlSegment::sizeUnderline(
	float*											outLeft,
	float*											outRight) {

	*outLeft = 0.0;
	*outRight = Bounds().right;
}

// -------------------------------------------------------- //
// BView impl.
// -------------------------------------------------------- //

void ValControlSegment::AttachedToWindow() {
//	if(parent()->backBuffer())
//		SetViewColor(B_TRANSPARENT_COLOR);

	// adopt parent's view color
	SetViewColor(parent()->ViewColor());
}

void ValControlSegment::Draw(
	BRect												updateRect) {

	if(m_underlineStyle == NO_UNDERLINE)	
		return;
	
	// +++++ move to layout function
	float fY = parent()->baselineOffset() + 1;
	
	rgb_color white = {255,255,255,255};
	rgb_color blue = {0,0,255,255};
	rgb_color gray = {140,140,140,255};
	rgb_color old = HighColor();

	SetLowColor(white);
	if(parent()->IsEnabled() && parent()->IsFocus())
		SetHighColor(blue);
	else
		SetHighColor(gray);

	if(m_xUnderlineRight <= m_xUnderlineLeft)
		sizeUnderline(&m_xUnderlineLeft, &m_xUnderlineRight);

//	PRINT((
//		"### ValControlSegment::Draw(): underline spans %.1f, %.1f\n",
//		m_xUnderlineLeft, m_xUnderlineRight));
//	
	StrokeLine(
		BPoint(m_xUnderlineLeft, fY),
		BPoint(m_xUnderlineRight, fY),
		(m_underlineStyle == DOTTED_UNDERLINE) ? B_MIXED_COLORS :
			B_SOLID_HIGH);
	SetHighColor(old);
}

void ValControlSegment::FrameResized(
	float												width,
	float												height) {
	_inherited::FrameResized(width, height);

//	PRINT((
//		"### ValControlSegment::FrameResized(%.1f,%.1f)\n",
//		fWidth, fHeight));
	sizeUnderline(&m_xUnderlineLeft, &m_xUnderlineRight);
}

void ValControlSegment::MouseDown(
	BPoint											point) {

	if(!parent()->IsEnabled())
		return;
		
	parent()->MakeFocus();

	// not left button?
	uint32 nButton;
	GetMouse(&point, &nButton);
	if(!(nButton & B_PRIMARY_MOUSE_BUTTON))
		return;
		
	// double click?
	bigtime_t doubleClickInterval;
	if(get_click_speed(&doubleClickInterval) < B_OK) {
		PRINT((
			"* ValControlSegment::MouseDown():\n"
			"  get_click_speed() failed."));
		return;
	}
	
	bigtime_t now = system_time();
	if(now - m_lastClickTime < doubleClickInterval) {
		if(isTracking()) {
			setTracking(false);
			mouseReleased();
		}

		// hand off to parent control
		parent()->showEditField();
		m_lastClickTime = 0LL;
		return;
	}
	else
		m_lastClickTime = now;

		
	// engage tracking
	trackMouse(point, TRACK_VERTICAL);
}

void ValControlSegment::MouseMoved(
	BPoint											point,
	uint32											transit,
	const BMessage*							dragMessage) {

	if(!parent()->IsEnabled())
		return;

	if(!isTracking() || m_prevPoint == point)
		return;

//	float fXDelta = m_trackFlags & TRACK_HORIZONTAL ?
//		point.x - m_prevPoint.x : 0.0;
	float fYDelta = m_trackFlags & TRACK_VERTICAL ?
		point.y - m_prevPoint.y : 0.0;

//	printf("MouseMoved(): %2f,%2f\n",
//		fXDelta, fYDelta);

	
	// hand off
	// +++++ config'able x/y response would be nice...
	float fRemaining = handleDragUpdate(fYDelta);
	
	m_prevPoint = point;
	m_prevPoint.y -= fRemaining;
}

void ValControlSegment::MouseUp(
	BPoint											point) {

	if(isTracking()) {
		setTracking(false);
		mouseReleased();
	}
}	
	
// -------------------------------------------------------- //
// BHandler impl.
// -------------------------------------------------------- //

void ValControlSegment::MessageReceived(
	BMessage*										message) {

	switch(message->what) {
		default:
			_inherited::MessageReceived(message);
	}
}

// -------------------------------------------------------- //
// archiving/instantiation
// -------------------------------------------------------- //

ValControlSegment::ValControlSegment(
	BMessage*										archive) :
	BView(archive) {

	init();
	archive->FindInt32("ulStyle", (int32*)&m_underlineStyle);	
}
	
status_t ValControlSegment::Archive(
	BMessage*										archive,
	bool												deep) const {
	_inherited::Archive(archive, deep);
	
	archive->AddInt32("ulStyle", m_underlineStyle);
	return B_OK;
}

// END -- ValControlSegment.cpp --
