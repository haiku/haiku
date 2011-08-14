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


// ValCtrlValCtrlLayoutEntry.h
// +++++ cortex integration 23aug99:
//       hide this class!
//
// e.moon 28jan99
#ifndef VAL_CTRL_LAYOUT_ENTRY_H
#define VAL_CTRL_LAYOUT_ENTRY_H


#include "cortex_defs.h"

#include <View.h>


__BEGIN_CORTEX_NAMESPACE

// a ValCtrlLayoutEntry manages the layout state information for
// a single child view

class ValCtrlLayoutEntry {
	public:
		friend class ValControl;

	public: // static instances
		static ValCtrlLayoutEntry decimalPoint;

	public:
		enum entry_type {
			SEGMENT_ENTRY,
			VIEW_ENTRY,
			DECIMAL_POINT_ENTRY
		};

		enum layout_flags {
			LAYOUT_NONE	= 0,
			LAYOUT_NO_PADDING = 1,	// disables left/right padding
		};

	public:
		ValCtrlLayoutEntry() {}
			// stl compatibility ctor

		// standard: yes, the frame is left invalid; that's for
		// ValControl::insertEntry() to fix
		ValCtrlLayoutEntry(BView* _pView, entry_type t,
			layout_flags f = LAYOUT_NONE)
			:
			pView(_pView),
			type(t),
			flags(f),
			fPadding(0.0)
			{}

		ValCtrlLayoutEntry(entry_type t, layout_flags f = LAYOUT_NONE)
			:
			pView(0),
			type(t),
			flags(f),
			fPadding(0.0)
			{}
	
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
		BView* pView;

		entry_type type;
		layout_flags flags;

		// layout state info (filled in by ValControl):
		BRect frame;
		float fPadding; 
};

__END_CORTEX_NAMESPACE
#endif	// VAL_CTRL_LAYOUT_ENTRY_H
