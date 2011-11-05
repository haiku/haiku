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


// DiagramDefs.h (Cortex/DiagramView)
//
// * PURPOSE
//   Contains some global constants used by most of the
//   DiagramView classes, currently only message "what"s
//
// * HISTORY
//   c.lenz		25sep99		Begun
//
#ifndef DIAGRAM_DEFS_H
#define DIAGRAM_DEFS_H

#include "cortex_defs.h"
__BEGIN_CORTEX_NAMESPACE

// Message constants
enum message_t
{
	// Selection changed
	// - (DiagramItem *)		"item"
	// - (bool)					"replace"
	M_SELECTION_CHANGED		= DiagramView_message_base,

	// Diagram Box dragged
	// - (DiagramBox *)			"item"
	// - (BPoint)				"offset"
	M_BOX_DRAGGED,

	// Diagram Wire dragged
	// - (DiagramEndPoint *)	"from"
	M_WIRE_DRAGGED,

	// Diagram Wire dropped
	// - (DiagramEndPoint *)	"from"
	// - (DiagramEndPoint *)	"to"
	// - (bool)					"success"
	M_WIRE_DROPPED,

	// Diagram View 'Rect Tracking'
	// - (BPoint)				"origin"
	M_RECT_TRACKING
};

__END_CORTEX_NAMESPACE
#endif	// DIAGRAM_DEFS_H
