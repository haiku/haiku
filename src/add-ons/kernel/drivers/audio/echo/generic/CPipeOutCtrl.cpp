// ****************************************************************************
//
//		CPipeOutCtrl.cpp
//
//		Class to control output pipes on cards with or without vmixers.
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
#include "CPipeOutCtrl.h"

extern INT32 PanToDb( INT32 iPan );


//*****************************************************************************
//
// Destructor (this class uses the default constructor)
//
//*****************************************************************************

CPipeOutCtrl::~CPipeOutCtrl()
{
	Cleanup();	
}	


//*****************************************************************************
//
// Init
//
//*****************************************************************************

ECHOSTATUS CPipeOutCtrl::Init(CEchoGals *pEG)
{
	DWORD	dwBytes;
	WORD	wPipe,wStereoBus;

	m_Gains = NULL;
	m_Mutes = NULL;
	m_Pans = NULL;
	m_PanDbs = NULL;
	
	//
	// Cache stuff
	//
	m_pEG = pEG;
	m_wNumPipesOut = pEG->GetNumPipesOut();
	m_wNumBussesOut = pEG->GetNumBussesOut();
	m_fHasVmixer = pEG->HasVmixer();
	
	//
	// Allocate the arrays
	//
	if (m_fHasVmixer)
	{
		WORD wNumStereoBusses = m_wNumBussesOut >> 1;
		
		//
		// Allocate arrays for vmixer support
		//
		dwBytes = sizeof(INT8) * m_wNumPipesOut * wNumStereoBusses;
		OsAllocateNonPaged(dwBytes,(void **) &m_Gains);
		if (NULL == m_Gains)
		{
			return ECHOSTATUS_NO_MEM;
		}

		dwBytes = sizeof(BYTE) * m_wNumPipesOut * wNumStereoBusses;
		OsAllocateNonPaged(dwBytes,(void **) &m_Mutes);
		if (NULL == m_Mutes)
		{
			Cleanup();
			return ECHOSTATUS_NO_MEM;
		}
		
		dwBytes = sizeof(WORD) * m_wNumPipesOut * wNumStereoBusses;
		OsAllocateNonPaged(dwBytes,(void **) &m_Pans);
		if (NULL == m_Pans)
		{
			Cleanup();
			return ECHOSTATUS_NO_MEM;
		}
		
		dwBytes = sizeof(PAN_DB) * m_wNumPipesOut * wNumStereoBusses;
		OsAllocateNonPaged(dwBytes,(void **) &m_PanDbs);
		if (NULL == m_PanDbs)
		{
			Cleanup();
			return ECHOSTATUS_NO_MEM;
		}
		
		//
		// Initialize pans and mutes
		//
		for (wPipe = 0; wPipe < m_wNumPipesOut; wPipe++)
			for (wStereoBus = 0; wStereoBus < wNumStereoBusses; wStereoBus++)
			{
				WORD wIndex;
				
				wIndex = GetIndex(wPipe,wStereoBus << 1);

				//
				//	Pans
				// 
				if (0 == (wPipe & 1))
				{
					//
					// Even channel - pan hard left
					//
					m_Pans[wIndex] = 0;
					m_PanDbs[wIndex].iLeft = 0;
					m_PanDbs[wIndex].iRight = GENERIC_TO_DSP( ECHOGAIN_MUTED );
				}
				else
				{
					//
					// Odd channel - pan hard right
					//
					m_Pans[wIndex] = MAX_MIXER_PAN;
					m_PanDbs[wIndex].iLeft = GENERIC_TO_DSP( ECHOGAIN_MUTED );
					m_PanDbs[wIndex].iRight = 0;
				}
				
				//
				// Mutes
				//
				if ((wPipe >> 1) == wStereoBus)
				{
					m_Mutes[wIndex] = FALSE;
				}
				else
				{
					m_Mutes[wIndex] = TRUE;
				}
				
				//
				// Set the gain to the DSP; use fImmedate = FALSE here
				// to make this faster
				//
				SetGain(wPipe,wStereoBus << 1,ECHOGAIN_UPDATE,FALSE);
				
			}
			
			//
			// Set the gain one more time with the immediate flag set to
			// make sure the DSP gets the message
			//
			SetGain(0,0,ECHOGAIN_UPDATE,TRUE);
	}
	else
	{
		//
		// Allocate arrays for no vmixer support - don't need pans
		//
		dwBytes = sizeof(INT8) * m_wNumPipesOut;
		OsAllocateNonPaged(dwBytes,(void **) &m_Gains);
		if (NULL == m_Gains)
		{
			return ECHOSTATUS_NO_MEM;
		}

		dwBytes = sizeof(BYTE) * m_wNumPipesOut;
		OsAllocateNonPaged(dwBytes,(void **) &m_Mutes);
		if (NULL == m_Mutes)
		{
			OsFreeNonPaged(m_Gains);
			return ECHOSTATUS_NO_MEM;
		}
		
	}	  
	
	return ECHOSTATUS_OK;
	
}	// Init


