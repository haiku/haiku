// ****************************************************************************
//
//  	CGdDspCommObject.cpp
//
//		Implementation file for Gina20/Darla20 DSP interface class.
//
//		Darla20 and Gina20 are very similar; this class is used for both.
//		CGinaDspCommObject and CDarlaDspCommObject dervie from this class, in
//		turn.
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


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CGdDspCommObject::CGdDspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CDspCommObject( pdwRegBase, pOsSupport )
{
	m_byGDCurrentSpdifStatus = GD_SPDIF_STATUS_UNDEF;
	m_byGDCurrentClockState = GD_CLOCK_UNDEF;
}	// CGdDspCommObject::CGinaDspCommObject( DWORD dwPhysRegBase )


//===========================================================================
//
// Destructor
//
//===========================================================================

CGdDspCommObject::~CGdDspCommObject()
{
}	// CGdDspCommObject::~CGdDspCommObject()




/****************************************************************************

	Hardware config

 ****************************************************************************/

//===========================================================================
//
//	Called after LoadFirmware to restore old gains, meters on, monitors, etc.
//	No error checking is done here.
//
//===========================================================================

void CGdDspCommObject::RestoreDspSettings()
{
	
	m_pDspCommPage->byGDClockState = GD_CLOCK_UNDEF;
	m_pDspCommPage->byGDSpdifStatus = GD_SPDIF_STATUS_UNDEF;
	CDspCommObject::RestoreDspSettings();
	
}	// void CLaylaDspCommObject::RestoreDspSettings()


//===========================================================================
//
// SetSampleRate
// 
// Set the audio sample rate for Gina/Darla
//
// For Gina and Darla, three parameters need to be set: the resampler state,
// the clock state, and the S/PDIF output status state.  The idea is that
// these parameters are changed as infrequently as possible.
//
// Resampling is no longer supported.
//
//===========================================================================

DWORD CGdDspCommObject::SetSampleRate( DWORD dwNewSampleRate )
{
	BYTE		byGDClockState, byGDSpdifStatus;
	DWORD		fUseSpdifInClock;

	if ( !WaitForHandshake() )
		return ECHOSTATUS_DSP_DEAD;
	
	//
	//	Pick the clock
	// 
	fUseSpdifInClock = (GetCardType() == GINA) &&
							 (GetInputClock() == ECHO_CLOCK_SPDIF);
	if (fUseSpdifInClock)
	{
		byGDClockState = GD_CLOCK_SPDIFIN;
	}		
	else
	{
		switch (dwNewSampleRate)
		{
			case 44100 :
				byGDClockState = GD_CLOCK_44;
				break;

			case 48000 :
				byGDClockState = GD_CLOCK_48;
				break;

			default :
				byGDClockState = GD_CLOCK_NOCHANGE;
				break;
		}
	}

	if ( m_byGDCurrentClockState == byGDClockState )
	{
		byGDClockState = GD_CLOCK_NOCHANGE;
	}

	//
	// Select S/PDIF output status
	//
	byGDSpdifStatus = SelectGinaDarlaSpdifStatus( dwNewSampleRate );

	//
	// Write the audio state to the comm page
	//
	m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );
	m_pDspCommPage->byGDClockState = byGDClockState;
	m_pDspCommPage->byGDSpdifStatus = byGDSpdifStatus;
	m_pDspCommPage->byGDResamplerState = 3;				// magic number - 
																		// should always be 3

	//
	// Send command to DSP
	//
	ClearHandshake();
	SendVector( DSP_VC_SET_GD_AUDIO_STATE );

	//			
	// Save the new audio state
	//
	if ( byGDClockState != GD_CLOCK_NOCHANGE )
		m_byGDCurrentClockState = byGDClockState;

	if ( byGDSpdifStatus != GD_SPDIF_STATUS_NOCHANGE )
		m_byGDCurrentSpdifStatus = byGDSpdifStatus;

	return dwNewSampleRate;
	
} // DWORD CGdDspCommObject::SetSampleRate( DWORD dwNewSampleRate )


//===========================================================================
// 
// SelectGinaDarlaSpdifStatus
// 
//===========================================================================

BYTE CGdDspCommObject::SelectGinaDarlaSpdifStatus( DWORD dwNewSampleRate )
{
	BYTE byNewStatus = 0;
	
	switch (dwNewSampleRate)
	{
		case 44100 :
			byNewStatus = GD_SPDIF_STATUS_44;
			break;
			
		case 48000 :
			byNewStatus = GD_SPDIF_STATUS_48;
			break;
			
		default :
			byNewStatus = GD_SPDIF_STATUS_NOCHANGE;
			break;
	}
	
	if (byNewStatus == m_byGDCurrentSpdifStatus)
	{
		byNewStatus = GD_SPDIF_STATUS_NOCHANGE;
	}
	
	return byNewStatus;
	
}	// BYTE CGdDspCommObject::SelectGinaDarlaSpdifStatus( DWORD dwNewSampleRate )

// **** GinaDspCommObject.cpp ****
