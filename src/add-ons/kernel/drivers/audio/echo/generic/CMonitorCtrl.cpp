// ****************************************************************************
//
//		CMonitorCtrl.cpp
//
//		Class to control monitors
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
#include "CMonitorCtrl.h"

//*****************************************************************************
//
// Destructor (this class uses the default constructor)
//
//*****************************************************************************

CMonitorCtrl::~CMonitorCtrl()
{
	Cleanup();	
}	


//*****************************************************************************
//
// Init
//
//*****************************************************************************

ECHOSTATUS CMonitorCtrl::Init(CEchoGals *pEG)
{
	DWORD	dwBytes;
	DWORD	dwArraySize;

	m_Gains = NULL;
	m_Mutes = NULL;
	m_Pans = NULL;
	m_PanDbs = NULL;
	
	//
	// Cache stuff
	//
	m_pEG = pEG;
	m_wNumBussesIn = pEG->GetNumBussesIn();
	m_wNumBussesOut = pEG->GetNumBussesOut();

	//
	// Indigo has no inputs; attempting to allocate 0 bytes 
	// causes a BSOD on Windows ME.
	//
	if ((0 == m_wNumBussesIn) || (0 == m_wNumBussesOut))
	{
		ECHO_DEBUGPRINTF(("CMonitorCtrl::Init - this card has no inputs!\n"));
		return ECHOSTATUS_OK;
	}
	
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//
	// Allocate the arrays
	//
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		
	dwArraySize = m_wNumBussesIn * (m_wNumBussesOut >> 1);

	dwBytes = sizeof(INT8) * dwArraySize;
	OsAllocateNonPaged(dwBytes,(void **) &m_Gains);
	if (NULL == m_Gains)
	{
		Cleanup();
		return ECHOSTATUS_NO_MEM;
	}
	
	dwBytes = sizeof(WORD) * dwArraySize;
	OsAllocateNonPaged(dwBytes,(void **) &m_Pans);
	if (NULL == m_Pans)
	{
		Cleanup();
		return ECHOSTATUS_NO_MEM;
	}
	
	dwBytes = sizeof(BYTE) * dwArraySize;
	OsAllocateNonPaged(dwBytes,(void **) &m_Mutes);
	if (NULL == m_Mutes)
	{
		Cleanup();
		return ECHOSTATUS_NO_MEM;
	}
	
	dwBytes = sizeof(PAN_DB) * dwArraySize;
	OsAllocateNonPaged(dwBytes,(void **) &m_PanDbs );
	if (NULL == m_PanDbs)
	{
		Cleanup();
		return ECHOSTATUS_NO_MEM;
	}

	//==============================================================
	//		
	// Init the arrays
	//
	//==============================================================
	
	WORD wBusIn,wBusOut,wIndex;
	
	for (wBusIn = 0; wBusIn < m_wNumBussesIn; wBusIn++)
		for (wBusOut = 0; wBusOut < m_wNumBussesOut; wBusOut += 2)
		{
			wIndex = GetIndex(wBusIn,wBusOut);
			
			//
			// Pan hard left for even inputs, hard right for odd
			//
			if (0 == (wBusIn & 1))
			{
				m_Pans[wIndex] = 0;
				m_PanDbs[wIndex].iLeft = 0;
				m_PanDbs[wIndex].iRight = (INT8) GENERIC_TO_DSP( ECHOGAIN_MUTED );
			}
			else
			{
				m_Pans[wIndex] = MAX_MIXER_PAN;
				m_PanDbs[wIndex].iLeft = (INT8) GENERIC_TO_DSP( ECHOGAIN_MUTED );
				m_PanDbs[wIndex].iRight = 0;
			}
			
			//
			// Mute unless this is not a digital input
			// and the input is going to the same-numbered output
			//
			if ( (wBusIn  < m_pEG->GetFirstDigitalBusIn()) &&
				  ( (wBusIn & 0xfffe) == wBusOut ) )
			{
				m_Mutes[wIndex] = FALSE;
			}
			else
			{
				m_Mutes[wIndex] = TRUE;
			}
			
			//
			// Put stuff in the comm page
			//
			SetGain(wBusIn,wBusOut,ECHOGAIN_UPDATE,FALSE);
		}
		
	//
	// Now actually update the DSP
	//
	m_pEG->GetDspCommObject()->UpdateAudioOutLineLevel();

	
	return ECHOSTATUS_OK;
	
}	// Init


//*****************************************************************************
//
// Cleanup - free allocated memory
//
//*****************************************************************************

