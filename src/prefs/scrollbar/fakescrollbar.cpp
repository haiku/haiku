
#include "scrollbar.h"
#include "fakescrollbar.h"
#include "messages.h"

FakeScrollBar::FakeScrollBar( BPoint position, bool selected, 
	uint32 style, uint32 type, uint32 arrow, uint32 size, BMessage *message ):
	BView( BRect( position.x, position.y, position.x+132, 
	position.y+20 ), "FakeScrollBar", B_FOLLOW_LEFT |
	B_FOLLOW_TOP, B_WILL_DRAW )
{
	Selected=selected;
	Pressed=false;
	ArrowStyle=arrow;
	KnobType=type;
	KnobStyle=style;
	KnobSize=size;
	
	Message=message;
}

void FakeScrollBar::DrawArrowBox( uint32 x, bool facingleft )
{
	// draw box
	SetHighColor( 216, 216, 216 );
	FillRect( BRect( x, Bounds().top+3, 
		x+16, Bounds().bottom-3 ), B_SOLID_HIGH );

	SetHighColor( 152, 152, 152 );
	StrokeRect( BRect( x, Bounds().top+3, x+16, 
		Bounds().bottom-3 ) , B_SOLID_HIGH );		
		
	SetHighColor( 255, 255, 255 );
	StrokeLine( BPoint( x+1, Bounds().top+4 ), 
		BPoint( x+15, Bounds().top+4 ), B_SOLID_HIGH );
	StrokeLine( BPoint( x+1, Bounds().top+4 ), 
		BPoint( x+1, Bounds().bottom-4 ), B_SOLID_HIGH );

	SetHighColor( 192, 192, 192 );
	StrokeLine( BPoint( x+15, Bounds().bottom-4 ), 
		BPoint( x+15, Bounds().top+4 ), B_SOLID_HIGH );
	StrokeLine( BPoint( x+15, Bounds().bottom-4 ),  
		BPoint( x+1, Bounds().bottom-4 ), B_SOLID_HIGH );

	if ( facingleft == true )
	{		
		// draw arrow 	
		SetHighColor( 112, 112, 112 );
		StrokeLine( BPoint( x+4, Bounds().top+10 ), 
			BPoint( x+11, Bounds().top+7 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+4, Bounds().top+10 ), 
			BPoint( x+11, Bounds().bottom-7 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+11, Bounds().top+7 ), 
			BPoint( x+11, Bounds().bottom-7 ), B_SOLID_HIGH );
	
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint( x+4, Bounds().top+11 ), 
			BPoint( x+11, Bounds().bottom-6 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+12, Bounds().top+7 ), 
			BPoint( x+12, Bounds().bottom-6 ), B_SOLID_HIGH );
			
		SetHighColor( 192, 192, 192 );
		StrokeLine( BPoint( x+4, Bounds().top+9 ), 
			BPoint( x+11, Bounds().top+6 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+3, Bounds().top+10 ), 
			BPoint( x+3, Bounds().top+10 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+12, Bounds().top+6 ), 
			BPoint( x+12, Bounds().top+6 ), B_SOLID_HIGH );
	}
	else
		{		
		// draw arrow 	
		SetHighColor( 112, 112, 112 );
		StrokeLine( BPoint( x+12, Bounds().top+10 ), 
			BPoint( x+5, Bounds().top+7 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+12, Bounds().top+10 ), 
			BPoint( x+5, Bounds().bottom-7 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+5, Bounds().top+7 ), 
			BPoint( x+5, Bounds().bottom-7 ), B_SOLID_HIGH );
	
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint( x+12, Bounds().top+11 ), 
			BPoint( x+5, Bounds().bottom-6 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+4, Bounds().top+7 ), 
			BPoint( x+4, Bounds().bottom-6 ), B_SOLID_HIGH );
			
		SetHighColor( 192, 192, 192 );
		StrokeLine( BPoint( x+12, Bounds().top+9 ), 
			BPoint( x+5, Bounds().top+6 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+13, Bounds().top+10 ), 
			BPoint( x+13, Bounds().top+10 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+4, Bounds().top+6 ), 
			BPoint( x+4, Bounds().top+6 ), B_SOLID_HIGH );
	}
}

