// ValControlSegment.h
// +++++ cortex integration 23aug99:
//       - allow adjustment of dragScaleFactor
//       
// e.moon 17jan99
//
// ABSTRACT CLASS: ValControlSegment
// Represents a single manipulable (manipulatable?) segment of
// a ValControl interface element.  
//
// CLASS: ValControlDigitSegment
// Extends ValControlSegment to provide a single or multi-digit
// numeric segment.
//
// CLASS: ValControlStringSegment
// Extends ValControlSegment to provide a string-selection
// segment.  [+++++ driven by BMenu?]

#ifndef __ValControlSegment_H__
#define __ValControlSegment_H__

#include <View.h>

// possibly 'fed' by a BMenu?
class BMenu;

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// forward declarations
class ValControl;
class ValCtrlLayoutEntry;

// ---------------------------------------------------------------- //
// +++++ base 'manipulatable segment' class

// Might have some functionality:
// - underline (overridable)
// - chaining: segments need to inform segments to their left
//             when they 'roll around', using the M_INCREMENT and
//             M_DECREMENT messages.

class ValControlSegment : public BView {
	typedef BView _inherited;

	friend class ValControl;
		
public:													// constants
	
	enum underline_style {
		NO_UNDERLINE,
		DOTTED_UNDERLINE,
		SOLID_UNDERLINE
	};
	
	// mouse-tracking
	enum track_flags {
		TRACK_HORIZONTAL		=1,
		TRACK_VERTICAL			=2
	};

protected:											// pure virtuals
	// fetch layout entry (called by ValControl)
	virtual ValCtrlLayoutEntry makeLayoutEntry()=0;
	
public:													// hooks

	// * mouse-tracking callbacks
	
	// do any font-related layout work
	virtual void fontChanged(
		const BFont*								font) {}

	// return 'unused pixels' (if value updates are triggered
	// at less than one per pixel)
	virtual float handleDragUpdate(
		float												distance) { return 0; }

	virtual void mouseReleased() {}
	
	// underline size (tweak if you must)
	// +++++ 18sep99: 'ewww.'
	virtual void sizeUnderline(
		float*											outLeft,
		float*											outRight);
	
public:													// ctor/dtor/accessors
	virtual ~ValControlSegment();
	
protected:											// ctor/internal operations
	ValControlSegment(
		underline_style							underlineStyle);

	// get parent
	ValControl* parent() const;

	// init mouse tracking: must be called from MouseDown()
	void trackMouse(
		BPoint											point,
		track_flags									flags);
	
	void setTracking(
		bool												tracking);
	bool isTracking() const;
	
	// fetch pixel:unit ratio for dragging
	double dragScaleFactor() const;
	
public:													// BView impl.
	// calls sizeUnderline()
	virtual void AttachedToWindow();

	virtual void Draw(
		BRect												updateRect);
	
	// calls sizeUnderline() after bounds changed
	virtual void FrameResized(
		float												width,
		float												height);

	// calls trackMouse(TRACK_VERTICAL) if left button down
	virtual void MouseDown(
		BPoint											point);
	
	// feeds trackUpdate()
	virtual void MouseMoved(
		BPoint											point,
		uint32											transit,
		const BMessage*							dragMessage);
	
	// triggers mouseReleased()
	virtual void MouseUp(
		BPoint											point);
	
public:						// BHandler impl.
	virtual void MessageReceived(
		BMessage*										message); //nyi

public:						// archiving/instantiation
	ValControlSegment(
		BMessage*										archive);
	virtual status_t Archive(
		BMessage*										archive,
		bool												deep=true) const;

private:						// guts
	// ctor helper
	void init();

//	// left segment (ValControl)
//	ValControlSegment*		m_pLeft;
//
	// mouse-tracking machinery
	track_flags										m_trackFlags;
	bool													m_tracking;
	BPoint												m_prevPoint;
	double												m_dragScaleFactor;
	
	bigtime_t											m_lastClickTime;

	// look'n'feel parameters	
	underline_style								m_underlineStyle;
	float													m_xUnderlineLeft, m_xUnderlineRight;
	
	// constants
	static const double						s_defDragScaleFactor;
};

__END_CORTEX_NAMESPACE
#endif /* __ValControlSegment_H__ */
