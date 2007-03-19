// ****************************************************************************
//
//  	CDspCommObjectVmixer.cpp
//
//		Implementation file for DSP interface class with vmixer support.
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
#include "CDspCommObjectVmixer.h"


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CDspCommObjectVmixer::CDspCommObjectVmixer
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CDspCommObject( pdwRegBase, pOsSupport )
{
}	// CDspCommObjectVmixer::CDspCommObjectVmixer( DWORD dwPhysRegBase )


//===========================================================================
//
// Destructor
//
//===========================================================================

CDspCommObjectVmixer::~CDspCommObjectVmixer()
{
}	// CDspCommObjectVmixer::~CDspCommObjectVmixer()




/****************************************************************************

	Hardware setup and config

 ****************************************************************************/

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
//===========================================================================

ECHOSTATUS CDspCommObjectVmixer::GetAudioMeters
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

	pMeters->iNumBussesOut = (INT32) m_wNumBussesOut;
	for (i = 0; i < m_wNumBussesOut; i++)
	{
		pMeters->iBusOutVU[i] = 
			DSP_TO_GENERIC( ((INT32) (char) m_pDspCommPage->VUMeter[ dwCh ]) );

		pMeters->iBusOutPeak[i] = 
			DSP_TO_GENERIC( ((INT32) (char) m_pDspCommPage->PeakMeter[ dwCh ]) );
		
		dwCh++;
	}

	pMeters->iNumBussesIn = (INT32) m_wNumBussesIn;	
	for (i = 0; i < m_wNumPipesIn; i++)
	{
		pMeters->iBusInVU[i] = 
			DSP_TO_GENERIC( ((INT32) (char) m_pDspCommPage->VUMeter[ dwCh ]) );
		pMeters->iBusInPeak[i] = 
			DSP_TO_GENERIC( ((INT32) (char) m_pDspCommPage->PeakMeter[ dwCh ]) );
	
		dwCh++;
	}
	
	pMeters->iNumPipesOut = (INT32) m_wNumPipesOut;
	for (i = 0; i < m_wNumPipesOut; i++)
	{
		pMeters->iPipeOutVU[i] =
			DSP_TO_GENERIC( ((INT32) (char) m_pDspCommPage->VUMeter[ dwCh ]) );
		pMeters->iPipeOutPeak[i] = 
			DSP_TO_GENERIC( ((INT32) (char) m_pDspCommPage->PeakMeter[ dwCh ]) );
		
		dwCh++;
	}

	return ECHOSTATUS_OK;
	
} // GetAudioMeters


//===========================================================================
//
// GetPipeOutGain and SetPipeOutGain
//
// This doesn't set the line out volume; instead, it sets the
// vmixer volume.
// 
//===========================================================================

ECHOSTATUS CDspCommObjectVmixer::SetPipeOutGain
( 
	WORD wPipeOut, 
	WORD wBusOut, 
	INT32 iGain,
	BOOL fImmediate
)
{
	if (wPipeOut >= m_wNumPipesOut)
	{
		ECHO_DEBUGPRINTF( ("CDspCommObjectVmixer::SetPipeOutGain: Invalid out pipe "
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

/*
		ECHO_DEBUGPRINTF( ("CDspCommObjectVmixer::SetPipeOutGain: Out pipe %d, "
								 "out bus %d = 0x%lx\n",
								 wPipeOut,
								 wBusOut,
								 iGain) );
*/
		
		if (fImmediate)
		{
			return UpdateVmixerLevel();
		}
		
		return ECHOSTATUS_OK;
	}

	ECHO_DEBUGPRINTF( ("CDspCommObjectVmixer::SetPipeOutGain: Invalid out bus "
							 "%d\n",
							 wBusOut) );
							 
	return ECHOSTATUS_INVALID_CHANNEL;
	
}	// SetPipeOutGain


ECHOSTATUS CDspCommObjectVmixer::GetPipeOutGain
( 
	WORD wPipeOut, 
	WORD wBusOut,
	INT32 &iGain
)
{
	if (wPipeOut >= m_wNumPipesOut)
	{
		ECHO_DEBUGPRINTF( ("CDspCommObjectVmixer::GetPipeOutGain: Invalid out pipe "
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

	ECHO_DEBUGPRINTF( ("CDspCommObjectVmixer::GetPipeOutGain: Invalid out bus "
							 "%d\n",
							 wBusOut) );
							 
	return ECHOSTATUS_INVALID_CHANNEL;
	
}	// GetPipeOutGain

//===========================================================================
//
// SetBusOutGain
//
//===========================================================================

ECHOSTATUS CDspCommObjectVmixer::SetBusOutGain(WORD wBusOut,INT32 iGain)
{
	if ( wBusOut < m_wNumBussesOut )
	{
		if ( !WaitForHandshake() )
			return ECHOSTATUS_DSP_DEAD;
			
		iGain = GENERIC_TO_DSP(iGain);
		m_pDspCommPage->OutLineLevel[ wBusOut ] = (BYTE) iGain;

		ECHO_DEBUGPRINTF( ("CDspCommObjectVmixer::SetBusOutGain: Out bus %d "
								 "= %lu\n",
								 wBusOut,
								 iGain) );
								 
		return UpdateAudioOutLineLevel();

	}

	ECHO_DEBUGPRINTF( ("CDspCommObjectVmixer::SetBusOutGain: Invalid out bus "
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

ECHOSTATUS CDspCommObjectVmixer::UpdateVmixerLevel()
{
	//ECHO_DEBUGPRINTF( ( "CDspCommObjectVmixer::UpdateVmixerLevel:\n" ) );

	ClearHandshake();
	return( SendVector( DSP_VC_SET_VMIXER_GAIN ) );
}


// **** CDspCommObjectVmixer.cpp ****
