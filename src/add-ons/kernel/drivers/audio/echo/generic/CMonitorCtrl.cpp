// ****************************************************************************
//
//		CMonitorCtrl.cpp
//
//		Class to control monitors
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
#include "CMonitorCtrl.h"

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
				m_PanDbs[wIndex].iRight = GENERIC_TO_DSP( ECHOGAIN_MUTED );
			}
			else
			{
				m_Pans[wIndex] = MAX_MIXER_PAN;
				m_PanDbs[wIndex].iLeft = GENERIC_TO_DSP( ECHOGAIN_MUTED );
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
	INT32 	iGain,
	BOOL 	fImmediate
)
{
	ECHOSTATUS Status;

	if (NULL == m_pEG)
		return ECHOSTATUS_DSP_DEAD;
	
	if (	(NULL == m_Gains) ||
			(NULL == m_PanDbs) )
		return ECHOSTATUS_NO_MEM;

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
	
		m_Gains[ wIndex ] = GENERIC_TO_DSP( iGain );
		
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
	//	Convert this pan setting INTo left and right dB values
	// 		
	m_PanDbs[wIndex].iLeft = GENERIC_TO_DSP( PanToDb(MAX_MIXER_PAN - iPan) );
	m_PanDbs[wIndex].iRight = GENERIC_TO_DSP( PanToDb(iPan) );

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
	WORD	wIndex = GetIndex(wBusIn,wBusOut);
	
	if (NULL == m_Pans)
		return ECHOSTATUS_NO_MEM;

	iPan = m_Pans[ wIndex ];

	return ECHOSTATUS_OK;
}	