//*****************************************************************************
//
// Cleanup - free allocated memory
//
//*****************************************************************************

void CPipeOutCtrl::Cleanup()
{
	if (m_Gains)
		OsFreeNonPaged(m_Gains);
		
	if (m_Mutes)
		OsFreeNonPaged(m_Mutes);
		
	if (m_Pans)
		OsFreeNonPaged(m_Pans);
		
	if (m_PanDbs)
		OsFreeNonPaged(m_PanDbs);
	
}	// Cleanup


//*****************************************************************************
//
// Set and get gain
//
// For cards without vmixers, output bus gain is not handled by the DSP.
// Instead, the driver adjusts the output pipe volumes by the output bus gain
// and sends that value to the DSP.
//
// For cards with vmixers, the output bus gain is handled by the DSP, so
// the gain setting does not need to take into account the output bus gain
// stored by the driver.
//
//*****************************************************************************

ECHOSTATUS CPipeOutCtrl::SetGain
(
	WORD 	wPipeOut, 
	WORD 	wBusOut, 
	INT32 iGain,
	BOOL 	fImmediate
)
{
	INT32 iBusOutGain;
	ECHOSTATUS Status;

	if ( NULL == m_pEG)
		return ECHOSTATUS_DSP_DEAD;
		
	if (!m_fHasVmixer && (wPipeOut != wBusOut))
		return ECHOSTATUS_OK;
	
	if ((NULL == m_Gains) || (NULL == m_Mutes))
		return ECHOSTATUS_NO_MEM;
	
	if ((wPipeOut >= m_wNumPipesOut) || (wBusOut >= m_wNumBussesOut))
	{
		ECHO_DEBUGPRINTF(("CPipeOutCtrl::SetGain - out of range pipe %d bus %d\n",
								wPipeOut,wBusOut));
		return ECHOSTATUS_INVALID_PARAM;		
	}
	
	WORD wIndex = GetIndex(wPipeOut,wBusOut);

	/*
	ECHO_DEBUGPRINTF(("CPipeOutCtrl::SetGain pipe %d  bus %d  gain 0x%lx  index %d\n",
							wPipeOut,wBusOut,iGain,wIndex));
	*/
	
	if (ECHOGAIN_UPDATE == iGain)
	{
		iGain = DSP_TO_GENERIC( m_Gains[ wIndex ] );
	}
	else
	{
		if (iGain > ECHOGAIN_MAXOUT)
			iGain = ECHOGAIN_MAXOUT;
		else if (iGain < ECHOGAIN_MUTED)
			iGain = ECHOGAIN_MUTED;
	
		m_Gains[ wIndex ] = (INT8) GENERIC_TO_DSP( iGain );
		
		//
		// Store the notify
		//
		m_pEG->MixerControlChanged(ECHO_PIPE_OUT,MXN_LEVEL,wPipeOut,wBusOut);
	}
		
	if (m_fHasVmixer)
	{
		wBusOut &= 0xfffe;
		
		if (NULL == m_Pans)
			return ECHOSTATUS_NO_MEM;
	
		//
		// For vmixer cards, the DSP handles the output bus gain, 
		// so no need to account for it here.  Vmixer output pipes
		// do have to handle panning.
		//
		INT32 iLeft = iGain + DSP_TO_GENERIC( m_PanDbs[wIndex].iLeft );
		INT32 iRight = iGain + DSP_TO_GENERIC( m_PanDbs[wIndex].iRight );
		
		//
		// Add master gain values
		//
		iBusOutGain = m_pEG->m_BusOutLineLevels[wBusOut].GetGain();
		iLeft += iBusOutGain;
		iBusOutGain = m_pEG->m_BusOutLineLevels[wBusOut+1].GetGain();
		iRight += iBusOutGain;
		
		//
		// Muting and clamping
		//		
		if (m_Mutes[wIndex])
		{
			iLeft = ECHOGAIN_MUTED;
			iRight = ECHOGAIN_MUTED;
		}
		else
		{
			if (  (m_pEG->m_BusOutLineLevels[wBusOut].IsMuteOn()) ||
					(iLeft < ECHOGAIN_MUTED))
			{
				iLeft = ECHOGAIN_MUTED;
			}
			else if (iLeft > ECHOGAIN_MAXOUT)
			{
				iLeft = ECHOGAIN_MAXOUT;				
			}

			if (  (m_pEG->m_BusOutLineLevels[wBusOut + 1].IsMuteOn()) ||
					(iRight < ECHOGAIN_MUTED))
			{
				iRight = ECHOGAIN_MUTED;
			}
			else if (iRight > ECHOGAIN_MAXOUT)
			{
				iRight = ECHOGAIN_MAXOUT;				
			}
		}
		
		//
		// Set the left channel gain
		//
		Status = m_pEG->GetDspCommObject()->SetPipeOutGain(wPipeOut,
																			wBusOut,
																			iLeft,
																			FALSE);
		if (ECHOSTATUS_OK == Status)
		{
			//
			// And the right channel
			//
			Status = m_pEG->GetDspCommObject()->SetPipeOutGain(wPipeOut,
																				wBusOut + 1,
																				iRight,
																				fImmediate);
		}
		
	}
	else
	{
		//
		// Add this output pipe gain to the output bus gain
		// Since these gains are in decibels, it's OK to just add them
		//
		iBusOutGain = m_pEG->m_BusOutLineLevels[wBusOut].GetGain();
		iGain += iBusOutGain;

		//
		// Mute this output pipe if this output bus is muted
		//	
		if (m_Mutes[ wIndex ] ||
			 (m_pEG->m_BusOutLineLevels[wBusOut].IsMuteOn()) )
		{
			iGain = ECHOGAIN_MUTED;
		}
		else
		{
			//
			// Clamp the output pipe gain if necessary
			//
			if (iGain < ECHOGAIN_MUTED)
				iGain = ECHOGAIN_MUTED;
			else if (iGain > ECHOGAIN_MAXOUT)
				iGain = ECHOGAIN_MAXOUT;
			
		}

		//
		// Set the gain
		//
		Status = m_pEG->GetDspCommObject()->SetPipeOutGain(wPipeOut,
																			wBusOut,
																			iGain,
																			fImmediate);

	}
	
	return Status;
}	


