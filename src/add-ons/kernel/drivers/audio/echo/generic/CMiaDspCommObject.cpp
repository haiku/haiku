// ****************************************************************************
//
//  	CMiaDspCommObject.cpp
//
//		Implementation file for EchoGals generic driver Darla 24 DSP
//		interface class.
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
#include "CMiaDspCommObject.h"

#include "MiaDSP.c"


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CMiaDspCommObject::CMiaDspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CDspCommObject( pdwRegBase, pOsSupport )
{
	strcpy( m_szCardName, "Mia" );
	m_pdwDspRegBase = pdwRegBase;		// Virtual addr DSP's register base

	m_wNumPipesOut = 8;
	m_wNumPipesIn = 4;
	m_wNumBussesOut = 4;
	m_wNumBussesIn = 4;
	m_wFirstDigitalBusOut = 2;
	m_wFirstDigitalBusIn = 2;

	m_fHasVmixer = TRUE;
		
	m_wNumMidiOut = 0;					// # MIDI out channels
	m_wNumMidiIn = 0;						// # MIDI in  channels

	m_pDspCommPage->dwSampleRate = SWAP( (DWORD) 44100 );

	m_bHasASIC = FALSE;

	m_pwDspCodeToLoad = pwMiaDSP;

	m_byDigitalMode = DIGITAL_MODE_NONE;
	
	//
	// Since this card has no ASIC, mark it as loaded so everything works OK
	//
	m_bASICLoaded = TRUE;
	
}	// CMiaDspCommObject::CMiaDspCommObject( DWORD dwPhysRegBase )


//===========================================================================
//
// Destructor
//
//===========================================================================

CMiaDspCommObject::~CMiaDspCommObject()
{
}	// CMiaDspCommObject::~CMiaDspCommObject()




/****************************************************************************

	Hardware setup and config

 ****************************************************************************/

//===========================================================================
//
//	Set the input clock
//
//===========================================================================

ECHOSTATUS CMiaDspCommObject::SetInputClock(WORD wClock)
{
	DWORD	dwSampleRate = GetSampleRate();

	ECHO_DEBUGPRINTF( ("CMiaDspCommObject::SetInputClock:\n") );

	switch ( wClock )
	{
		case ECHO_CLOCK_INTERNAL : 
		{
			ECHO_DEBUGPRINTF( ( "\tSet Mia clock to INTERNAL\n" ) );
	
			// If the sample rate is out of range for some reason, set it
			// to a reasonable value.  mattg
			if ( ( dwSampleRate < 8000  ) ||
			     ( dwSampleRate > 96000 ) )
			{
				dwSampleRate = 48000;
			}

			break;
		} // CLK_CLOCKININTERNAL

		case ECHO_CLOCK_SPDIF :
		{
			ECHO_DEBUGPRINTF( ( "\tSet Mia clock to SPDIF\n" ) );
			break;
		} // CLK_CLOCKINSPDIF

		default :
			ECHO_DEBUGPRINTF(("Input clock 0x%x not supported for Mia\n",wClock));
			ECHO_DEBUGBREAK();
				return ECHOSTATUS_CLOCK_NOT_SUPPORTED;
	}	// switch (wInputClock)
	
	m_wInputClock = wClock;

	SetSampleRate( dwSampleRate );

	return ECHOSTATUS_OK;
}	// ECHOSTATUS CMiaDspCommObject::SetInputClock


//===========================================================================
//
// SetSampleRate
// 
// Set the audio sample rate for Mia
//
//===========================================================================