void FakeScrollBar::Draw( BRect updateRect )
{
	SetViewColor( 216, 216, 216 );
	// Draw border.	
	if ( Selected == true )
		SetHighColor( 0, 0, 0 );
	else if ( Pressed == true )
		SetHighColor( 128, 128, 128 );
	else
		SetHighColor( 216, 216, 216 );
		
	SetPenSize( 2 );
	StrokeRect( BRect( Bounds().left, Bounds().top, 
		Bounds().right-1, Bounds().bottom-1 ), B_SOLID_HIGH );
	
	SetPenSize( 1 );
	SetHighColor( 216, 216, 216 );
	FillRect( BRect( Bounds().left+2, Bounds().top+2, 
		Bounds().right-2, Bounds().bottom-2 ), B_SOLID_HIGH );	
		
	// Draw first arrows
	if ( ArrowStyle == SCROLLBAR_ARROW_STYLE_SINGLE )
	{
		DrawArrowBox( 3, true );
		DrawArrowBox( 113, false );
	}
	
	// If double arrows, draw another set.
	if ( ArrowStyle == SCROLLBAR_ARROW_STYLE_DOUBLE )
	{
		DrawArrowBox( 3, true );
		DrawArrowBox( 113, false );
		DrawArrowBox( 19, false );
		DrawArrowBox( 97, true );
	}
	
	// Draw back
	if ( ArrowStyle == SCROLLBAR_ARROW_STYLE_DOUBLE )
	{
		SetHighColor( 200, 200, 200 );
		FillRect( BRect( 36, 3, 96, 15 ), B_SOLID_HIGH );
		
		SetHighColor( 152, 152, 152 );		
		StrokeLine( BPoint( 36, 3 ), BPoint( 96, 3 ), B_SOLID_HIGH );
		StrokeLine( BPoint( 36, 17 ), BPoint( 96, 17 ), B_SOLID_HIGH );
		SetHighColor( 216, 216, 216 );
		StrokeLine( BPoint( 36, 16 ), BPoint( 96, 16 ), B_SOLID_HIGH );
		SetHighColor( 184, 184, 184 );
		FillRect( BRect( 36, 4, 37, 15 ), B_SOLID_HIGH );
		FillRect( BRect( 36, 4, 96, 5 ), B_SOLID_HIGH );
	}
	else if ( ArrowStyle == SCROLLBAR_ARROW_STYLE_SINGLE )
	{
		SetHighColor( 200, 200, 200 );
		FillRect( BRect( 20, 3, 112, 15 ), B_SOLID_HIGH );
		
		SetHighColor( 152, 152, 152 );		
		StrokeLine( BPoint( 20, 3 ), BPoint( 112, 3 ), B_SOLID_HIGH );
		StrokeLine( BPoint( 20, 17 ), BPoint( 112, 17 ), B_SOLID_HIGH );
		SetHighColor( 216, 216, 216 );
		StrokeLine( BPoint( 20, 16 ), BPoint( 112, 16 ), B_SOLID_HIGH );
		SetHighColor( 184, 184, 184 );
		FillRect( BRect( 20, 4, 21, 15 ), B_SOLID_HIGH );
		FillRect( BRect( 20, 4, 112, 5 ), B_SOLID_HIGH );
	}
	else
	{
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
	}
	
	// Draw knob
	if ( KnobType == SCROLLBAR_KNOB_TYPE_PROPORTIONAL )
	{
		if ( ArrowStyle == SCROLLBAR_ARROW_STYLE_DOUBLE )
		{
			SetHighColor( 216, 216, 216 );
			FillRect( BRect( 44, 3, 88, 17 ), B_SOLID_HIGH );
			SetHighColor( 152, 152, 152 );
			StrokeRect( BRect( 44, 3, 88, 17 ), B_SOLID_HIGH );
		
			SetHighColor( 255, 255, 255 );
			StrokeLine( BPoint( 45, 4 ), BPoint( 87, 4 ), B_SOLID_HIGH );
			StrokeLine( BPoint( 45, 4 ), BPoint( 45, 16 ), B_SOLID_HIGH );
		
			SetHighColor( 184, 184, 184 );
			StrokeLine( BPoint( 45, 16 ), BPoint( 87, 16 ), B_SOLID_HIGH );
			StrokeLine( BPoint( 87, 16 ), BPoint( 87, 4 ), B_SOLID_HIGH );

			SetHighColor( 96, 96, 96 );
			StrokeLine( BPoint( 88, 4 ), BPoint( 88, 16 ), B_SOLID_HIGH );
		}
		else if ( ArrowStyle == SCROLLBAR_ARROW_STYLE_SINGLE )
		{
			SetHighColor( 216, 216, 216 );
			FillRect( BRect( 32, 3, 100, 17 ), B_SOLID_HIGH );
			SetHighColor( 152, 152, 152 );
			StrokeRect( BRect( 32, 3, 100, 17 ), B_SOLID_HIGH );
		
			SetHighColor( 255, 255, 255 );
			StrokeLine( BPoint( 33, 4 ), BPoint( 99, 4 ), B_SOLID_HIGH );
			StrokeLine( BPoint( 33, 4 ), BPoint( 33, 16 ), B_SOLID_HIGH );
		
			SetHighColor( 184, 184, 184 );
			StrokeLine( BPoint( 33, 16 ), BPoint( 99, 16 ), B_SOLID_HIGH );
			StrokeLine( BPoint( 99, 16 ), BPoint( 99, 4 ), B_SOLID_HIGH );
			
			SetHighColor( 96, 96, 96 );
			StrokeLine( BPoint( 100, 4 ), BPoint( 100, 16 ), B_SOLID_HIGH );
		}
		else
		{
			SetHighColor( 216, 216, 216 );
			FillRect( BRect( 13, 3, 119, 17 ), B_SOLID_HIGH );
			SetHighColor( 152, 152, 152 );
			StrokeRect( BRect( 13, 3, 119, 17 ), B_SOLID_HIGH );
			
			SetHighColor( 255, 255, 255 );
			StrokeLine( BPoint( 14, 4 ), BPoint( 118, 4 ), B_SOLID_HIGH );
			StrokeLine( BPoint( 14, 4 ), BPoint( 14, 16 ), B_SOLID_HIGH );
			
			SetHighColor( 184, 184, 184 );
			StrokeLine( BPoint( 14, 16 ), BPoint( 118, 16 ), B_SOLID_HIGH );
			StrokeLine( BPoint( 118, 16 ), BPoint( 118, 4 ), B_SOLID_HIGH );
			
			SetHighColor( 96, 96, 96 );
			StrokeLine( BPoint( 119, 4 ), BPoint( 119, 16 ), B_SOLID_HIGH );
		}
	}
	else
	{
		if ( ArrowStyle == SCROLLBAR_ARROW_STYLE_NONE )
		{
			SetHighColor( 216, 216, 216 );
			FillRect( BRect( 13, 3, 119, 17 ), B_SOLID_HIGH );
			SetHighColor( 152, 152, 152 );
			StrokeRect( BRect( 13, 3, 119, 17 ), B_SOLID_HIGH );
			
			SetHighColor( 255, 255, 255 );
			StrokeLine( BPoint( 14, 4 ), BPoint( 118, 4 ), B_SOLID_HIGH );
			StrokeLine( BPoint( 14, 4 ), BPoint( 14, 16 ), B_SOLID_HIGH );
			
			SetHighColor( 184, 184, 184 );
			StrokeLine( BPoint( 14, 16 ), BPoint( 118, 16 ), B_SOLID_HIGH );
			StrokeLine( BPoint( 118, 16 ), BPoint( 118, 4 ), B_SOLID_HIGH );
			
			SetHighColor( 96, 96, 96 );
			StrokeLine( BPoint( 119, 4 ), BPoint( 119, 16 ), B_SOLID_HIGH );
		}
		else
		{
			SetHighColor( 216, 216, 216 );
			FillRect( BRect( (Bounds().right/2)-(KnobSize/2), 3, 
				(Bounds().right/2)+(KnobSize/2), 17 ), B_SOLID_HIGH );	
			SetHighColor( 152, 152, 152 );
			StrokeRect( BRect( (Bounds().right/2)-(KnobSize/2), 3, 
				(Bounds().right/2)+(KnobSize/2), 17 ), B_SOLID_HIGH );
			
			SetHighColor( 255, 255, 255 );
			StrokeLine( BPoint( (Bounds().right/2)-(KnobSize/2)+1, 4 ), 
				BPoint( (Bounds().right/2)+(KnobSize/2)-1, 4 ), B_SOLID_HIGH );
			StrokeLine( BPoint( (Bounds().right/2)-(KnobSize/2)+1, 4 ), 
				BPoint( (Bounds().right/2)-(KnobSize/2)+1, 16 ), B_SOLID_HIGH );
		
			SetHighColor( 184, 184, 184 );
			StrokeLine( BPoint( (Bounds().right/2)-(KnobSize/2)+1, 16 ), 
				BPoint( (Bounds().right/2)+(KnobSize/2)-1, 16 ), B_SOLID_HIGH );
			StrokeLine( BPoint( (Bounds().right/2)+(KnobSize/2)-1, 16 ), 
				BPoint( (Bounds().right/2)+(KnobSize/2)-1, 4 ), B_SOLID_HIGH );

			SetHighColor( 96, 96, 96 );
			StrokeLine( BPoint( (Bounds().right/2)+(KnobSize/2), 4 ), 
				BPoint( (Bounds().right/2)+(KnobSize/2), 16 ), B_SOLID_HIGH );
		}
	}	

	// Draw knob grabber
	if ( KnobStyle == SCROLLBAR_KNOB_STYLE_DOT )
	{
		if ( KnobType == SCROLLBAR_KNOB_TYPE_PROPORTIONAL || 
			 ArrowStyle == SCROLLBAR_ARROW_STYLE_NONE )
			DrawDotGrabber( Bounds().right/2, 100 );
		else
			DrawDotGrabber( Bounds().right/2, KnobSize );
	}
	else if ( KnobStyle == SCROLLBAR_KNOB_STYLE_LINE )
	{
		if ( KnobType == SCROLLBAR_KNOB_TYPE_PROPORTIONAL || 
			 ArrowStyle == SCROLLBAR_ARROW_STYLE_NONE )
			DrawLineGrabber( Bounds().right/2, 100 );
		else
			DrawLineGrabber( Bounds().right/2, KnobSize );
	}
}

