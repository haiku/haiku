/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Aaron Hill <serac@hillvisions.com>
 */
#ifndef _GLIFE_GLIFESTATE_HPP
#define _GLIFE_GLIFESTATE_HPP


// Constants
const int32 c_iDefGridWidth = 60;
const int32 c_iDefGridHeight = 40;
const int32 c_iDefBorder = 0;

// GLifeState Class Declaration & Definition
class GLifeState
{
private:
	int32	m_iGridWidth;
	int32	m_iGridHeight;
	int32	m_iBorder;
	
public:
	// Constructor
				GLifeState( void ) : m_iGridWidth(c_iDefGridWidth),
									 m_iGridHeight(c_iDefGridHeight),
									 m_iBorder(c_iDefBorder) { };

	// Save/Restore State Methods
	status_t	SaveState( BMessage* pbmPrefs ) const
	{
		// Store current preferences
		pbmPrefs->AddInt32( "m_iGridWidth", m_iGridWidth );
		pbmPrefs->AddInt32( "m_iGridHeight", m_iGridHeight );
		pbmPrefs->AddInt32( "m_iBorder", m_iBorder );
					
		return B_OK;
	};
	
	void		RestoreState( BMessage* pbmPrefs )
	{
		// Retrieve preferences, substituting defaults
		if ( pbmPrefs->FindInt32( "m_iGridWidth",
								  &m_iGridWidth ) != B_OK )
			m_iGridWidth = c_iDefGridWidth;
		if ( pbmPrefs->FindInt32( "m_iGridHeight",
								  &m_iGridHeight ) != B_OK )
			m_iGridHeight = c_iDefGridHeight;
		if ( pbmPrefs->FindInt32( "m_iBorder",
								  &m_iBorder ) != B_OK )
			m_iBorder = c_iDefBorder;
	};
	
	// Accessor Methods
	int32&		GridWidth( void ) { return m_iGridWidth; };
	int32&		GridHeight( void ) { return m_iGridHeight; };
	int32&		Border( void ) { return m_iBorder; };
};


#endif /* _GLIFE_GLIFESTATE_HPP */
