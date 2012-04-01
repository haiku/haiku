/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Aaron Hill <serac@hillvisions.com>
 */


#include <stdio.h>
#include <Slider.h>
#include <StringView.h>
#include <View.h>

#include "GLifeState.hpp"
#include "GLifeConfig.hpp"


// ------------------------------------------------------
//  GLifeConfig Class Constructor Definition
GLifeConfig::GLifeConfig( BRect rFrame, GLifeState* pglsState )
	:	BView( rFrame, "", B_FOLLOW_NONE, B_WILL_DRAW ),
		m_pglsState( pglsState )
{
	// Static Labels
	AddChild( new BStringView( BRect(10, 0, 240, 12), B_EMPTY_STRING,
							   "Open GL \"Game of Life\" Screen Saver" ) );
	AddChild( new BStringView( BRect(16, 14, 240, 26), B_EMPTY_STRING,
							   "by Aaron Hill" ) );

	// Sliders
	m_pbsGridWidth = new BSlider( BRect(10, 34, 234, 84), "GridWidth",
								  "Width of Grid : ",
								  new BMessage( e_midGridWidth ),
								  10, 100, B_BLOCK_THUMB );
	m_pbsGridWidth->SetHashMarks( B_HASH_MARKS_BOTTOM );
	m_pbsGridWidth->SetLimitLabels( "10", "100" );
	m_pbsGridWidth->SetValue( pglsState->GridWidth() );
	m_pbsGridWidth->SetHashMarkCount( 10 );
	AddChild( m_pbsGridWidth );

	m_pbsGridHeight = new BSlider( BRect(10, 86, 234, 136), "GridHeight",
								   "Height of Grid : ",
								   new BMessage( e_midGridHeight ),
								   10, 100, B_BLOCK_THUMB );
	m_pbsGridHeight->SetHashMarks( B_HASH_MARKS_BOTTOM );
	m_pbsGridHeight->SetLimitLabels( "10", "100" );
	m_pbsGridHeight->SetValue( pglsState->GridHeight() );
	m_pbsGridHeight->SetHashMarkCount( 10 );
	AddChild( m_pbsGridHeight );

	m_pbsBorder = new BSlider( BRect(10, 138, 234, 188), "Border",
							   "Overlap Border : ",
							   new BMessage( e_midBorder ),
							   0, 10, B_BLOCK_THUMB );
	m_pbsBorder->SetHashMarks( B_HASH_MARKS_BOTTOM );
	m_pbsBorder->SetLimitLabels( "0", "10" );
	m_pbsBorder->SetValue( pglsState->Border() );
	m_pbsBorder->SetHashMarkCount( 11 );
	AddChild( m_pbsBorder );
}

// ------------------------------------------------------
//  GLifeConfig Class AttachedToWindow Definition
void GLifeConfig::AttachedToWindow( void )
{
	SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );
	
	m_pbsGridWidth->SetTarget( this );
	m_pbsGridHeight->SetTarget( this );
	m_pbsBorder->SetTarget( this );

#ifdef _USE_ASYNCHRONOUS
	
	m_uiWindowFlags = Window()->Flags();
	Window()->SetFlags( m_uiWindowFlags | B_ASYNCHRONOUS_CONTROLS );

#endif
}

// ------------------------------------------------------
//  GLifeConfig Class MessageReceived Definition
void GLifeConfig::MessageReceived( BMessage* pbmMessage )
{
	char	szNewLabel[64];
	int32	iValue;
	
	switch( pbmMessage->what )
	{
	case e_midGridWidth:
		m_pglsState->GridWidth() = ( iValue = m_pbsGridWidth->Value() );
		sprintf( szNewLabel, "Width of Grid : %li", iValue );
		m_pbsGridWidth->SetLabel( szNewLabel );
		break;
	case e_midGridHeight:
		m_pglsState->GridHeight() = ( iValue = m_pbsGridHeight->Value() );
		sprintf( szNewLabel, "Height of Grid : %li", iValue );
		m_pbsGridHeight->SetLabel( szNewLabel );
		break;
	case e_midBorder:
		m_pglsState->Border() = ( iValue = m_pbsBorder->Value() );
		sprintf( szNewLabel, "Overlap Border : %li", iValue );
		m_pbsBorder->SetLabel( szNewLabel );
		break;
		
	default:
		BView::MessageReceived( pbmMessage );
		break;
	}
}