void CMonitorCtrl::Cleanup()
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
//*****************************************************************************

ECHOSTATUS CMonitorCtrl::SetGain
(
	WORD 	wBusIn, 
	WORD 	wBusOut, 
	INT32 iGain,
	BOOL 	fImmediate
)
{
	ECHOSTATUS Status;

	if (NULL == m_pEG)
		return ECHOSTATUS_DSP_DEAD;
	
	if (	(NULL == m_Gains) ||
			(NULL == m_PanDbs) )
		return ECHOSTATUS_NO_MEM;
	
	if (	(wBusIn >= m_wNumBussesIn) || (wBusOut >= m_wNumBussesOut) )
	{
		ECHO_DEBUGPRINTF(("CMonitorCtrl::SetGain - out of range   in %d out %d\n",
								wBusIn,wBusOut));
		return ECHOSTATUS_INVALID_PARAM;		
	}
	
	//
	// Round down to the nearest even bus
	//
	wBusOut &= 0xfffe;

	//
	// Figure out the index into the array
	//
	WORD wIndex = GetIndex(wBusIn,wBusOut);
	
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
		// Gain has changed; store the notify
		//
		m_pEG->MixerControlChanged(ECHO_MONITOR,MXN_LEVEL,wBusIn,wBusOut);
	}
		
	//
	// Use the gain that was passed in, the pan setting,
	// and the mute to calculate the left and right gains
	//		
	INT32 iLeft = iGain + DSP_TO_GENERIC( m_PanDbs[wIndex].iLeft );
	INT32 iRight = iGain + DSP_TO_GENERIC( m_PanDbs[wIndex].iRight );
	
	//
	// Adjust left and right by the output bus gain
	//
	iLeft += m_pEG->m_BusOutLineLevels[wBusOut].GetGain();
	iRight += m_pEG->m_BusOutLineLevels[wBusOut + 1].GetGain();

	//
	// Either mute or clamp
	//
	if (TRUE == m_Mutes[wIndex])
	{
		iLeft = ECHOGAIN_MUTED;
		iRight = ECHOGAIN_MUTED;
	}
	else
	{
		if ( m_pEG->m_BusOutLineLevels[wBusOut].IsMuteOn() )
		{
			iLeft = ECHOGAIN_MUTED;
		}
		else
		{
			//
			// Clamp left
			//
			if (iLeft > ECHOGAIN_MAXOUT)
				iLeft = ECHOGAIN_MAXOUT;
			else if (iLeft < ECHOGAIN_MUTED)
				iLeft = ECHOGAIN_MUTED;
		}
			
		if ( m_pEG->m_BusOutLineLevels[wBusOut + 1].IsMuteOn() )
		{
			iRight = ECHOGAIN_MUTED;
		}
		else
		{
			//
			// Clamp right
			//
			if (iRight > ECHOGAIN_MAXOUT)
				iRight = ECHOGAIN_MAXOUT;
			else if (iRight < ECHOGAIN_MUTED)
				iRight = ECHOGAIN_MUTED;
			
		}
	}


	//
	// Set the left channel
	//	
	if ( (NULL == m_pEG) ||
			(NULL == m_pEG->GetDspCommObject() ) )
		return ECHOSTATUS_DSP_DEAD;
	
		
	Status = m_pEG->
					GetDspCommObject()->
						SetAudioMonitor( 	wBusOut,
												wBusIn,
												iLeft,
												FALSE);

	//
	//	Set the right channel
	// 
	if (ECHOSTATUS_OK == Status)
	{
		Status = m_pEG->
						GetDspCommObject()->
							SetAudioMonitor( 	wBusOut + 1,
													wBusIn,
													iRight,
													fImmediate);
	}

	return Status;
}	


ECHOSTATUS CMonitorCtrl::GetGain(WORD wBusIn, WORD wBusOut, INT32 &iGain)
{
	WORD	wIndex = GetIndex(wBusIn,wBusOut);
	
	if (NULL == m_Gains)
		return ECHOSTATUS_NO_MEM;
	
	if (	(wBusIn >= m_wNumBussesIn) || (wBusOut >= m_wNumBussesOut) )
	{
		ECHO_DEBUGPRINTF(("CMonitorCtrl::GetGain - out of range in %d out %d\n",
								wBusIn,wBusOut));
		return ECHOSTATUS_INVALID_PARAM;		
	}

	iGain = DSP_TO_GENERIC( m_Gains[wIndex] );

	return ECHOSTATUS_OK;
}	


//*****************************************************************************
//
// Set and get mute
//
//*****************************************************************************

