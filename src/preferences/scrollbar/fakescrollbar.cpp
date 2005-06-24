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
//	File Name:		fakescrollbar.cpp
//	Author:			MrSiggler
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Control to simulate a scrollbar
//  
//------------------------------------------------------------------------------
#include "fakescrollbar.h"
#include "messages.h"

FakeScrollBar::FakeScrollBar( 	BPoint position, bool selected, int32 knobstyle, 
								bool fixedthumb, bool doublearrow, int32 size, BMessage *message )
	
	: BControl(	BRect(position.x, position.y, position.x+132, position.y+20), "FakeScrollBar", NULL,
				message, B_FOLLOW_LEFT |	B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE)
{
	fIsSelected=selected;
	fIsPressed=false;
	fDoubleArrows=doublearrow;
	fFixedThumb=fixedthumb;
	
	fKnobStyle=knobstyle;
	fMinKnobSize=size;
}

FakeScrollBar::~FakeScrollBar(void)
{
}

void
FakeScrollBar::DrawArrowBox( uint32 x, bool facingleft )
{
	// draw box
	SetHighColor( 216, 216, 216 );
	FillRect( BRect(x, Bounds().top+3, x+16, Bounds().bottom-3), B_SOLID_HIGH );

	SetHighColor( 152, 152, 152 );
	StrokeRect( BRect(x, Bounds().top+3, x+16, Bounds().bottom-3 ) , B_SOLID_HIGH );		
		
	SetHighColor( 255, 255, 255 );
	StrokeLine( BPoint(x+1, Bounds().top+4), BPoint(x+15, Bounds().top+4), B_SOLID_HIGH );
	StrokeLine( BPoint(x+1, Bounds().top+4), BPoint(x+1, Bounds().bottom-4), B_SOLID_HIGH );

	SetHighColor( 192, 192, 192 );
	StrokeLine( BPoint(x+15, Bounds().bottom-4), BPoint(x+15, Bounds().top+4), B_SOLID_HIGH );
	StrokeLine( BPoint(x+15, Bounds().bottom-4), BPoint(x+1, Bounds().bottom-4), B_SOLID_HIGH );

	if ( facingleft )
	{		
		// draw arrow 	
		SetHighColor( 112, 112, 112 );
		StrokeLine( BPoint(x+4, Bounds().top+10), BPoint(x+11, Bounds().top+7), B_SOLID_HIGH );
		StrokeLine( BPoint(x+4, Bounds().top+10), BPoint(x+11, Bounds().bottom-7), B_SOLID_HIGH );
		StrokeLine( BPoint(x+11, Bounds().top+7), BPoint(x+11, Bounds().bottom-7), B_SOLID_HIGH );
	
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint(x+4, Bounds().top+11), BPoint(x+11, Bounds().bottom-6), B_SOLID_HIGH );
		StrokeLine( BPoint(x+12, Bounds().top+7), BPoint(x+12, Bounds().bottom-6), B_SOLID_HIGH );
			
		SetHighColor( 192, 192, 192 );
		StrokeLine( BPoint(x+4, Bounds().top+9), BPoint(x+11, Bounds().top+6), B_SOLID_HIGH );
		StrokeLine( BPoint(x+3, Bounds().top+10),	BPoint(x+3, Bounds().top+10), B_SOLID_HIGH );
		StrokeLine( BPoint(x+12, Bounds().top+6),	BPoint(x+12, Bounds().top+6), B_SOLID_HIGH );
	}
	else
	{		
		// draw arrow 	
		SetHighColor( 112, 112, 112 );
		StrokeLine( BPoint(x+12, Bounds().top+10), BPoint(x+5, Bounds().top+7), B_SOLID_HIGH );
		StrokeLine( BPoint(x+12, Bounds().top+10), BPoint(x+5, Bounds().bottom-7), B_SOLID_HIGH );
		StrokeLine( BPoint(x+5, Bounds().top+7), BPoint(x+5, Bounds().bottom-7), B_SOLID_HIGH );
	
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint(x+12, Bounds().top+11), BPoint(x+5, Bounds().bottom-6), B_SOLID_HIGH );
		StrokeLine( BPoint(x+4, Bounds().top+7), BPoint(x+4, Bounds().bottom-6), B_SOLID_HIGH );
			
		SetHighColor( 192, 192, 192 );
		StrokeLine( BPoint(x+12, Bounds().top+9), BPoint(x+5, Bounds().top+6), B_SOLID_HIGH );
		StrokeLine( BPoint(x+13, Bounds().top+10), BPoint(x+13, Bounds().top+10), B_SOLID_HIGH );
		StrokeLine( BPoint(x+4, Bounds().top+6), BPoint(x+4, Bounds().top+6), B_SOLID_HIGH );
	}
}

