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


// DiagramBox.h (Cortex/DiagramView.h)
//
// * HISTORY
//   c.lenz		25sep99		Begun
//
#ifndef DIAGRAM_BOX_H
#define DIAGRAM_BOX_H

#include "cortex_defs.h"
#include "DiagramItem.h"
#include "DiagramItemGroup.h"

#include <Region.h>
#include <Window.h>


__BEGIN_CORTEX_NAMESPACE


class DiagramBox : public DiagramItem, public DiagramItemGroup {
	public:
		DiagramBox(BRect frame, uint32 flags = 0);
		virtual ~DiagramBox();

		virtual void DrawBox() = 0;
			// a hook functions that
			// is called from Draw() to do the actual drawing

		// derived from DiagramItemGroup

		virtual bool AddItem(DiagramItem *item);
		virtual bool RemoveItem(DiagramItem *item);

		// derived from DiagramItem

		// returns the Boxes frame rectangle
		BRect Frame() const
		{
			return fFrame;
		}

		void Draw(BRect updateRect);

		virtual void MouseDown(BPoint point, uint32 buttons, uint32 clicks);
		virtual void MouseOver(BPoint point, uint32 transit);
		
		virtual void MessageDragged(BPoint point, uint32 transit, const BMessage *message);
		virtual void MessageDropped(BPoint point, BMessage *message);

		void MoveBy(float x, float y, BRegion *updateRegion);
		virtual void ResizeBy(float horizontal, float vertical);
		
		enum flag_t {
			M_DRAW_UNDER_ENDPOINTS = 0x1
		};

	private:
		void _SetOwner(DiagramView *owner);

	private:
		BRect fFrame;
			// the boxes' frame rectangle

		// flags:
		// 	M_DRAW_UNDER_ENDPOINTS -  don't remove EndPoint frames from
		//							the clipping region
		uint32 fFlags;
};

__END_CORTEX_NAMESPACE

#endif	// DIAGRAM_BOX_H
