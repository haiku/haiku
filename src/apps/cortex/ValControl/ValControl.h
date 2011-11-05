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
//   draggable.  Subclasses may mix segment types, or Add
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
#ifndef VAL_CONTROL_H
#define VAL_CONTROL_H


#include "cortex_defs.h"

#include "ValCtrlLayoutEntry.h"

#include <Bitmap.h>
#include <Control.h>
#include <Font.h>
#include <Message.h>
#include <View.h>

#include <vector>

__BEGIN_CORTEX_NAMESPACE

class ValControlSegment;


/* abstract */
class ValControl : public BControl {
	public:
		typedef BControl _Inherited;

	public:													// types & constants
		// child-view alignment options:
		enum align_mode {
			ALIGN_FLUSH_LEFT,
			ALIGN_FLUSH_RIGHT
		};
	
		// alignment flags +++++ 23aug99: not implemented -- should they be?
		enum align_flags {
			ALIGN_NONE = 0,
			ALIGN_GROW = 1
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

	public:	// messages (all ValControl-related messages go here!)
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

	public: // hooks
	
		// * parameter-mode value access

		// called to set the control's value from raw BParameter data
		virtual void setValue(const void* data, size_t size) = 0;

		// called to request the control's value in raw form
		virtual void getValue(void* data, size_t* ioSize) = 0;

		// * string value access

		virtual status_t setValueFrom(const char* text) = 0;

		virtual status_t getString(BString& buffer) = 0;

		// called when a child view's preferred size has changed;
		// it's up to the ValControl to grant the resize request.
		// Return true to notify the child that the request has
		// been granted, or false if denied (the default.)

		virtual bool childResizeRequest(BView* child) { return false; }

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
		void setUpdateMode(update_mode mode);

		// +++++ get/set font used by segments
		// (would separate 'value' and 'label' fonts be a good move?)
		//	const BFont* font() const;

		const BFont* labelFont() const; //nyi
		void setLabelFont(const BFont* labelFont); //nyi

		const BFont* valueFont() const; //nyi
		void setValueFont(const BFont* valueFont); //nyi

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

	public: // debugging [23aug99]
		virtual void dump();

	public: // BControl impl.
		// SetValue(), Value() aren't defined, since they only support
		//   32-bit integer values.  TextControl provides a precedent for
		//   this kind of naughtiness.

		// empty implementation (hands off to BControl)
		virtual void SetEnabled(bool enabled);

	public: // BView impl.

		// handle initial layout stuff:
		virtual void AttachedToWindow();
		virtual void AllAttached();

		// paint decorations (& decimal point)
		virtual void Draw(BRect updateRect);

		virtual void drawDecimalPoint(ValCtrlLayoutEntry& entry);

		// handle frame resize (grow backbuffer if need be)
		virtual void FrameResized(float width, float height);

		// calculate minimum size
		virtual void GetPreferredSize(float* outWidth, float* outHeight);
		
		virtual void MakeFocus(bool focused = true);
		
		virtual void MouseDown(BPoint where);

	public:
		virtual void MessageReceived(BMessage* message);

	public:						// archiving/instantiation
		ValControl(BMessage* archive);

		status_t Archive(BMessage* archive, bool deep = true) const;

	protected: // internal ctor/operations
		ValControl(BRect frame, const char* name, const char* label, 
			BMessage* message, align_mode alignMode, align_flags alignFlags,
			update_mode updateMode = UPDATE_ASYNC, bool backBuffer = true);

		// Add segment view (which is responsible for generating its
		// own ValCtrlLayoutEntry)
		void _Add(ValControlSegment* segment, entry_location from,
			uint16 distance = 0);

		// Add general view (manipulator, label, etc.)
		// (the entry's frame rectangle will be filled in)
		// covers ValCtrlLayoutEntry ctor:
		void _Add(ValCtrlLayoutEntry& entry, entry_location from);

		// access child-view ValCtrlLayoutEntry
		// (_IndexOf returns index from left)
		const ValCtrlLayoutEntry& _EntryAt(uint16 offset) const;

		const ValCtrlLayoutEntry& _EntryAt(entry_location from,
			uint16 distance = 0) const;

		uint16 _IndexOf(BView* child) const;

		uint16 CountEntries() const;

	private: // steaming entrails
		// (re-)initialize the backbuffer
		void _AllocBackBuffer(float width, float height);

		// insert a layout entry in ordered position (doesn't call
		// AddChild())
		void _InsertEntry(ValCtrlLayoutEntry& entry, uint16 index);

		// move given entry horizontally (update child view's position
		// and size as well, if any)
		void _SlideEntry(int index, float delta);
	
		// turn entry_location/offset into an index:
		uint16 _LocationToIndex(entry_location from, uint16 distance = 0) const;

		void _GetDefaultEntrySize(ValCtrlLayoutEntry::entry_type type,
			float* outWidth, float* outHeight);

		void _InvalidateAll();
	
	private:
		// the set of visible segments and other child views,
		// in left-to-right. top-to-bottom order
		typedef std::vector<ValCtrlLayoutEntry>		layout_set;
		layout_set			fLayoutSet;

		// true if value has been changed since last request
		// (allows caching of value)
		bool				fDirty;

		// when should messages be sent to the target?
		update_mode			fUpdateMode;

		BFont				fLabelFont;
		BFont				fValueFont;

		align_mode			fAlignMode;
		align_flags			fAlignFlags;

		// the bounds rectangle requested upon construction.
		// if the ALIGN_GROW flag is set, the real bounds
		// rectangle may be wider
		BRect				fOrigBounds;

		// backbuffer (made available to segments for flicker-free
		// drawing)
		bool				fHaveBackBuffer;
		BBitmap*			fBackBuffer;
		BView*				fBackBufferView;

		static const float	fSegmentPadding;

		static const float	fDecimalPointWidth;
		static const float	fDecimalPointHeight;

	private:
		class fnInitChild;
};

__END_CORTEX_NAMESPACE
#endif	// VAL_CONTROL_H
