// ****************************************************************************
//
//		CLineLevel.cpp
//
//		Source file for EchoGals generic driver line level control class.
//
// 	Controls line levels for input and output busses.
//
//		Implemented as a base class with 2 derived classes, one for
//		each type of bus.
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


/****************************************************************************

	CLineLevel - Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CLineLevel::CLineLevel()
{

	Init( 0, NULL, NULL );

}	// CLineLevel::CLineLevel()


//===========================================================================
//
// Destructor
//
//===========================================================================

CLineLevel::~CLineLevel()
{
}	// CLineLevel::~CLineLevel()


//===========================================================================
//
// Initialization
//
//===========================================================================

void CLineLevel::Init
(
	WORD				wChannelIndex,	// Which channel we represent
	CEchoGals *		pEchoGals,		// For setting line levels
	INT32				iGain				// Initial gain setting
)
{
	m_iGain = 0;						// Current gain in dB X 256
	m_fMuted = FALSE;
	m_pEchoGals = pEchoGals;		// Ptr to our creator object
	m_wChannelIndex = wChannelIndex;
											// pipe index for this line

	if ( NULL != m_pEchoGals &&
		  NULL != m_pEchoGals->GetDspCommObject() &&
		  !m_pEchoGals->GetDspCommObject()->IsBoardBad() )
	{
		SetGain( iGain );				// If DSP is up, then set gain to caller's
											//	initial value
	}
}	// void CLineLevel::Init


/****************************************************************************

	CLineLevel - Set and get stuff

 ****************************************************************************/

//===========================================================================
//
// Set the mute
//
//===========================================================================

ECHOSTATUS CLineLevel::SetMute( BOOL fMute )
{
	m_fMuted = fMute;
	return SetGain(ECHOGAIN_UPDATE);
}



/****************************************************************************

	CBusInLineLevel - Construction and destruction

 ****************************************************************************/

//===========================================================================
//
//	Construction/destruction
//
//===========================================================================

CBusInLineLevel::CBusInLineLevel()
{
}	// CBusInLineLevel::CBusInLineLevel()


CBusInLineLevel::~CBusInLineLevel()
{
}	// COutLineLevel::~COutLineLevel()



/****************************************************************************

	CBusInLineLevel - Get and set stuff

 ****************************************************************************/


//===========================================================================
//
// Set the mute
//
//===========================================================================

ECHOSTATUS CBusInLineLevel::SetMute( BOOL fMute )
{
	if (fMute != m_fMuted)
	{
		m_pEchoGals->MixerControlChanged(ECHO_BUS_IN,
													MXN_MUTE,
													m_wChannelIndex);
	}
	return CLineLevel::SetMute(fMute);
}


//===========================================================================
//
// Set the gain
//
//===========================================================================

ECHOSTATUS CBusInLineLevel::SetGain
( 
	INT32 	iGain,
	BOOL 	fImmediate
)
{
	ECHOSTATUS	Status;

	if ( NULL == m_pEchoGals || 
		  NULL == m_pEchoGals->GetDspCommObject() || 
		  m_pEchoGals->GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;

	//
	// If the magic ECHOGAIN_UPDATE value was passed in,
	// use the stored gain.  Otherwise, clamp the gain.
	//
	if ( ECHOGAIN_UPDATE == iGain )
		iGain = m_iGain;
	else if ( iGain < ECHOGAIN_MININP )
		iGain = ECHOGAIN_MININP;
	else if ( iGain > ECHOGAIN_MAXINP )
		iGain = ECHOGAIN_MAXINP;
	
	//
	// Generate a control notify if necessary
	//	
	if ( m_iGain != iGain )
	{
		m_iGain = iGain;
		m_pEchoGals->MixerControlChanged(ECHO_BUS_IN,
													MXN_LEVEL,
													m_wChannelIndex);
	}

	//
	// Mute?
	//		
	if (m_fMuted)
	{
		iGain = ECHOGAIN_MININP;
	}
	
	//
	// Tell the DSP what to do
	//
	iGain <<= 1; 	// Preserver half-dB steps in input gain
	Status = 
		m_pEchoGals->GetDspCommObject()->SetBusInGain
						( m_wChannelIndex,
						  (BYTE) ( GENERIC_TO_DSP( iGain ) ) );
								  // Shift iGain up by 1 to preserve half-dB steps

	return Status;

}	// ECHOSTATUS CBusInLineLevel::SetGain



/****************************************************************************

	CBusOutLineLevel - Construction and destruction

 ****************************************************************************/

//===========================================================================
//
//	Construction/destruction
//
//===========================================================================

//
//	Construction/destruction
//
CBusOutLineLevel::CBusOutLineLevel()
{
}	// CBusOutLineLevel::CBusOutLineLevel()


CBusOutLineLevel::~CBusOutLineLevel()
{
}	// CBusOutLineLevel::~CBusOutLineLevel()




/****************************************************************************

	CBusOutLineLevel - Get and set stuff

 ****************************************************************************/


//===========================================================================
//
// Set the mute
//
//===========================================================================

ECHOSTATUS CBusOutLineLevel::SetMute( BOOL fMute )
{
	if (fMute != m_fMuted)
	{
		m_pEchoGals->MixerControlChanged(ECHO_BUS_OUT,
													MXN_MUTE,
													m_wChannelIndex);
	}
	return CLineLevel::SetMute(fMute);
}


//===========================================================================
//
// Set the gain
//
//===========================================================================

ECHOSTATUS CBusOutLineLevel::SetGain
( 
	INT32 	iGain,
	BOOL 	fImmediate
)
{
	ECHOSTATUS	Status = ECHOSTATUS_OK;

	if ( NULL == m_pEchoGals || 
		  NULL == m_pEchoGals->GetDspCommObject() || 
		  m_pEchoGals->GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;

	//
	// If iGain is ECHOGAIN_UPDATE, then the caller
	// wants this function to re-do the gain setting
	// with the currently stored value
	//
	// Otherwise, clamp the gain setting
	//
	if ( ECHOGAIN_UPDATE == iGain )
		iGain = m_iGain;
	else if ( iGain < ECHOGAIN_MUTED )
		iGain = ECHOGAIN_MUTED;
	else if ( iGain > ECHOGAIN_MAXOUT )
		iGain = ECHOGAIN_MAXOUT;
		
	//
	// Mark this control as changed
	//
	if ( m_iGain != iGain )
	{
		m_iGain = iGain;

		if ( ECHOSTATUS_OK == Status )
			Status = m_pEchoGals->MixerControlChanged(ECHO_BUS_OUT,
																	MXN_LEVEL,
																	m_wChannelIndex);
	}

	//
	// Set the gain to mute if this bus is muted
	//
	int iGainTemp = iGain;
	if ( m_fMuted )
		iGainTemp = ECHOGAIN_MUTED;

	//
	// Tell all the monitors for this output bus to 
	// update their gains
	//
	m_pEchoGals->AdjustMonitorsForBusOut(m_wChannelIndex);
	
	//
	// Tell all the output pipes for this output bus
	// to update their gains
	//
	m_pEchoGals->AdjustPipesOutForBusOut(m_wChannelIndex,iGainTemp);
	
	return Status;
	
}	// ECHOSTATUS CBusOutLineLevel::SetGain


// **** CLineLevel.cpp ****