void FakeScrollBar::DrawLineGrabber( uint32 x, uint32 size )
{
	if ( size < 12 )
	{
		// Draw nothing!
	}
	else if ( size < 22 )
	{
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint( x-1, 7 ), BPoint( x+1, 7 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x-1, 7 ), BPoint( x-1, 13 ), B_SOLID_HIGH );
		
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint( x+1, 7 ), BPoint( x+1, 13 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+1, 13 ), BPoint( x-1, 13 ), B_SOLID_HIGH );	
	}
	else
	{
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint( x-1, 7 ), BPoint( x+1, 7 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x-1, 7 ), BPoint( x-1, 13 ), B_SOLID_HIGH );
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint( x+1, 7 ), BPoint( x+1, 13 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+1, 13 ), BPoint( x-1, 13 ), B_SOLID_HIGH );	
		
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint( x-5, 7 ), BPoint( x-3, 7 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x-5, 7 ), BPoint( x-5, 13 ), B_SOLID_HIGH );
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint( x-3, 7 ), BPoint( x-3, 13 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x-3, 13 ), BPoint( x-5, 13 ), B_SOLID_HIGH );
		
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint( x+3, 7 ), BPoint( x+5, 7 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+3, 7 ), BPoint( x+3, 13 ), B_SOLID_HIGH );
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint( x+5, 7 ), BPoint( x+5, 13 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+5, 13 ), BPoint( x+3, 13 ), B_SOLID_HIGH );
	}
}

