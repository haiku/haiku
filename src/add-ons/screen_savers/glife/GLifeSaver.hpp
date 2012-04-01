/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Aaron Hill <serac@hillvisions.com>
 */
#ifndef _GLIFE_GLIFESAVER_HPP
#define _GLIFE_GLIFESAVER_HPP


// Required Includes
// #include <ScreenSaver.h>
// #include <View.h>


// Constants
const int32 c_iTickSize = 50000;


// GLifeSaver Class Declaration
class GLifeSaver : public BScreenSaver
{
private:
	GLifeState	m_glsState;

	GLifeView*	m_pglvViewport;

public:
	// Constructor
				GLifeSaver( BMessage*, image_id );

	// State/Preferences Methods			
	status_t	SaveState( BMessage* ) const;
	void		RestoreState( BMessage* );
	void		StartConfig( BView* );

	// Start/Stop Methods
	status_t	StartSaver( BView*, bool );
	void		StopSaver( void );

	// Graphics Methods
	void		DirectConnected( direct_buffer_info* );
	void		DirectDraw( int32 );
};


#endif /* _GLIFE_GLIFESAVER_HPP */