ECHOSTATUS CPipeOutCtrl::GetGain(WORD wPipeOut, WORD wBusOut, INT32 &iGain)
{
	WORD	wIndex = GetIndex(wPipeOut,wBusOut);
	
	if (NULL == m_Gains)
		return ECHOSTATUS_NO_MEM;

	if ((wPipeOut >= m_wNumPipesOut) || (wBusOut >= m_wNumBussesOut))
	{
		ECHO_DEBUGPRINTF(("CPipeOutCtrl::GetGain - out of range pipe %d bus %d\n",
								wPipeOut,wBusOut));
		return ECHOSTATUS_INVALID_PARAM;		
	}

	iGain = DSP_TO_GENERIC( m_Gains[wIndex] );

	/*
	ECHO_DEBUGPRINTF(("CPipeOutCtrl::GetGain pipe %d  bus %d  gain 0x%lx  index %d\n",
							wPipeOut,wBusOut,iGain,wIndex));
	*/
					
	return ECHOSTATUS_OK;
}	


//*****************************************************************************
//
// Set and get mute
//
//*****************************************************************************

ECHOSTATUS CPipeOutCtrl::SetMute
(
	WORD wPipeOut, 
	WORD wBusOut, 
	BOOL bMute,
	BOOL fImmediate
)
{
	if (!m_fHasVmixer && (wPipeOut != wBusOut))
		return ECHOSTATUS_OK;

	if (NULL == m_Mutes)
		return ECHOSTATUS_NO_MEM;

	if ((wPipeOut >= m_wNumPipesOut) || (wBusOut >= m_wNumBussesOut))
	{
		ECHO_DEBUGPRINTF(("CPipeOutCtrl::SetMute - out of range pipe %d bus %d\n",
								wPipeOut,wBusOut));
		return ECHOSTATUS_INVALID_PARAM;		
	}

	WORD wIndex = GetIndex(wPipeOut,wBusOut);
	
	/*
	ECHO_DEBUGPRINTF(("CPipeOutCtrl::SetMute wPipeOut %d  wBusOut %d  bMute %ld\n",
							wPipeOut,wBusOut,bMute));
	*/
	
	//
	// Store the mute
	//
 	m_Mutes[ wIndex ] = (BYTE) bMute;

	//
	// Store the notify
	//
	m_pEG->MixerControlChanged(ECHO_PIPE_OUT,MXN_MUTE,wPipeOut,wBusOut);

	//
	// Call the SetGain function to do all the heavy lifting
	// Use the ECHOGAIN_UPDATE value to tell the function to 
	// recalculate the gain setting using the currently stored value.
	//
	return SetGain(wPipeOut,wBusOut,ECHOGAIN_UPDATE,fImmediate);
}	


