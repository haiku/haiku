#include <stdio.h>

#include "scrollbar.h"
#include "knobsizeadjuster.h"
#include "messages.h"

KnobSizeAdjuster::KnobSizeAdjuster( BPoint position, uint32 size ):
	BView( BRect( position.x, position.y, position.x+133, 
	position.y+26 ), "KnobSizeAdjuster", B_FOLLOW_LEFT |
	B_FOLLOW_TOP, B_WILL_DRAW )
{
	KnobSize=size;
}

void KnobSizeAdjuster::Draw( BRect updateRect )
{
	SetViewColor( 216, 216, 216 );
	// Draw border.	
	SetHighColor( 216, 216, 216 );
	FillRect( Bounds(), B_SOLID_HIGH );	
		
	// Draw back
	SetHighColor( 152, 152, 152 );
	StrokeLine( BPoint( 4, 3 ), BPoint( Bounds().right-5, 3 ), B_SOLID_HIGH );
	StrokeLine( BPoint( 4, 17 ), BPoint( Bounds().right-5, 17 ), B_SOLID_HIGH );
	SetHighColor( 184, 184, 184 );
	FillRect( BRect( 4, 4, Bounds().right-5, 5 ), B_SOLID_HIGH );
	SetHighColor( 200, 200, 200 );
	FillRect( BRect( 4, 6, Bounds().right-5, 15), B_SOLID_HIGH );
	StrokeLine( BPoint( 3, 15 ), BPoint( 3, 13 ), B_SOLID_HIGH );
	StrokeLine( BPoint( Bounds().right-4, 7 ), BPoint( Bounds().right-4, 11 ), B_SOLID_HIGH );
	StrokeLine( BPoint( Bounds().right-3, 9 ), BPoint( Bounds().right-3, 9 ), B_SOLID_HIGH );
	SetHighColor( 216, 216, 216 );
	StrokeLine( BPoint( 4, 7 ), BPoint( 4, 11 ), B_SOLID_HIGH );
	StrokeLine( BPoint( 5, 9 ), BPoint( 5, 9 ), B_SOLID_HIGH );
	StrokeLine( BPoint( Bounds().right-5, 15 ), BPoint( Bounds().right-5, 13 ), B_SOLID_HIGH );
	
	// Draw knob
	SetHighColor( 216, 216, 216 );
	FillRect( BRect( 41, 3, 41+KnobSize, 17 ), B_SOLID_HIGH );
	SetHighColor( 152, 152, 152 );
	StrokeRect( BRect( 41, 3, 41+KnobSize, 17 ), B_SOLID_HIGH );
		
	SetHighColor( 255, 255, 255 );
	StrokeLine( BPoint( 42, 4 ), BPoint( 41+KnobSize-1, 4 ), B_SOLID_HIGH );
	StrokeLine( BPoint( 42, 4 ), BPoint( 42, 16 ), B_SOLID_HIGH );
		
	SetHighColor( 184, 184, 184 );
	StrokeLine( BPoint( 42, 16 ), BPoint( 41+KnobSize-1, 16 ), B_SOLID_HIGH );
	StrokeLine( BPoint( 41+KnobSize-1, 16 ), BPoint( 41+KnobSize-1, 4 ), B_SOLID_HIGH );
			
	SetHighColor( 96, 96, 96 );
	StrokeLine( BPoint( 41+KnobSize, 4 ), BPoint( 41+KnobSize, 16 ), B_SOLID_HIGH );
	
	// Draw slider
	SetHighColor( 0, 255, 0 );
	FillTriangle( BPoint( 41+KnobSize, 18), BPoint( 41+KnobSize+5, 23 ), BPoint( 41+KnobSize-5, 23), B_SOLID_HIGH );
	SetHighColor( 0, 0, 0 );
	StrokeTriangle( BPoint( 41+KnobSize, 18), BPoint( 41+KnobSize+5, 23 ), 
		BPoint( 41+KnobSize-5, 23), B_SOLID_HIGH );
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
		be_app->PostMessage( SCROLLBAR_KNOB_SIZE_CHANGED );
}

void KnobSizeAdjuster::MouseMoved( BPoint point, uint32 transit, 
	const BMessage *message )
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

void KnobSizeAdjuster::SetKnobSize( uint32 size )	
{
	KnobSize=size;
	Settings.KnobSize=KnobSize;
	
	Invalidate();
}

void KnobSizeAdjuster::SetPressed( bool pressed )
{
	Pressed=pressed;
}

void KnobSizeAdjuster::SetTracking( bool tracking )
{
	Tracking=tracking;
	
	if ( IsTracking() == true )
		SetEventMask( B_POINTER_EVENTS );
	else
		SetEventMask( 0 );
}

bool KnobSizeAdjuster::IsPressed()
{
	return Pressed;
}

bool KnobSizeAdjuster::IsTracking()
{
	return Tracking;
}


