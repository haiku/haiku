#include <Box.h>
#include <File.h>
#include <StringView.h>
#include <FindDirectory.h>
#include <Path.h>

#include "scrollbar.h"
#include "fakescrollbar.h"
#include "scrollbar_window.h"
#include "messages.h"

ScrollBarWindow::ScrollBarWindow():BWindow( BRect(0,0,0,0), 
	"ScrollBarWindow", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE |
	B_ASYNCHRONOUS_CONTROLS )
{
	MoveTo( 100, 100 );
	ResizeTo( 348, 275 );
	SetTitle( "OBOS Scroll Bar" );

	// Set defaults
	Settings.ArrowStyle = SCROLLBAR_ARROW_STYLE_DOUBLE;
	Settings.KnobStyle = SCROLLBAR_KNOB_STYLE_DOT;
	Settings.KnobType = SCROLLBAR_KNOB_TYPE_PROPORTIONAL;
	Settings.KnobSize = 16;

	// Load settings
	BPath path;
	find_directory( B_USER_SETTINGS_DIRECTORY, &path );
	path.Append( "OBOS_ScrollBar_Settings", true );
	BFile file( path.Path(), B_READ_ONLY );
	BMessage msg;
	msg.Unflatten( &file );
	LoadSettings( &msg );
	RevertSettings=Settings;
		
	MainBox = new BBox( BRect(0,0,348,275), "MainBox", B_FOLLOW_LEFT | 
		B_FOLLOW_TOP, B_WILL_DRAW, B_PLAIN_BORDER );
	AddChild( MainBox );

	// Arrow style 
	BBox* ArrowStyleBox = new BBox( BRect( 12, 7, 169, 119) );
	ArrowStyleBox->SetLabel( "Arrow Style" );
	MainBox->AddChild( ArrowStyleBox );
	
	BStringView* DoubleArrowLabel = new BStringView( BRect(13,13,48,31), 
		"DoubleArrowLabel", "Double:" );
	ArrowStyleBox->AddChild( DoubleArrowLabel );
	
	DoubleArrowView = new FakeScrollBar( BPoint( 12, 31 ), false, 
		Settings.KnobStyle, Settings.KnobType, SCROLLBAR_ARROW_STYLE_DOUBLE, 
		Settings.KnobSize, (new BMessage(SCROLLBAR_ARROW_STYLE_DOUBLE)) );
	ArrowStyleBox->AddChild( DoubleArrowView );
	
	BStringView* SingleArrowLabel = new BStringView( BRect(13,65,48,80), 
		"SingleArrowLabel", "Single:" );
	ArrowStyleBox->AddChild( SingleArrowLabel );
	
	SingleArrowView = new FakeScrollBar( BPoint( 12, 80 ), false, 
		Settings.KnobStyle, Settings.KnobType, SCROLLBAR_ARROW_STYLE_SINGLE, 
		Settings.KnobSize, (new BMessage(SCROLLBAR_ARROW_STYLE_SINGLE)) );
	ArrowStyleBox->AddChild( SingleArrowView );
	
	// Knob Type
	BBox* KnobTypeBox = new BBox( BRect( 180, 7, 338, 119) );
	KnobTypeBox->SetLabel( "Knob Type" );
	MainBox->AddChild( KnobTypeBox );
	
	BStringView* ProportionalKnobLabel = new BStringView( BRect(13,13,80,31), 
		"ProportionalKnobLabel", "Proportional:" );
	KnobTypeBox->AddChild( ProportionalKnobLabel );
	
	ProportionalKnobView = new FakeScrollBar( BPoint( 12, 31 ), false, 
		Settings.KnobStyle, SCROLLBAR_KNOB_TYPE_PROPORTIONAL, Settings.ArrowStyle, 
		Settings.KnobSize, (new BMessage(SCROLLBAR_KNOB_TYPE_PROPORTIONAL)) );
	KnobTypeBox->AddChild( ProportionalKnobView );
		
	BStringView* FixedKnobLabel = new BStringView( BRect(13,65,48,80), 
		"fixedStringView", "Fixed:" );
	KnobTypeBox->AddChild( FixedKnobLabel );
		
	FixedKnobView = new FakeScrollBar( BPoint( 12, 80 ), false, 
		Settings.KnobStyle, SCROLLBAR_KNOB_TYPE_FIXED, Settings.ArrowStyle, 
		Settings.KnobSize, (new BMessage(SCROLLBAR_KNOB_TYPE_FIXED)) );
	KnobTypeBox->AddChild( FixedKnobView );
	
	// Minimum knob size
	BBox* MinKnobSizeBox = new BBox( BRect( 12, 123, 169, 232) );
	MinKnobSizeBox->SetLabel( "Minimum Knob Size" );
	MainBox->AddChild( MinKnobSizeBox );
	MinimumKnobSize = new KnobSizeAdjuster( BPoint( 12, 50 ), Settings.KnobSize );
	MinKnobSizeBox->AddChild( MinimumKnobSize );

	// Knob Style	
	BBox* KnobStyleBox = new BBox( BRect( 180, 123, 338, 232) );
	KnobStyleBox->SetLabel( "Knob Style" );
	MainBox->AddChild( KnobStyleBox );
	
	FlatKnobView = new FakeScrollBar( BPoint( 12, 19 ), false, 
		SCROLLBAR_KNOB_STYLE_FLAT, Settings.KnobType, 
		SCROLLBAR_ARROW_STYLE_NONE, Settings.KnobSize, (new BMessage(SCROLLBAR_KNOB_STYLE_FLAT)) );
	KnobStyleBox->AddChild( FlatKnobView );
	
	DotKnobView = new FakeScrollBar( BPoint( 12, 49 ), false, 
		SCROLLBAR_KNOB_STYLE_DOT, Settings.KnobType, 
		SCROLLBAR_ARROW_STYLE_NONE, Settings.KnobSize, (new BMessage(SCROLLBAR_KNOB_STYLE_DOT)) );
	KnobStyleBox->AddChild( DotKnobView );
	
    LineKnobView = new FakeScrollBar( BPoint( 12, 79 ), false, 
		SCROLLBAR_KNOB_STYLE_LINE, Settings.KnobType, 
		SCROLLBAR_ARROW_STYLE_NONE, Settings.KnobSize, (new BMessage(SCROLLBAR_KNOB_STYLE_LINE)) );
  	KnobStyleBox->AddChild( LineKnobView );
  	
  	// Set the proper things selected..
  	if ( Settings.ArrowStyle == SCROLLBAR_ARROW_STYLE_DOUBLE )
  		DoubleArrowView->SetSelected( true );
  	else if ( Settings.ArrowStyle == SCROLLBAR_ARROW_STYLE_SINGLE )
  		SingleArrowView->SetSelected( true );
  	
   	if ( Settings.KnobType == SCROLLBAR_KNOB_TYPE_PROPORTIONAL )
  		ProportionalKnobView->SetSelected( true );
  	else if ( Settings.KnobType == SCROLLBAR_KNOB_TYPE_FIXED )
  		FixedKnobView->SetSelected( true );
  		
   	if ( Settings.KnobStyle == SCROLLBAR_KNOB_STYLE_FLAT )
  		FlatKnobView->SetSelected( true );
  	else if ( Settings.KnobStyle == SCROLLBAR_KNOB_STYLE_DOT )
  		DotKnobView->SetSelected( true );
  	else if ( Settings.KnobStyle == SCROLLBAR_KNOB_STYLE_LINE )
  		LineKnobView->SetSelected( true );  	
  	
  	// Buttons
	BButton* DefaultsButton = new BButton( BRect( 10, 242, 85, 265), "DefaultsButton", 
		"Defaults", (new BMessage(SCROLLBAR_DEFAULTS)), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
	MainBox->AddChild( DefaultsButton );
	
	RevertButton = new BButton( BRect( 95, 242, 170, 265), "RevertButton", "Revert", 
		(new BMessage(SCROLLBAR_REVERT)), B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
	RevertButton->SetEnabled( FALSE );
	MainBox->AddChild( RevertButton );
}

void ScrollBarWindow::MessageReceived( BMessage *message )
{
	switch ( message->what )
	{
		case TEST_MSG:
	
			break;
		case SCROLLBAR_ARROW_STYLE_DOUBLE:
			if ( DoubleArrowView->IsSelected() == false )
			{
				RevertButton->SetEnabled( TRUE );
				ProportionalKnobView->SetArrowStyle( SCROLLBAR_ARROW_STYLE_DOUBLE );
				FixedKnobView->SetArrowStyle( SCROLLBAR_ARROW_STYLE_DOUBLE );
				DoubleArrowView->SetSelected( true );
				SingleArrowView->SetSelected( false );
				Settings.ArrowStyle=SCROLLBAR_ARROW_STYLE_DOUBLE;
			}
			break;	
		case SCROLLBAR_ARROW_STYLE_SINGLE:
			if ( SingleArrowView->IsSelected() == false )
			{
				RevertButton->SetEnabled( TRUE );
				ProportionalKnobView->SetArrowStyle( SCROLLBAR_ARROW_STYLE_SINGLE );
				FixedKnobView->SetArrowStyle( SCROLLBAR_ARROW_STYLE_SINGLE );
				DoubleArrowView->SetSelected( false );
				SingleArrowView->SetSelected( true );
				Settings.ArrowStyle=SCROLLBAR_ARROW_STYLE_SINGLE;
			}
			break;		
		case SCROLLBAR_KNOB_TYPE_PROPORTIONAL:
			if ( ProportionalKnobView->IsSelected() == false )
			{
				RevertButton->SetEnabled( TRUE );
				DoubleArrowView->SetKnobType( SCROLLBAR_KNOB_TYPE_PROPORTIONAL );
				SingleArrowView->SetKnobType( SCROLLBAR_KNOB_TYPE_PROPORTIONAL );
				ProportionalKnobView->SetSelected( true );
				FixedKnobView->SetSelected( false );
				Settings.KnobType=SCROLLBAR_KNOB_TYPE_PROPORTIONAL;
			}
			break;	
		case SCROLLBAR_KNOB_TYPE_FIXED:
			if ( FixedKnobView->IsSelected() == false )
			{
				RevertButton->SetEnabled( TRUE );
				DoubleArrowView->SetKnobType( SCROLLBAR_KNOB_TYPE_FIXED );
				SingleArrowView->SetKnobType( SCROLLBAR_KNOB_TYPE_FIXED );
				ProportionalKnobView->SetSelected( false );
				FixedKnobView->SetSelected( true );
				Settings.KnobType=SCROLLBAR_KNOB_TYPE_FIXED;
			}
			break;	
		case SCROLLBAR_KNOB_STYLE_FLAT:
			if ( FlatKnobView->IsSelected() == false )
			{
				RevertButton->SetEnabled( TRUE );
				DoubleArrowView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_FLAT );
				SingleArrowView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_FLAT );
				ProportionalKnobView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_FLAT );
				FixedKnobView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_FLAT );
				FlatKnobView->SetSelected( true );
				DotKnobView->SetSelected( false );
				LineKnobView->SetSelected( false );
				Settings.KnobStyle=SCROLLBAR_KNOB_STYLE_FLAT;
			}
			break;		
		case SCROLLBAR_KNOB_STYLE_DOT:
			if ( DotKnobView->IsSelected() == false )
			{
				RevertButton->SetEnabled( TRUE );
				DoubleArrowView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_DOT );
				SingleArrowView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_DOT );
				ProportionalKnobView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_DOT );
				FixedKnobView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_DOT );
				FlatKnobView->SetSelected( false );
				DotKnobView->SetSelected( true );
				LineKnobView->SetSelected( false );
				Settings.KnobStyle=SCROLLBAR_KNOB_STYLE_DOT;
			}
			break;	
		case SCROLLBAR_KNOB_STYLE_LINE:
			if ( LineKnobView->IsSelected() == false )
			{
				RevertButton->SetEnabled( TRUE );
				DoubleArrowView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_LINE );
				SingleArrowView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_LINE );
				ProportionalKnobView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_LINE );
				FixedKnobView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_LINE );
				FlatKnobView->SetSelected( false );
				DotKnobView->SetSelected( false );
				LineKnobView->SetSelected( true );
				Settings.KnobStyle=SCROLLBAR_KNOB_STYLE_LINE;
			}
			break;			
		case SCROLLBAR_KNOB_SIZE_CHANGED:
			RevertButton->SetEnabled( TRUE );
			DoubleArrowView->SetKnobSize( Settings.KnobSize );
			SingleArrowView->SetKnobSize( Settings.KnobSize );
			ProportionalKnobView->SetKnobSize( Settings.KnobSize );
			FixedKnobView->SetKnobSize( Settings.KnobSize );
			break;		
		case SCROLLBAR_DEFAULTS:
			// Load settings. Using defaults for now..
			Settings.ArrowStyle = SCROLLBAR_ARROW_STYLE_DOUBLE;
			Settings.KnobStyle = SCROLLBAR_KNOB_STYLE_DOT;
			Settings.KnobType = SCROLLBAR_KNOB_TYPE_PROPORTIONAL;
			Settings.KnobSize = 16;
			RevertButton->SetEnabled( true );
			ProportionalKnobView->SetArrowStyle( SCROLLBAR_ARROW_STYLE_DOUBLE );
			FixedKnobView->SetArrowStyle( SCROLLBAR_ARROW_STYLE_DOUBLE );
			DoubleArrowView->SetSelected( true );
			SingleArrowView->SetSelected( false );
			DoubleArrowView->SetKnobType( SCROLLBAR_KNOB_TYPE_PROPORTIONAL );
			SingleArrowView->SetKnobType( SCROLLBAR_KNOB_TYPE_PROPORTIONAL );
			ProportionalKnobView->SetSelected( true );
			FixedKnobView->SetSelected( false );
			DoubleArrowView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_DOT );
			SingleArrowView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_DOT );
			ProportionalKnobView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_DOT );
			FixedKnobView->SetKnobStyle( SCROLLBAR_KNOB_STYLE_DOT );
			FlatKnobView->SetSelected( false );
			DotKnobView->SetSelected( true );
			LineKnobView->SetSelected( false );
			MinimumKnobSize->SetKnobSize( 16 );
			DoubleArrowView->SetKnobSize( Settings.KnobSize );
			SingleArrowView->SetKnobSize( Settings.KnobSize );
			ProportionalKnobView->SetKnobSize( Settings.KnobSize );
			FixedKnobView->SetKnobSize( Settings.KnobSize );
			break;
		case SCROLLBAR_REVERT:
			RevertButton->SetEnabled( FALSE );
			Settings=RevertSettings;
			ProportionalKnobView->SetArrowStyle( Settings.ArrowStyle );
			FixedKnobView->SetArrowStyle( Settings.ArrowStyle );
			//DoubleArrowView->SetSelected( true );
			//SingleArrowView->SetSelected( false );
			DoubleArrowView->SetKnobType( Settings.KnobType );
			SingleArrowView->SetKnobType( Settings.KnobType );
			//ProportionalKnobView->SetSelected( true );
			//FixedKnobView->SetSelected( false );
			DoubleArrowView->SetKnobStyle( Settings.KnobStyle );
			SingleArrowView->SetKnobStyle( Settings.KnobStyle );
			ProportionalKnobView->SetKnobStyle( Settings.KnobStyle );
			FixedKnobView->SetKnobStyle( Settings.KnobStyle );
			//FlatKnobView->SetSelected( false );
			//DotKnobView->SetSelected( true );
			//LineKnobView->SetSelected( false );
			MinimumKnobSize->SetKnobSize( Settings.KnobSize );
			DoubleArrowView->SetKnobSize( Settings.KnobSize );
			SingleArrowView->SetKnobSize( Settings.KnobSize );
			ProportionalKnobView->SetKnobSize( Settings.KnobSize );
			FixedKnobView->SetKnobSize( Settings.KnobSize );	
			
			if ( Settings.ArrowStyle == SCROLLBAR_ARROW_STYLE_DOUBLE )
			{
				DoubleArrowView->SetSelected( true );
				SingleArrowView->SetSelected( false );
			}
			else
			{
				DoubleArrowView->SetSelected( false );
				SingleArrowView->SetSelected( true );
			}
			
			if ( Settings.KnobType == SCROLLBAR_KNOB_TYPE_PROPORTIONAL )
			{
				ProportionalKnobView->SetSelected( true );
				FixedKnobView->SetSelected( false );
			}
			else
			{
				ProportionalKnobView->SetSelected( false );
				FixedKnobView->SetSelected( true );
			}
				
			if ( Settings.KnobStyle == SCROLLBAR_KNOB_STYLE_FLAT )
			{
				FlatKnobView->SetSelected( true );
				DotKnobView->SetSelected( false );
				LineKnobView->SetSelected( false );
			}
			else if ( Settings.KnobStyle == SCROLLBAR_KNOB_STYLE_DOT )
			{
				FlatKnobView->SetSelected( false );
				DotKnobView->SetSelected( true );
				LineKnobView->SetSelected( false );
			}
			else if ( Settings.KnobStyle == SCROLLBAR_KNOB_STYLE_LINE )
			{
				FlatKnobView->SetSelected( false );
				DotKnobView->SetSelected( false );
				LineKnobView->SetSelected( true );
			}
			break;
	}
}

