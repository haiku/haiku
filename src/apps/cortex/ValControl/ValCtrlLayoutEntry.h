// ValCtrlValCtrlLayoutEntry.h
// +++++ cortex integration 23aug99:
//       hide this class!
//
// e.moon 28jan99

#ifndef __VALCTRLLAYOUTENTRY_H__
#define __VALCTRLLAYOUTENTRY_H__

#include <View.h>

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// a ValCtrlLayoutEntry manages the layout state information for
// a single child view

class ValCtrlLayoutEntry {
	friend class ValControl;

public:										// static instances
	static ValCtrlLayoutEntry decimalPoint;

public:
	enum entry_type {
		SEGMENT_ENTRY,
		VIEW_ENTRY,
		DECIMAL_POINT_ENTRY
	};

	enum layout_flags {
		LAYOUT_NONE					=0,
		LAYOUT_NO_PADDING		=1,	// disables left/right padding
	};

public:
	// stl compatibility ctor
	ValCtrlLayoutEntry() {}
		
	// standard: yes, the frame is left invalid; that's for
	// ValControl::insertEntry() to fix
	ValCtrlLayoutEntry(BView* _pView, entry_type t,
		layout_flags f=LAYOUT_NONE) :
		
		pView(_pView),
		type(t),
		flags(f),
		fPadding(0.0) {}
		
	ValCtrlLayoutEntry(entry_type t,
		layout_flags f=LAYOUT_NONE) :
		
		pView(0),
		type(t),
		flags(f),
		fPadding(0.0) {}
			
	ValCtrlLayoutEntry(const ValCtrlLayoutEntry& clone) {
		operator=(clone);
	}
		
	ValCtrlLayoutEntry& operator=(const ValCtrlLayoutEntry& clone) {
		type = clone.type;
		flags = clone.flags;
		pView = clone.pView;
		
		frame = clone.frame;
		fPadding = clone.fPadding;
		return *this;
	}
		
	// order first by x position, then by y:
	bool operator<(const ValCtrlLayoutEntry& b) const {
		return frame.left < b.frame.left ||
			frame.top < b.frame.top;
	}

protected:
	// move & size ref'd child view to match 'frame'
	static void InitChildFrame(ValCtrlLayoutEntry& e);

public:

	BView*				pView;

	entry_type		type;
	layout_flags	flags;
		
	// layout state info (filled in by ValControl):
	BRect					frame;
	float					fPadding; 
};

__END_CORTEX_NAMESPACE
#endif /* __VALCTRLLAYOUTENTRY_H__ */