ECHOSTATUS CMonitorCtrl::SetMute
(
	WORD wBusIn, 
	WORD wBusOut, 
	BOOL bMute,
	BOOL fImmediate
)
{
	if (NULL == m_Mutes)
		return ECHOSTATUS_NO_MEM;

	if (	(wBusIn >= m_wNumBussesIn) || (wBusOut >= m_wNumBussesOut) )
	{
		ECHO_DEBUGPRINTF(("CMonitorCtrl::SetMute - out of range   in %d out %d\n",
								wBusIn,wBusOut));
		return ECHOSTATUS_INVALID_PARAM;		
	}
	
	wBusOut &= 0xfffe;	

	WORD wIndex = GetIndex(wBusIn,wBusOut);
	
	//
	// Store the mute
	//
 	m_Mutes[ wIndex ] = (BYTE) bMute;

	//
	// Store the notify
	//
	m_pEG->MixerControlChanged(ECHO_MONITOR,MXN_MUTE,wBusIn,wBusOut);
	

	//
	// Call the SetGain function to do all the heavy lifting
	// Use the ECHOGAIN_UPDATE value to tell the function to 
	// recalculate the gain setting using the currently stored value.
	//
	return SetGain(wBusIn,wBusOut,ECHOGAIN_UPDATE,fImmediate);
}	


ECHOSTATUS CMonitorCtrl::GetMute(WORD wBusIn, WORD wBusOut, BOOL &bMute)
{
	wBusOut &= 0xfffe;

	if (	(wBusIn >= m_wNumBussesIn) || (wBusOut >= m_wNumBussesOut) )
	{
		ECHO_DEBUGPRINTF(("CMonitorCtrl::GetMute - out of range   in %d out %d\n",
								wBusIn,wBusOut));
		return ECHOSTATUS_INVALID_PARAM;		
	}


	WORD	wIndex = GetIndex(wBusIn,wBusOut);
	
	if (NULL == m_Mutes)
		return ECHOSTATUS_NO_MEM;

	bMute = (BOOL) m_Mutes[ wIndex ];

	return ECHOSTATUS_OK;
}	


//*****************************************************************************
//
// Set and get pan
//
//*****************************************************************************

ECHOSTATUS CMonitorCtrl::SetPan(WORD wBusIn, WORD wBusOut, INT32 iPan)
{
	if (	(wBusIn >= m_wNumBussesIn) || (wBusOut >= m_wNumBussesOut) )
	{
		ECHO_DEBUGPRINTF(("CMonitorCtrl::SetPan - out of range   in %d out %d\n",
								wBusIn,wBusOut));
		return ECHOSTATUS_INVALID_PARAM;		
	}

	wBusOut &= 0xfffe;

	WORD	wIndex = GetIndex(wBusIn,wBusOut);		

	if (NULL == m_Pans)
		return ECHOSTATUS_NO_MEM;

	//
	// Clamp it and stash it
	//		
	if (iPan < 0)
		iPan = 0;
	else if (iPan > MAX_MIXER_PAN)
		iPan = MAX_MIXER_PAN;
		
	m_Pans[wIndex] = (WORD) iPan;

	//
	//	Convert this pan setting into left and right dB values
	// 		
	m_PanDbs[wIndex].iLeft = (INT8) GENERIC_TO_DSP( PanToDb(MAX_MIXER_PAN - iPan) );
	m_PanDbs[wIndex].iRight = (INT8) GENERIC_TO_DSP( PanToDb(iPan) );

	//
	// Store the notify
	//
	m_pEG->MixerControlChanged(ECHO_MONITOR,MXN_PAN,wBusIn,wBusOut);

	//
	// Once again SetGain does all the hard work
	//
	return SetGain(wBusIn,wBusOut,ECHOGAIN_UPDATE);
}	


ECHOSTATUS CMonitorCtrl::GetPan(WORD wBusIn, WORD wBusOut, INT32 &iPan)
{
	if (	(wBusIn >= m_wNumBussesIn) || (wBusOut >= m_wNumBussesOut) )
	{
		ECHO_DEBUGPRINTF(("CMonitorCtrl::GetPan - out of range   in %d out %d\n",
								wBusIn,wBusOut));
		return ECHOSTATUS_INVALID_PARAM;		
	}


	wBusOut &= 0xfffe;

	WORD	wIndex = GetIndex(wBusIn,wBusOut);
	
	if (NULL == m_Pans)
		return ECHOSTATUS_NO_MEM;

	iPan = m_Pans[ wIndex ];

	return ECHOSTATUS_OK;
}	
