// ****************************************************************************
//
//  	CGinaDspCommObject.cpp
//
//		Implementation file Gina20 DSP interface class.
//
//		Copyright Echo Digital Audio Corporation (c) 1998 - 2002
//		All rights reserved
//		www.echoaudio.com
//		
//		Permission is hereby granted, free of charge, to any person obtaining a
//		copy of this software and associated documentation files (the
//		"Software"), to deal with the Software without restriction, including
//		without limitation the rights to use, copy, modify, merge, publish,
//		distribute, sublicense, and/or sell copies of the Software, and to
//		permit persons to whom the Software is furnished to do so, subject to
//		the following conditions:
//		
//		- Redistributions of source code must retain the above copyright
//		notice, this list of conditions and the following disclaimers.
//		
//		- Redistributions in binary form must reproduce the above copyright
//		notice, this list of conditions and the following disclaimers in the
//		documentation and/or other materials provided with the distribution.
//		
//		- Neither the name of Echo Digital Audio, nor the names of its
//		contributors may be used to endorse or promote products derived from
//		this Software without specific prior written permission.
//
//		THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//		EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//		MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//		IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
//		ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//		TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//		SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
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
