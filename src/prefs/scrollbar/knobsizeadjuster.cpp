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
//	File Name:		knobsizeadjuster.cpp
//	Author:			MrSiggler
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Control for changing the minimum size of thumb for scrollbars
//  
//------------------------------------------------------------------------------
#include <stdio.h>

#include "ScrollBarApp.h"
#include "knobsizeadjuster.h"
#include "messages.h"

#define MAXIMUM_KNOB_SIZE	50
#define MINIMUM_KNOB_SIZE	9

KnobSizeAdjuster::KnobSizeAdjuster( BPoint position, uint32 size )
	:BControl(	BRect(position.x, position.y, position.x+133, position.y+26), 
				"KnobSizeAdjuster", NULL, new BMessage(SCROLLBAR_KNOB_SIZE_CHANGED),
				B_FOLLOW_LEFT |	B_FOLLOW_TOP, B_WILL_DRAW )
{
	fKnobSize=size;
}

void KnobSizeAdjuster::Draw( BRect updateRect )
{
	SetViewColor( 216, 216, 216 );
	
	// Draw border
	SetHighColor( 216, 216, 216 );
	FillRect( Bounds() );	
		
	// Draw back
	SetHighColor( 152, 152, 152 );
	StrokeLine( BPoint(4, 3), BPoint(Bounds().right-5, 3) );
	StrokeLine( BPoint(4, 17), BPoint(Bounds().right-5, 17) );
	
	SetHighColor( 184, 184, 184 );
	FillRect( BRect(4, 4, Bounds().right-5, 5) );
	
	SetHighColor( 200, 200, 200 );
	FillRect( BRect(4, 6, Bounds().right-5, 15) );
	StrokeLine( BPoint(3, 15), BPoint(3, 13) );
	StrokeLine( BPoint(Bounds().right-4, 7), BPoint(Bounds().right-4, 11) );
	StrokeLine( BPoint(Bounds().right-3, 9), BPoint(Bounds().right-3, 9) );
	
	SetHighColor( 216, 216, 216 );
	StrokeLine( BPoint(4, 7), BPoint(4, 11) );
	StrokeLine( BPoint(5, 9), BPoint(5, 9) );
	StrokeLine( BPoint(Bounds().right-5, 15), BPoint(Bounds().right-5, 13) );
	
	// Draw knob
	SetHighColor( 216, 216, 216 );
	FillRect( BRect(41, 3, 41+fKnobSize, 17) );
	
	SetHighColor( 152, 152, 152 );
	StrokeRect( BRect(41, 3, 41+fKnobSize, 17) );
		
	SetHighColor( 255, 255, 255 );
	StrokeLine( BPoint(42, 4), BPoint(41+fKnobSize-1, 4) );
	StrokeLine( BPoint(42, 4), BPoint(42, 16) );
		
	SetHighColor( 184, 184, 184 );
	StrokeLine( BPoint(42, 16), BPoint(41+fKnobSize-1, 16) );
	StrokeLine( BPoint(41+fKnobSize-1, 16), BPoint(41+fKnobSize-1, 4) );
			
	SetHighColor( 96, 96, 96 );
	StrokeLine( BPoint(41+fKnobSize, 4), BPoint(41+fKnobSize, 16) );
	
	// Draw slider
	SetHighColor( 0, 255, 0 );
	FillTriangle( BPoint(41+fKnobSize, 18), BPoint(41+fKnobSize+5, 23), BPoint(41+fKnobSize-5, 23) );
	
	SetHighColor( 0, 0, 0 );
	StrokeTriangle( BPoint(41+fKnobSize, 18), BPoint(41+fKnobSize+5, 23), 
		BPoint(41+fKnobSize-5, 23) );
}

void KnobSizeAdjuster::MouseDown( BPoint point )
{
	int32 newsize=0;

	if ( point.x > 41+MINIMUM_KNOB_SIZE && 	point.x < MAXIMUM_KNOB_SIZE+41 )
		newsize = (int)point.x-41;
	
	if ( point.x <= 41+MINIMUM_KNOB_SIZE )
		newsize = MINIMUM_KNOB_SIZE;
	
	if ( point.x >= 41+MAXIMUM_KNOB_SIZE )
		newsize = MAXIMUM_KNOB_SIZE;
	
	SetKnobSize( newsize );	
			
	SetTracking( true );
}

void KnobSizeAdjuster::MouseUp( BPoint point )
{
	SetTracking( false );
	Invoke();
}

void KnobSizeAdjuster::MouseMoved( BPoint point, uint32 transit, const BMessage *message )
{
	int32 newsize=0;

	if ( transit == B_INSIDE_VIEW && IsTracking() == true )
	{
		if ( point.x > 41+MINIMUM_KNOB_SIZE && 	point.x < MAXIMUM_KNOB_SIZE+41 )
			newsize = (int)point.x-41;
		
		if ( point.x <= 41+MINIMUM_KNOB_SIZE )
			newsize = MINIMUM_KNOB_SIZE;
		
		if ( point.x >= 41+MAXIMUM_KNOB_SIZE )
			newsize = MAXIMUM_KNOB_SIZE;
		
		SetKnobSize( newsize );	
	}
}

void KnobSizeAdjuster::SetKnobSize( int32 size )	
{
	fKnobSize=size;
	gSettings.min_knob_size=fKnobSize;
	
	Invalidate();
}

void KnobSizeAdjuster::SetPressed( bool pressed )
{
	fPressed=pressed;
}

void KnobSizeAdjuster::SetTracking( bool tracking )
{
	fTracking=tracking;
	
	if ( IsTracking() == true )
		SetEventMask( B_POINTER_EVENTS );
	else
		SetEventMask( 0 );
}

bool KnobSizeAdjuster::IsPressed()
{
	return fPressed;
}

bool KnobSizeAdjuster::IsTracking()
{
	return fTracking;
}


