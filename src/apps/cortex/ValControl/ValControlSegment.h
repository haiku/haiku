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
#ifndef VAL_CONTROL_SEGMENT_H
#define VAL_CONTROL_SEGMENT_H


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
	public:
		typedef BView _Inherited;

		friend class ValControl;
		
	public:

		enum underline_style {
			NO_UNDERLINE,
			DOTTED_UNDERLINE,
			SOLID_UNDERLINE
		};

		// mouse-tracking
		enum track_flags {
			TRACK_HORIZONTAL = 1,
			TRACK_VERTICAL = 2
		};

	protected:											// pure virtuals
		// fetch layout entry (called by ValControl)
		virtual ValCtrlLayoutEntry makeLayoutEntry() = 0;

	public:													// hooks

		// * mouse-tracking callbacks

		// do any font-related layout work
		virtual void fontChanged(const BFont* font) {}

		// return 'unused pixels' (if value updates are triggered
		// at less than one per pixel)
		virtual float handleDragUpdate(float distance) { return 0; }

		virtual void mouseReleased() {}

		// underline size (tweak if you must)
		// +++++ 18sep99: 'ewww.'
		virtual void sizeUnderline(float* outLeft, float* outRight);

	public:
		virtual ~ValControlSegment();

	protected: // ctor/internal operations
		ValControlSegment(underline_style underlineStyle);

		// get parent
		ValControl* parent() const;

		// init mouse tracking: must be called from MouseDown()
		void trackMouse(BPoint point, track_flags flags);

		void setTracking(bool tracking);
		bool isTracking() const;

		// fetch pixel:unit ratio for dragging
		double dragScaleFactor() const;

	public:// BView impl.
		// calls sizeUnderline()
		virtual void AttachedToWindow();

		virtual void Draw(BRect updateRect);

		// calls sizeUnderline() after bounds changed
		virtual void FrameResized(float width, float height);

		// calls trackMouse(TRACK_VERTICAL) if left button down
		virtual void MouseDown(BPoint point);

		// feeds trackUpdate()
		virtual void MouseMoved(BPoint point, uint32 transit,
			const BMessage* dragMessage);

		// triggers mouseReleased()
		virtual void MouseUp(BPoint point);

	public:
		virtual void MessageReceived(BMessage* message); //nyi

	public:
		ValControlSegment(BMessage* archive);
		virtual status_t Archive(BMessage* archive, bool deep = true) const;

	private:
		void _Init();

		// left segment (ValControl)
		// ValControlSegment*		m_pLeft;

		// mouse-tracking machinery
		track_flags				fTrackFlags;
		bool					fTracking;
		BPoint					fPrevPoint;
		double					fDragScaleFactor;

		bigtime_t				fLastClickTime;

		// look'n'feel parameters	
		underline_style			fUnderlineStyle;
		float					fXUnderlineLeft;
		float					fXUnderlineRight;

		// constants
		static const double		fDefDragScaleFactor;
};

__END_CORTEX_NAMESPACE
#endif	// VAL_CONTROL_SEGMENT_H