void
FakeScrollBar::Draw( BRect updateRect )
{
	SetViewColor( 216, 216, 216 );
	
	// Draw border
	if ( fIsSelected )
		SetHighColor( 0, 0, 0 );
	else if ( fIsPressed )
		SetHighColor( 128, 128, 128 );
	else
		SetHighColor( 216, 216, 216 );
		
	SetPenSize( 2 );
	StrokeRect( BRect(Bounds().left, Bounds().top, Bounds().right-1, Bounds().bottom-1) );
	
	SetPenSize( 1 );
	SetHighColor( 216, 216, 216 );
	
	FillRect(	BRect(Bounds().left+2, Bounds().top+2, Bounds().right-2, Bounds().bottom-2) );	
		
	// Draw first arrows
	DrawArrowBox( 3, true );
	DrawArrowBox( 113, false );
	
	// If double arrows, draw another set.
	if ( fDoubleArrows )
	{
		DrawArrowBox( 19, false );
		DrawArrowBox( 97, true );
	}
	
	// Draw back
	if ( fDoubleArrows )
	{
		SetHighColor( 200, 200, 200 );
		FillRect( BRect(36, 3, 96, 15) );
		
		SetHighColor( 152, 152, 152 );		
		StrokeLine( BPoint(36, 3), BPoint(96, 3) );
		StrokeLine( BPoint(36, 17), BPoint(96, 17) );
		
		SetHighColor( 216, 216, 216 );
		StrokeLine( BPoint(36, 16), BPoint(96, 16) );
		
		SetHighColor( 184, 184, 184 );
		FillRect( BRect(36, 4, 37, 15) );
		FillRect( BRect(36, 4, 96, 5) );
	}
	else
	{
		SetHighColor( 200, 200, 200 );
		FillRect( BRect(20, 3, 112, 15) );
		
		SetHighColor( 152, 152, 152 );		
		StrokeLine( BPoint(20, 3), BPoint(112, 3) );
		StrokeLine( BPoint(20, 17), BPoint(112, 17) );
		
		SetHighColor( 216, 216, 216 );
		StrokeLine( BPoint(20, 16), BPoint(112, 16) );
		
		SetHighColor( 184, 184, 184 );
		FillRect( BRect(20, 4, 21, 15) );
		FillRect( BRect(20, 4, 112, 5) );
	}
	
	// Draw knob
	if ( fFixedThumb )
	{
		SetHighColor( 216, 216, 216 );
		FillRect( 	BRect((Bounds().right/2)-(fMinKnobSize/2), 3, 
					(Bounds().right/2)+(fMinKnobSize/2), 17) );	
		SetHighColor( 152, 152, 152 );
		StrokeRect( BRect((Bounds().right/2)-(fMinKnobSize/2), 3, 
					(Bounds().right/2)+(fMinKnobSize/2), 17) );
		
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint((Bounds().right/2)-(fMinKnobSize/2)+1, 4), 
					BPoint((Bounds().right/2)+(fMinKnobSize/2)-1, 4) );
		StrokeLine( BPoint((Bounds().right/2)-(fMinKnobSize/2)+1, 4), 
					BPoint((Bounds().right/2)-(fMinKnobSize/2)+1, 16) );
	
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint((Bounds().right/2)-(fMinKnobSize/2)+1, 16), 
					BPoint((Bounds().right/2)+(fMinKnobSize/2)-1, 16) );
		StrokeLine( BPoint((Bounds().right/2)+(fMinKnobSize/2)-1, 16), 
					BPoint((Bounds().right/2)+(fMinKnobSize/2)-1, 4) );

		SetHighColor( 96, 96, 96 );
		StrokeLine( BPoint((Bounds().right/2)+(fMinKnobSize/2), 4), 
					BPoint((Bounds().right/2)+(fMinKnobSize/2), 16) );
	}	
	else
	{
		if ( fDoubleArrows )
		{
			SetHighColor( 216, 216, 216 );
			FillRect( BRect(44, 3, 88, 17) );
			SetHighColor( 152, 152, 152 );
			StrokeRect( BRect(44, 3, 88, 17) );
		
			SetHighColor( 255, 255, 255 );
			StrokeLine( BPoint(45, 4), BPoint(87, 4) );
			StrokeLine( BPoint(45, 4), BPoint(45, 16) );
		
			SetHighColor( 184, 184, 184 );
			StrokeLine( BPoint(45, 16), BPoint(87, 16) );
			StrokeLine( BPoint(87, 16), BPoint(87, 4) );

			SetHighColor( 96, 96, 96 );
			StrokeLine( BPoint(88, 4), BPoint(88, 16) );
		}
		else
		{
			SetHighColor( 216, 216, 216 );
			FillRect( BRect(32, 3, 100, 17) );
			SetHighColor( 152, 152, 152 );
			StrokeRect( BRect(32, 3, 100, 17) );
		
			SetHighColor( 255, 255, 255 );
			StrokeLine( BPoint(33, 4), BPoint(99, 4) );
			StrokeLine( BPoint(33, 4), BPoint(33, 16) );
		
			SetHighColor( 184, 184, 184 );
			StrokeLine( BPoint(33, 16), BPoint(99, 16) );
			StrokeLine( BPoint(99, 16), BPoint(99, 4) );
			
			SetHighColor( 96, 96, 96 );
			StrokeLine( BPoint(100, 4), BPoint(100, 16) );
		}
	}

	// Draw knob grabber
	if ( fKnobStyle == THUMB_DOTS )
	{
		if( fFixedThumb )
			DrawDotGrabber( Bounds().right/2, fMinKnobSize );
		else
			DrawDotGrabber( Bounds().right/2, 100 );
	}
	else if ( fKnobStyle == THUMB_LINES )
	{
		if ( fFixedThumb )
			DrawLineGrabber( Bounds().right/2, fMinKnobSize );
		else
			DrawLineGrabber( Bounds().right/2, 100 );
	}
}