DWORD CMiaDspCommObject::SetSampleRate( DWORD dwNewSampleRate )
{
	//
	// Set the sample rate
	//
	DWORD dwControlReg = MIA_48000;

	switch ( dwNewSampleRate )
	{
		case 96000 :
			dwControlReg = MIA_96000;
			break;
			
		case 88200 :
			dwControlReg = MIA_88200;
			break;
			
		case 44100 : 
			dwControlReg = MIA_44100;
			break;
			
		case 32000 :
			dwControlReg = MIA_32000;
			break;
			
		case 24000 :
			dwControlReg = MIA_24000;
			break;
			
		case 22050 :
			dwControlReg = MIA_22050;
			break;

		case 16000 :
			dwControlReg = MIA_16000;
			break;

		case 12000 :
			dwControlReg = MIA_12000;
			break;
			
		case 11025 :
			dwControlReg = MIA_11025;
			break;
			
		case 8000 :
			dwControlReg = MIA_8000;
			break;
	}

	//
	// Override the clock setting if this Mia is set to S/PDIF clock
	//	
	if ( ECHO_CLOCK_SPDIF == GetInputClock() )
	{
		dwControlReg &= MIA_SRC_MASK;
		dwControlReg |= MIA_SPDIF;
	}
	
	//
	//	Set the control register if it has changed
	//
	if (dwControlReg != GetControlRegister())
	{
		if ( !WaitForHandshake() )
			return 0xffffffff;

		//
		// Set the values in the comm page; the dwSampleRate
		// field isn't used by the DSP, but is read by the call
		// to GetSampleRate below
		//		
		m_pDspCommPage->dwSampleRate = SWAP( dwNewSampleRate );
		SetControlRegister( dwControlReg );

		//
		//	Poke the DSP
		// 
		ClearHandshake();
		SendVector( DSP_VC_UPDATE_CLOCKS );
	}
	
	return GetSampleRate();
		
} // DWORD CMiaDspCommObject::SetSampleRate( DWORD dwNewSampleRate )



//===========================================================================
//
// GetAudioMeters
// 
// Meters are written to the comm page by the DSP as follows:
//
// Output busses
// Input busses
// Output pipes (vmixer cards only)
//
// This function is overridden for vmixer cards
//
//===========================================================================

ECHOSTATUS CMiaDspCommObject::GetAudioMeters
(
	PECHOGALS_METERS	pMeters
)
{
	WORD i;

	pMeters->iNumPipesIn = 0;
	
	//
	//	Output 
	// 
	DWORD dwCh = 0;

	pMeters->iNumBussesOut = (int) m_wNumBussesOut;
	for (i = 0; i < m_wNumBussesOut; i++)
	{
		pMeters->iBusOutVU[i] = 
			DSP_TO_GENERIC( ((int) (char) m_pDspCommPage->VULevel[ dwCh ]) );

		pMeters->iBusOutPeak[i] = 
			DSP_TO_GENERIC( ((int) (char) m_pDspCommPage->PeakMeter[ dwCh ]) );
		
		dwCh++;
	}

	pMeters->iNumBussesIn = (int) m_wNumBussesIn;	
	for (i = 0; i < m_wNumPipesIn; i++)
	{
		pMeters->iBusInVU[i] = 
			DSP_TO_GENERIC( ((int) (char) m_pDspCommPage->VULevel[ dwCh ]) );
		pMeters->iBusInPeak[i] = 
			DSP_TO_GENERIC( ((int) (char) m_pDspCommPage->PeakMeter[ dwCh ]) );
	
		dwCh++;
	}
	
	pMeters->iNumPipesOut = (int) m_wNumPipesOut;
	for (i = 0; i < m_wNumPipesOut; i++)
	{
		pMeters->iPipeOutVU[i] =
			DSP_TO_GENERIC( ((int) (char) m_pDspCommPage->VULevel[ dwCh ]) );
		pMeters->iPipeOutPeak[i] = 
			DSP_TO_GENERIC( ((int) (char) m_pDspCommPage->PeakMeter[ dwCh ]) );
		
		dwCh++;
	}

	return ECHOSTATUS_OK;
	
} // GetAudioMeters


//===========================================================================
//
// GetPipeOutGain and SetPipeOutGain
//
// On Mia, this doesn't set the line out volume; instead, it sets the
// vmixer volume.
// 
//===========================================================================

