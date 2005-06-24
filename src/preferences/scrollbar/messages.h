//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		messages.h
//	Author:			MrSiggler
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	message defs
//  
//------------------------------------------------------------------------------
#ifndef SCROLLBAR_MESSAGES_H
#define SCROLLBAR_MESSAGES_H

const uint32 SCROLLBAR_DEFAULTS					= 'dflt';
const uint32 SCROLLBAR_REVERT					= 'rvrt';

const uint32 SCROLLBAR_ARROW_STYLE_NONE			= 'anon';
const uint32 SCROLLBAR_ARROW_STYLE_DOUBLE		= 'adbl';
const uint32 SCROLLBAR_ARROW_STYLE_SINGLE		= 'asng';

const uint32 SCROLLBAR_KNOB_TYPE_PROPORTIONAL	= 'ktpr';
const uint32 SCROLLBAR_KNOB_TYPE_FIXED			= 'ktfx';

const uint32 SCROLLBAR_KNOB_STYLE_FLAT			= 'ksfl';
const uint32 SCROLLBAR_KNOB_STYLE_DOT			= 'ksdt';
const uint32 SCROLLBAR_KNOB_STYLE_LINE			= 'ksln';

const uint32 SCROLLBAR_KNOB_SIZE_CHANGED		= 'ksch';

const uint32 TEST_MSG	=	'tstm';

#endif