void
FakeScrollBar::DrawLineGrabber( uint32 x, uint32 size )
{
	if ( size < 12 )
	{
		// Draw nothing!
	}
	else if ( size < 22 )
	{
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint(x-1, 7), BPoint(x+1, 7) );
		StrokeLine( BPoint(x-1, 7), BPoint(x-1, 13) );
		
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint(x+1, 7), BPoint(x+1, 13) );
		StrokeLine( BPoint(x+1, 13), BPoint(x-1, 13) );	
	}
	else
	{
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint(x-1, 7), BPoint(x+1, 7) );
		StrokeLine( BPoint(x-1, 7), BPoint(x-1, 13) );
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint(x+1, 7), BPoint(x+1, 13) );
		StrokeLine( BPoint(x+1, 13), BPoint(x-1, 13) );	
		
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint(x-5, 7), BPoint(x-3, 7) );
		StrokeLine( BPoint(x-5, 7), BPoint(x-5, 13) );
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint(x-3, 7), BPoint(x-3, 13) );
		StrokeLine( BPoint(x-3, 13), BPoint(x-5, 13) );
		
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint(x+3, 7), BPoint(x+5, 7) );
		StrokeLine( BPoint(x+3, 7), BPoint(x+3, 13) );
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint(x+5, 7), BPoint(x+5, 13) );
		StrokeLine( BPoint(x+5, 13), BPoint(x+3, 13) );
	}
}

