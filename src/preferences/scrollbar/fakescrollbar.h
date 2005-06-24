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
//	File Name:		fakescrollbar.h
//	Author:			MrSiggler
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Control to simulate a scrollbar
//  
//------------------------------------------------------------------------------
#ifndef SCROLLBAR_FAKE_H
#define SCROLLBAR_FAKE_H

#include <Control.h>

// These values are the same as what is kept in the scroll_bar_info struct.
enum
{
	THUMB_BLANK = 0,
	THUMB_DOTS,
	THUMB_LINES
};

class FakeScrollBar : public BControl
{
public:
				FakeScrollBar(	BPoint position, bool selected, int32 knobstyle, 
								bool fixedthumb, bool doublearrows, int32 size,
								BMessage *message );
				~FakeScrollBar(void);
	
	void		SetFromScrollBarInfo(const scroll_bar_info &info);
	
	void		SetDoubleArrows( bool doublearrows );	
	void		SetKnobSize( int32 size );	
	void		SetFixedThumb( bool fixed );
	void		SetKnobStyle( uint32 style );
	
	void		SetSelected( bool selected );	
	void		SetPressed( bool pressed );
	void		SetTracking( bool tracking );
	
	bool		IsPressed();
	bool		IsSelected();
	bool		IsTracking();
	
private:
	
	void		MouseDown( BPoint point );
	void		MouseUp( BPoint point );
	void		MouseMoved( BPoint point, uint32 transit, const BMessage *message );
	
	void		Draw( BRect updateRect );
	void		DrawArrowBox( uint32 x, bool facingleft );
	void		DrawDotGrabber( uint32 x, uint32 size );
	void		DrawLineGrabber( uint32 x, uint32 size );
	
	bool	 	fIsSelected;
	bool 		fIsPressed;
	bool		fIsTracking;
	
	int32		fKnobStyle; 
	
	bool		fDoubleArrows;
	bool		fFixedThumb;
	int32		fMinKnobSize;
};

#endif