void ScrollBarWindow::LoadSettings( BMessage *message )
{
	BPoint point;
	int32 ArrowStyle, KnobType, KnobSize, KnobStyle;
	ArrowStyle=KnobType=KnobSize=KnobStyle=0;
	
	if ( message->FindPoint( "position", &point ) == B_OK )
		MoveTo( point );
	
	if ( message->FindInt32( "arrowstyle", &ArrowStyle ) == B_OK )
	{
		if ( ArrowStyle == 0 )
			Settings.ArrowStyle=SCROLLBAR_ARROW_STYLE_DOUBLE;
		else if ( ArrowStyle == 1 )
			Settings.ArrowStyle=SCROLLBAR_ARROW_STYLE_SINGLE;
	}
	if ( message->FindInt32( "knobtype", &KnobType ) == B_OK )
	{
		if ( KnobType == 0 )
			Settings.KnobType=SCROLLBAR_KNOB_TYPE_PROPORTIONAL;
		else if ( KnobType == 1 )
			 Settings.KnobType=SCROLLBAR_KNOB_TYPE_FIXED;
	}
	if ( message->FindInt32( "knobstyle", &KnobStyle ) == B_OK )
	{
		if ( KnobStyle == 0 )
			Settings.KnobStyle=SCROLLBAR_KNOB_STYLE_FLAT;
		else if ( KnobStyle == 1 )
			Settings.KnobStyle=SCROLLBAR_KNOB_STYLE_DOT;
		else if ( KnobStyle == 2 )
			Settings.KnobStyle=SCROLLBAR_KNOB_STYLE_LINE;
	}	
	if ( message->FindInt32( "knobsize", &KnobSize ) == B_OK )
		Settings.KnobSize=KnobSize;
}