void
FakeScrollBar::DrawDotGrabber( uint32 x, uint32 size )
{
	if ( size < 14 )
	{
		// Draw Nothing!
	}
	else if ( size < 27 )
	{
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint(x-2, 8), BPoint(x+2, 8) );
		StrokeLine( BPoint(x-2, 8), BPoint(x-2, 12) );
		
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint(x+2, 8), BPoint(x+2, 12) );
		StrokeLine( BPoint(x+2, 12), BPoint(x-2, 12) );	
	}
	else
	{
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint(x-2, 8), BPoint(x+2, 8) );
		StrokeLine( BPoint(x-2, 8), BPoint(x-2, 12) );
		
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint(x+2, 8), BPoint(x+2, 12) );
		StrokeLine( BPoint(x+2, 12), BPoint(x-2, 12) );	
		
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint(x-9, 8), BPoint(x-5, 8) );
		StrokeLine( BPoint(x-9, 8), BPoint(x-9, 12) );
		
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint(x-5, 8), BPoint(x-5, 12) );
		StrokeLine( BPoint(x-5, 12), BPoint(x-9, 12) );
		
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint(x+5, 8), BPoint(x+9, 8) );
		StrokeLine( BPoint(x+5, 8), BPoint(x+5, 12) );
		
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint(x+9, 8), BPoint(x+9, 12) );
		StrokeLine( BPoint(x+9, 12), BPoint(x+5, 12) );
	}
}

void
FakeScrollBar::MouseDown( BPoint point )
{
	SetTracking( true );
}

void
FakeScrollBar::MouseUp( BPoint point )
{
	if ( IsPressed() )
		Invoke();
	
	SetTracking( false );
}

void
FakeScrollBar::MouseMoved( BPoint point, uint32 transit, const BMessage *message )
{
	if ( transit == B_ENTERED_VIEW && IsTracking() )
		SetPressed( true );
	
	if ( transit == B_EXITED_VIEW && IsTracking() )
		SetPressed( false );
}

void
FakeScrollBar::SetDoubleArrows( bool doublearrows )
{
	fDoubleArrows = doublearrows;
	Invalidate();
}

void
FakeScrollBar::SetFixedThumb( bool fixed )
{
	fFixedThumb = fixed;
	Invalidate();
}

void
FakeScrollBar::SetKnobStyle( uint32 style )
{
	fKnobStyle=style;
	Invalidate();
}

void
FakeScrollBar::SetKnobSize( int32 size )	
{
	fMinKnobSize=size;
	Invalidate();
}

void
FakeScrollBar::SetFromScrollBarInfo(const scroll_bar_info &info)
{
	fDoubleArrows=info.double_arrows;
	fFixedThumb=!info.proportional;
	fMinKnobSize=info.min_knob_size;
	fKnobStyle=info.knob;
	Invalidate();
}

void
FakeScrollBar::SetSelected( bool selected )
{
	fIsSelected=selected;
	Invalidate();
}

void
FakeScrollBar::SetPressed( bool pressed )
{
	fIsPressed=pressed;
	Invalidate();
}

void
FakeScrollBar::SetTracking( bool tracking )
{
	fIsTracking=tracking;
	
	if ( IsTracking() )
	{
		SetEventMask( B_POINTER_EVENTS );
		SetPressed( true );
	}
	else
	{
		SetEventMask( 0 );
		SetPressed( false );
	}
}

bool
FakeScrollBar::IsSelected()
{
	return fIsSelected;
}

bool
FakeScrollBar::IsPressed()
{
	return fIsPressed;
}

bool
FakeScrollBar::IsTracking()
{
	return fIsTracking;
}
