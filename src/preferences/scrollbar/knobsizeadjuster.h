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
//	File Name:		knobsizeadjuster.h
//	Author:			MrSiggler
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Control for changing the minimum size of thumb for scrollbars
//  
//------------------------------------------------------------------------------
#ifndef SCROLLBAR_KNOB_ADJUSTER_H
#define SCROLLBAR_KNOB_ADJUSTER_H

#include <Control.h>

class KnobSizeAdjuster : public BControl
{
public:
	KnobSizeAdjuster( BPoint position, uint32 size );
		
	void SetKnobSize( int32 size );	
	void SetPressed( bool pressed );
	void SetTracking( bool tracking );
	bool IsPressed();
	bool IsTracking();
	
	int32 KnobSize(void) const { return fKnobSize; }
	
private:
	
	void Draw( BRect updateRect );
	void MouseDown( BPoint point );
	void MouseUp( BPoint point );
	void MouseMoved( BPoint point, uint32 transit, const BMessage *message );
	
	int32	fKnobSize; 
	bool	fPressed;
	bool	fTracking;
};

#endif