void ScrollBarWindow::SaveSettings()
{		
	BMessage msg;
	msg.AddPoint( "position", BPoint( Frame().left, Frame().top ) );
	
	// Save the arrow style
	if ( Settings.ArrowStyle == SCROLLBAR_ARROW_STYLE_DOUBLE )
		msg.AddInt32( "arrowstyle", 0 );
	else if ( Settings.ArrowStyle == SCROLLBAR_ARROW_STYLE_SINGLE )
		msg.AddInt32( "arrowstyle", 1 );
		
	if ( Settings.KnobType == SCROLLBAR_KNOB_TYPE_PROPORTIONAL )
		msg.AddInt32( "knobtype", 0 );
	else if ( Settings.KnobType == SCROLLBAR_KNOB_TYPE_FIXED )
		msg.AddInt32( "knobtype", 1 );
		
	if ( Settings.KnobStyle == SCROLLBAR_KNOB_STYLE_FLAT )
		msg.AddInt32( "knobstyle", 0 );
	else if ( Settings.KnobStyle == SCROLLBAR_KNOB_STYLE_DOT )
		msg.AddInt32( "knobstyle", 1 );
	else if ( Settings.KnobStyle == SCROLLBAR_KNOB_STYLE_LINE )
		msg.AddInt32( "knobstyle", 2 );
		
	msg.AddInt32( "knobsize", Settings.KnobSize );
	
	BPath path;
	status_t result = find_directory( B_USER_SETTINGS_DIRECTORY, &path );
	if ( result == B_OK )
	{
		path.Append( "OBOS_ScrollBar_Settings", true );
		BFile file( path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE );
		msg.Flatten( &file );
	}
}

bool ScrollBarWindow::QuitRequested()
{
	SaveSettings();
	be_app->PostMessage( B_QUIT_REQUESTED );
	return true;
}