void FakeScrollBar::DrawDotGrabber( uint32 x, uint32 size )
{
	if ( size < 14 )
	{
		// Draw Nothing!
	}
	else if ( size < 27 )
	{
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint( x-2, 8 ), BPoint( x+2, 8 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x-2, 8 ), BPoint( x-2, 12 ), B_SOLID_HIGH );
		
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint( x+2, 8 ), BPoint( x+2, 12 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+2, 12 ), BPoint( x-2, 12 ), B_SOLID_HIGH );	
	}
	else
	{
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint( x-2, 8 ), BPoint( x+2, 8 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x-2, 8 ), BPoint( x-2, 12 ), B_SOLID_HIGH );
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint( x+2, 8 ), BPoint( x+2, 12 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+2, 12 ), BPoint( x-2, 12 ), B_SOLID_HIGH );	
		
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint( x-9, 8 ), BPoint( x-5, 8 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x-9, 8 ), BPoint( x-9, 12 ), B_SOLID_HIGH );
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint( x-5, 8 ), BPoint( x-5, 12 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x-5, 12 ), BPoint( x-9, 12 ), B_SOLID_HIGH );
		
		SetHighColor( 255, 255, 255 );
		StrokeLine( BPoint( x+5, 8 ), BPoint( x+9, 8 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+5, 8 ), BPoint( x+5, 12 ), B_SOLID_HIGH );
		
		SetHighColor( 184, 184, 184 );
		StrokeLine( BPoint( x+9, 8 ), BPoint( x+9, 12 ), B_SOLID_HIGH );
		StrokeLine( BPoint( x+9, 12 ), BPoint( x+5, 12 ), B_SOLID_HIGH );
	}
}

