// ValControl.h
// +++++ cortex integration 23aug99:
//       - way too many protected members
//       - config'able font
//       - implement GetPreferredSize() (should work before the view is attached
//         to a window -- doable?)
//       - keyboard entry (pop-up text field)
//       - 'spin' mode: value changes based on vertical distance of pointer
//         (configurable; set buttons/modifiers to activate either mode?)
//
//       - should parameter binding happen at this level?
//       +++++ how about a ValControlFactory?  give it a BParameter, get back a
//             ValControl subclass... +++++
//
// e.moon 16jan99
//
// ABSTRACT CLASS: ValControl
// An abstract base class for 'value controls' -- interface
// components that display a value that may be modified via
// click-drag.  Other editing methods (such as 'click, then
// type') may be supported by subclasses.
//
// IMPLEMENT
//   getValue() and setValue(), for raw (BParameter-style) value access
//   MessageReceived(), to handle:
//     M_SET_VALUE
//     M_GET_VALUE
//     M_OFFSET_VALUE		(May be sent by segments during mouse action,
//                       +++++ is a faster method needed?)
//
// NOTES
// The control view consists of:
//
// - One or more segments.  Each segment is individually
//   draggable.  Subclasses may mix segment types, or add
//   and remove segments dynamically.
//
// - A manipulator region, to which subcontrols (such as 'spin
//   arrows') may be added.
//
// Views/segments may be aligned flush left or flush right.  The
// side towards which views are aligned may be referred to as
// the 'anchor' side.
//
// Quickie interface guidelines:
//
// - Value controls are always underlined, indicating that the
//   value is editable.  (+++++ possible extension: dotted-underline
//   for continuous variation, and solid for discrete/step variation)
//
// - When a value control's 'click action' is to pop down a menu of
//   available choices (or pop up any sort of non-typable display)
//   this should be indicated by a small right triangle in the
//   manipulator area.
//   +++++ this may need some clarification; pop-down sliders, for example?
//
// * HISTORY
//   e.moon		19sep99		Cleanup
//   e.moon		23aug99		begun Cortex integration
//   e.moon		17jan99		started

#ifndef __ValControl_H__
#define __ValControl_H__

#include <vector>

#include <Bitmap.h>
#include <View.h>
#include <Font.h>
#include <Control.h>
#include <Message.h>

#include "ValCtrlLayoutEntry.h"

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

class ValControlSegment;

// ---------------------------------------------------------------- //


/* abstract */
class ValControl : public BControl {
	typedef BControl _inherited;

public:													// types & constants
	// child-view alignment options:
	enum align_mode {
		ALIGN_FLUSH_LEFT,
		ALIGN_FLUSH_RIGHT
	};
	
	// alignment flags +++++ 23aug99: not implemented -- should they be?
	enum align_flags {
		ALIGN_NONE 					= 0,
		ALIGN_GROW 					= 1
	};
	
	// should value update messages be sent asynchronously (during
	// a mouse operation) or synchronously (after the mouse is
	// released)?
	enum update_mode {
		UPDATE_ASYNC,
		UPDATE_SYNC
	};
	
	enum entry_location {
		LEFT_MOST = 0,
		FROM_LEFT = 0,
		RIGHT_MOST = 1,
		FROM_RIGHT = 1
	};

	// layout system state +++++

public:													// types

public:													// messages (all ValControl-related messages go here!)
	enum message_t {

		// Set value of a control or segment:
		// [your value field(s)] or "_value" (string)
		M_SET_VALUE					= ValControl_message_base,

		// Request for value of control/segment:		
		// [your value field(s)]
		M_GET_VALUE,

		// ... reply to M_GET_VALUE with this:
		// [your value field(s)]
		M_VALUE
	};
	
public:													// hooks

	// * parameter-mode value access

	// called to set the control's value from raw BParameter data
	virtual void setValue(
		const void*									data,
		size_t											size)=0;

	// called to request the control's value in raw form
	virtual void getValue(
		void*												data,
		size_t*											ioSize)=0;

	// * string value access

	virtual status_t setValueFrom(
		const char*									text)=0;

	virtual status_t getString(
		BString&										buffer)=0;

	// called when a child view's preferred size has changed;
	// it's up to the ValControl to grant the resize request.
	// Return true to notify the child that the request has
	// been granted, or false if denied (the default.)
	
	virtual bool childResizeRequest(
		BView*											child)	{ return false; }
	
public:													// ctor/dtor/accessors
	virtual ~ValControl();
	
	// value-access methods are left up to the subclasses,
	//   since they'll take varying types of arguments.
	//  (M_SET_VALUE and M_GET_VALUE should always behave
	//   as you'd expect, with a 'value' field of the appropriate
	//   type replacing or returning the current value.) +++++ decrepit
	//
	// Note that all implementations offering pop-up keyboard entry
	// must accept an M_SET_VALUE with a value of B_STRING_TYPE.
	
	// get/set update mode (determines whether value updates are
	// sent to the target during mouse operations, or only on
	// mouse-up)
	update_mode updateMode() const;
	void setUpdateMode(
		update_mode									mode);
	
	// +++++ get/set font used by segments
	// (would separate 'value' and 'label' fonts be a good move?)
//	const BFont* font() const;

