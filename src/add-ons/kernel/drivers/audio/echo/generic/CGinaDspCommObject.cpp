// ****************************************************************************
//
//  	CGinaDspCommObject.cpp
//
//		Implementation file Gina20 DSP interface class.
//
// ----------------------------------------------------------------------------
//
// This file is part of Echo Digital Audio's generic driver library.
// Copyright Echo Digital Audio Corporation (c) 1998 - 2005
// All rights reserved
// www.echoaudio.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// ****************************************************************************

#include "CEchoGals.h"
#include "CGinaDspCommObject.h"

#include "Gina20DSP.c"


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CGinaDspCommObject::CGinaDspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CGdDspCommObject( pdwRegBase, pOsSupport )
{
	strcpy( m_szCardName, "Gina" );
	m_pdwDspRegBase = pdwRegBase;		// Virtual addr DSP's register base

	m_wNumPipesOut = 10;
	m_wNumPipesIn = 4;
	m_wNumBussesOut = 10;
	m_wNumBussesIn = 4;
	m_wFirstDigitalBusOut = 8;
	m_wFirstDigitalBusIn = 2;

	m_fHasVmixer = FALSE;
	
	m_wNumMidiOut = 0;					// # MIDI out channels
	m_wNumMidiIn = 0;						// # MIDI in  channels
	
	m_pwDspCodeToLoad = pwGina20DSP;
	
	//
	// Since this card has no ASIC, mark it as loaded so everything works OK
	//
	m_bASICLoaded = TRUE;
	
}	// CGinaDspCommObject::CGinaDspCommObject( DWORD dwPhysRegBase )


//===========================================================================
//
// Destructor
//
//===========================================================================

CGinaDspCommObject::~CGinaDspCommObject()
{
}	// CGinaDspCommObject::~CGinaDspCommObject()




/****************************************************************************

	Hardware config

 ****************************************************************************/

//===========================================================================
//
// Destructor
//
//===========================================================================

ECHOSTATUS CGinaDspCommObject::SetInputClock(WORD wClock)
{
	ECHO_DEBUGPRINTF( ( "CGinaDspCommObject::SetInputClock:\n" ) );
	
	switch ( wClock )
	{
		case ECHO_CLOCK_INTERNAL : 

			// Reset the audio state to unknown (just in case)
			m_byGDCurrentClockState = GD_CLOCK_UNDEF;
			m_byGDCurrentSpdifStatus = GD_SPDIF_STATUS_UNDEF;

			SetSampleRate();
			
			m_wInputClock = wClock;
			
			ECHO_DEBUGPRINTF( ( "\tSet Gina clock to INTERNAL\n" ) );
			break;
					
		case ECHO_CLOCK_SPDIF : 
			m_pDspCommPage->byGDClockState = GD_CLOCK_SPDIFIN;
			m_pDspCommPage->byGDSpdifStatus = GD_SPDIF_STATUS_NOCHANGE;

			ClearHandshake();
			SendVector( DSP_VC_SET_GD_AUDIO_STATE );

			m_byGDCurrentClockState = GD_CLOCK_SPDIFIN;

			ECHO_DEBUGPRINTF( ( "\tSet Gina clock to SPDIF\n" ) );
			
			m_wInputClock = wClock;
			break;
			
		default :
			return ECHOSTATUS_CLOCK_NOT_SUPPORTED;
			
	}		// switch ( wClock )

	return ECHOSTATUS_OK;
	
} 		// ECHOSTATUS CGinaDspCommObject::SetInputClock()


// **** GinaDspCommObject.cpp ****
