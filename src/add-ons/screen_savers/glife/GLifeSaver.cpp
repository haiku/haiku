/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Aaron Hill <serac@hillvisions.com>
 *		Alexander von Gluck <kallisti5@unixzen.com>
 */
 

#include <stdio.h>
#include <stdlib.h>
#include <GLView.h>
#include <ScreenSaver.h>
#include <View.h>

#include "GLifeGrid.h"
#include "GLifeState.h"
#include "GLifeConfig.h"
#include "GLifeView.h"
#include "GLifeSaver.h"

// ------------------------------------------------------
//  GLifeSaver Class Constructor Definition
GLifeSaver::GLifeSaver( BMessage* pbmPrefs, image_id iidImage )
	:	BScreenSaver( pbmPrefs, iidImage )
{
	// Check for preferences
	if ( !( pbmPrefs->IsEmpty() ) )
		RestoreState( pbmPrefs );
	
	// Seed random number generator
	srandom( system_time() );
}

// ------------------------------------------------------
//  GLifeSaver Class SaveState Definition
status_t GLifeSaver::SaveState( BMessage* pbmPrefs ) const
{
	return m_glsState.SaveState( pbmPrefs );
}

// ------------------------------------------------------
//  GLifeSaver Class RestoreState Definition
void GLifeSaver::RestoreState( BMessage* pbmPrefs )
{
	m_glsState.RestoreState( pbmPrefs );
}

// ------------------------------------------------------
//  GLifeSaver Class StartConfig Definition
void GLifeSaver::StartConfig( BView* pbvView )
{
	// Setup the "config" class
	GLifeConfig* pglcConfig = new GLifeConfig( pbvView->Bounds(),
											   &m_glsState );
	pbvView->AddChild( pglcConfig );
}

// ------------------------------------------------------
//  GLifeSaver Class StartSaver Definition
status_t GLifeSaver::StartSaver( BView* pbvView, bool bPreview )
{
	if ( bPreview )
	{
		// We do not use the preview option
		m_pglvViewport = 0;
		return B_ERROR;
	}
	else
	{
		SetTickSize( c_iTickSize );
		
		m_pglvViewport = new GLifeView( pbvView->Bounds(),
										"GLifeView",
										B_FOLLOW_NONE,
										BGL_RGB | BGL_DEPTH | BGL_DOUBLE,
										&m_glsState );
		pbvView->AddChild( m_pglvViewport );
		
		return B_OK;
	}
}

// ------------------------------------------------------
//  GLifeSaver Class StopSaver Definition
void GLifeSaver::StopSaver( void )
{
	if ( m_pglvViewport )
		m_pglvViewport->EnableDirectMode( false );
}

// ------------------------------------------------------
//  GLifeSaver Class DirectConnected Definition
void GLifeSaver::DirectConnected( direct_buffer_info* pdbiInfo )
{
	m_pglvViewport->DirectConnected( pdbiInfo );
	m_pglvViewport->EnableDirectMode( true );
}

// ------------------------------------------------------
//  GLifeSaver Class DirectDraw Definition
void GLifeSaver::DirectDraw( int32 iFrame )
{
	m_pglvViewport->Advance();
}

// ------------------------------------------------------
//  Main Instantiation Function
extern "C" _EXPORT BScreenSaver*
instantiate_screen_saver( BMessage* pbmPrefs, image_id iidImage )
{
	return new GLifeSaver( pbmPrefs, iidImage );
}