	const BFont* labelFont() const; //nyi
	void setLabelFont(
		const BFont*								labelFont); //nyi

	const BFont* valueFont() const; //nyi
	void setValueFont(
		const BFont*								valueFont); //nyi

	// get baseline y offset: this is measured relative to the top of the
	// view
	float baselineOffset() const;
	
	// segment padding: this amount of padding is added to the
	// right of each segment bounds-rectangle
	float segmentPadding() const;
	
	// fast drag rate: returns ratio of pixels:units for fast
	// (left-button) dragging
	float fastDragRate() const; //nyi
	
	// slow drag rate: returns ratio for precise (both/middle-button)
	// dragging
	float slowDragRate() const; //nyi
	
	// fetch back-buffer
	BView* backBufferView() const; //nyi
	BBitmap* backBuffer() const; //nyi
	
	// pop up keyboard input field
	// +++++ should this turn into a message?
	virtual void showEditField();
	
public:													// debugging [23aug99]
	virtual void dump();

public:													// BControl impl.
	// SetValue(), Value() aren't defined, since they only support
	//   32-bit integer values.  TextControl provides a precedent for
	//   this kind of naughtiness.
	
	// empty implementation (hands off to BControl)
	virtual void SetEnabled(
		bool												enabled);

public:													// BView impl.

	// handle initial layout stuff:
	virtual void AttachedToWindow();
	virtual void AllAttached();
	
	// paint decorations (& decimal point)
	virtual void Draw(
		BRect												updateRect);

	virtual void drawDecimalPoint(
		ValCtrlLayoutEntry&					entry);
	
	// handle frame resize (grow backbuffer if need be)
	virtual void FrameResized(
		float												width,
		float												height);
	
	// calculate minimum size
	virtual void GetPreferredSize(
		float*											outWidth,
		float*											outHeight);
		
	virtual void MakeFocus(
		bool												focused=true);
		
	virtual void MouseDown(
		BPoint											where);

public:						// BHandler impl.
	virtual void MessageReceived(
		BMessage*										message);

public:						// archiving/instantiation
	ValControl(
		BMessage*										archive);

	status_t Archive(
		BMessage*										archive,
		bool												deep=true) const;

protected:					// internal ctor/operations
	ValControl(
		BRect												frame,
		const char*									name,
		const char*									label, 
		BMessage*										message,
		align_mode									alignMode,
		align_flags									alignFlags,
		update_mode									updateMode=UPDATE_ASYNC,
		bool												backBuffer=true);

	// add segment view (which is responsible for generating its
	// own ValCtrlLayoutEntry)
	void add(
		ValControlSegment*					segment,
		entry_location							from,
		uint16											distance=0);

	// add general view (manipulator, label, etc.)
	// (the entry's frame rectangle will be filled in)
	// covers ValCtrlLayoutEntry ctor:
	void add(
		ValCtrlLayoutEntry&					entry,
		entry_location							from);
	
	// access child-view ValCtrlLayoutEntry
	// (indexOf returns index from left)
	const ValCtrlLayoutEntry& entryAt(
		uint16											offset) const;

	const ValCtrlLayoutEntry& entryAt(
		entry_location							from,
		uint16											distance=0) const;

	uint16 indexOf(
		BView*											child) const;
		
	uint16 countEntries() const;

private:												// steaming entrails

	// (re-)initialize the backbuffer
	void _allocBackBuffer(
		float												width,
		float												height);

	// insert a layout entry in ordered position (doesn't call
	// AddChild())
	void _insertEntry(
		ValCtrlLayoutEntry&					entry,
		uint16											index);

	// move given entry horizontally (update child view's position
	// and size as well, if any)
	void _slideEntry(
		int													index,
		float												delta);
	
	// turn entry_location/offset into an index:
	uint16 _locationToIndex(
		entry_location							from,
		uint16											distance=0) const;

	void _getDefaultEntrySize(
		ValCtrlLayoutEntry::entry_type		type,
		float*											outWidth,
		float*											outHeight);
		
	void _invalidateAll();
	
protected:											// impl. members

	// the set of visible segments and other child views,
	// in left-to-right. top-to-bottom order
	typedef std::vector<ValCtrlLayoutEntry>		layout_set;
	layout_set										m_layoutSet;

	// true if value has been changed since last request
	// (allows caching of value)
	bool													m_dirty;

	// when should messages be sent to the target?
	update_mode										m_updateMode;

	// layout stuff
//	BFont													m_font;

	BFont													m_labelFont;
	BFont													m_valueFont;

	align_mode										m_alignMode;
	align_flags										m_alignFlags;
	
	// the bounds rectangle requested upon construction.
	// if the ALIGN_GROW flag is set, the real bounds
	// rectangle may be wider
	BRect													m_origBounds;
	
	// backbuffer (made available to segments for flicker-free
	// drawing)
	bool													m_haveBackBuffer;
	BBitmap*											m_backBuffer;
	BView*												m_backBufferView;
	
	static const float						s_segmentPadding;
	
	static const float						s_decimalPointWidth;
	static const float						s_decimalPointHeight;
	
private:												// guts
	class fnInitChild;
};

__END_CORTEX_NAMESPACE
#endif /* __ValControl_H__ */