void FakeScrollBar::MouseDown( BPoint point )
{
	SetTracking( true );
}

void FakeScrollBar::MouseUp( BPoint point )
{
	if ( IsPressed() == true )
		be_app->PostMessage( Message );
	SetTracking( false );
}

void FakeScrollBar::MouseMoved( BPoint point, uint32 transit, const BMessage *message )
{
	if ( transit == B_ENTERED_VIEW && IsTracking() == true )
		SetPressed( true );
	if ( transit == B_EXITED_VIEW && IsTracking() == true )
		SetPressed( false );
}

void FakeScrollBar::SetKnobStyle( uint32 style )
{
	KnobStyle=style;
	Invalidate();
}

void FakeScrollBar::SetKnobType( uint32 type )
{
	KnobType=type;
	Invalidate();
}

void FakeScrollBar::SetArrowStyle( uint32 arrow )
{
	ArrowStyle=arrow;
	Invalidate();
}

void FakeScrollBar::SetKnobSize( uint32 size )	
{
	KnobSize=size;
	Invalidate();
}

void FakeScrollBar::SetSelected( bool selected )
{
	Selected=selected;
	Invalidate();
}

void FakeScrollBar::SetPressed( bool pressed )
{
	Pressed=pressed;
	Invalidate();
}

void FakeScrollBar::SetTracking( bool tracking )
{
	Tracking=tracking;
	
	if ( IsTracking() == true )
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

bool FakeScrollBar::IsSelected()
{
	return Selected;
}

bool FakeScrollBar::IsPressed()
{
	return Pressed;
}

bool FakeScrollBar::IsTracking()
{
	return Tracking;
}