ECHOSTATUS CMiaDspCommObject::SetPipeOutGain
( 
	WORD wPipeOut, 
	WORD wBusOut, 
	int iGain,
	BOOL fImmediate
)
{
	if (wPipeOut >= m_wNumPipesOut)
	{
		ECHO_DEBUGPRINTF( ("CDspCommObject::SetPipeOutGain: Invalid out pipe "
								 "%d\n",
								 wPipeOut) );
								 
		return ECHOSTATUS_INVALID_CHANNEL;
	}
	
	iGain = GENERIC_TO_DSP(iGain);

	if ( wBusOut < m_wNumBussesOut )
	{
		if ( !WaitForHandshake() )
			return ECHOSTATUS_DSP_DEAD;
			
		DWORD dwIndex = wBusOut * m_wNumPipesOut + wPipeOut;
		m_pDspCommPage->byVmixerLevel[ dwIndex ] = (BYTE) iGain;

		ECHO_DEBUGPRINTF( ("CMiaDspCommObject::SetPipeOutGain: Out pipe %d, "
								 "out bus %d = %u\n",
								 wPipeOut,
								 wBusOut,
								 iGain) );
		
		if (fImmediate)
		{
			return UpdateVmixerLevel();
		}
		
		return ECHOSTATUS_OK;
	}

	ECHO_DEBUGPRINTF( ("CMiaDspCommObject::SetPipeOutGain: Invalid out bus "
							 "%d\n",
							 wBusOut) );
							 
	return ECHOSTATUS_INVALID_CHANNEL;
	
}	// SetPipeOutGain


ECHOSTATUS CMiaDspCommObject::GetPipeOutGain
( 
	WORD wPipeOut, 
	WORD wBusOut,
	int &iGain
)
{
	if (wPipeOut >= m_wNumPipesOut)
	{
		ECHO_DEBUGPRINTF( ("CMiaDspCommObject::GetPipeOutGain: Invalid out pipe "
								 "%d\n",
								 wPipeOut) );
								 
		return ECHOSTATUS_INVALID_CHANNEL;
	}

	if (wBusOut < m_wNumBussesOut)
	{
		iGain = m_pDspCommPage->byVmixerLevel[ wBusOut * m_wNumPipesOut + wPipeOut ];
		iGain = DSP_TO_GENERIC(iGain);
		return ECHOSTATUS_OK;		
	}

	ECHO_DEBUGPRINTF( ("CMiaDspCommObject::GetPipeOutGain: Invalid out bus "
							 "%d\n",
							 wBusOut) );
							 
	return ECHOSTATUS_INVALID_CHANNEL;
	
}	// GetPipeOutGain

//===========================================================================
//
// SetBusOutGain
//
//===========================================================================

ECHOSTATUS CMiaDspCommObject::SetBusOutGain(WORD wBusOut,int iGain)
{
	if ( wBusOut < m_wNumBussesOut )
	{
		if ( !WaitForHandshake() )
			return ECHOSTATUS_DSP_DEAD;
			
		iGain = GENERIC_TO_DSP(iGain);
		m_pDspCommPage->OutLineLevel[ wBusOut ] = (BYTE) iGain;

		ECHO_DEBUGPRINTF( ("CMiaDspCommObject::SetBusOutGain: Out bus %d "
								 "= %u\n",
								 wBusOut,
								 iGain) );
								 
		return UpdateAudioOutLineLevel();

	}

	ECHO_DEBUGPRINTF( ("CMiaDspCommObject::SetBusOutGain: Invalid out bus "
							 "%d\n",
							 wBusOut) );
							 
	return ECHOSTATUS_INVALID_CHANNEL;
}


//===========================================================================
//
// Tell the DSP to read and update vmixer levels 
//	from the comm page.
//
//===========================================================================

ECHOSTATUS CMiaDspCommObject::UpdateVmixerLevel()
{
	ECHO_DEBUGPRINTF( ( "CMiaDspCommObject::UpdateVmixerLevel:\n" ) );

	ClearHandshake();
	return( SendVector( DSP_VC_SET_VMIXER_GAIN ) );
}


// **** CMiaDspCommObject.cpp ****