ECHOSTATUS CPipeOutCtrl::GetMute(WORD wPipeOut, WORD wBusOut, BOOL &bMute)
{
	WORD	wIndex = GetIndex(wPipeOut,wBusOut);
	
	if (NULL == m_Mutes)
		return ECHOSTATUS_NO_MEM;

	if ((wPipeOut >= m_wNumPipesOut) || (wBusOut >= m_wNumBussesOut))
	{
		ECHO_DEBUGPRINTF(("CPipeOutCtrl::GetMute - out of range pipe %d bus %d\n",
								wPipeOut,wBusOut));
		return ECHOSTATUS_INVALID_PARAM;		
	}

	bMute = (BOOL) m_Mutes[ wIndex ];

	/*
	ECHO_DEBUGPRINTF(("CPipeOutCtrl::GetMute wPipeOut %d  wBusOut %d  bMute %ld\n",
							wPipeOut,wBusOut,bMute));
	*/

	return ECHOSTATUS_OK;
}	


//*****************************************************************************
//
// Set and get pan (vmixer only)
//
//*****************************************************************************

ECHOSTATUS CPipeOutCtrl::SetPan(WORD wPipeOut, WORD wBusOut, INT32 iPan)
{
	if (!m_fHasVmixer)
		return ECHOSTATUS_OK;
		
	if (NULL == m_Pans)
		return ECHOSTATUS_NO_MEM;
	
	if ((wPipeOut >= m_wNumPipesOut) || (wBusOut >= m_wNumBussesOut))
	{
		ECHO_DEBUGPRINTF(("CPipeOutCtrl::SetPan - out of range pipe %d bus %d\n",
								wPipeOut,wBusOut));
		return ECHOSTATUS_INVALID_PARAM;		
	}
		

	WORD	wIndex = GetIndex(wPipeOut,wBusOut);		

	//
	// Clamp it and stash it
	//		
	if (iPan < 0)
		iPan = 0;
	else if (iPan > MAX_MIXER_PAN)
		iPan = MAX_MIXER_PAN;
		
	m_Pans[wIndex] = (WORD) iPan;

	//
	// Store the notify
	//
	m_pEG->MixerControlChanged(ECHO_PIPE_OUT,MXN_PAN,wPipeOut,wBusOut);

	//
	//	Convert this pan setting into left and right dB values
	// 		
	m_PanDbs[wIndex].iLeft = (INT8) GENERIC_TO_DSP( PanToDb(MAX_MIXER_PAN - iPan) );
	m_PanDbs[wIndex].iRight = (INT8) GENERIC_TO_DSP( PanToDb(iPan) );

	//
	// Again, SetGain does all the hard work
	//	
	return SetGain(wPipeOut,wBusOut,ECHOGAIN_UPDATE);
}	


ECHOSTATUS CPipeOutCtrl::GetPan(WORD wPipeOut, WORD wBusOut, INT32 &iPan)
{
	WORD	wIndex = GetIndex(wPipeOut,wBusOut);
	
	if (NULL == m_Pans)
		return ECHOSTATUS_NO_MEM;

	if ((wPipeOut >= m_wNumPipesOut) || (wBusOut >= m_wNumBussesOut))
	{
		ECHO_DEBUGPRINTF(("CPipeOutCtrl::GetPan - out of range pipe %d bus %d\n",
								wPipeOut,wBusOut));
		return ECHOSTATUS_INVALID_PARAM;		
	}

	iPan = m_Pans[ wIndex ];

	return ECHOSTATUS_OK;
}	
