// ****************************************************************************
//
//		CEchoGals_WDM.cpp
//
//		Implementation file for the CEchoGals driver class.
//		Set editor tabs to 3 for your viewing pleasure.
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

#pragma optimize("",off)

//
// Make new daffy ducks for all channels
//
ECHOSTATUS CEchoGals::MakeDaffyDuck(WORD wPipeIndex)
{
	ECHOSTATUS 	Status = ECHOSTATUS_OK;
	
	//---------------------------------------------------------
	//
	// Allocate the main daffy duck for this channel
	//
	//---------------------------------------------------------

	m_pDaffyDucks[wPipeIndex] = new CDaffyDuck( 	m_pOsSupport,
																GetDspCommObject(),
																wPipeIndex );
	if (NULL == m_pDaffyDucks[wPipeIndex])
	{
		return ECHOSTATUS_NO_MEM;
	}
		
	//
	// Check the daffy duck				 
	//
	Status = m_pDaffyDucks[wPipeIndex]->InitCheck();
	if (ECHOSTATUS_OK != Status)
	{
		return Status;
	}

	//
	// Put the daffy duck in the comm page
	//
	GetDspCommObject()->
		SetAudioDuckListPhys(	wPipeIndex,
					  			m_pDaffyDucks[wPipeIndex]->GetPhysStartAddr() );
		
	return Status;
	
}	// MakeDaffyDuck


//
// Kill the daffy duck for a pipe
//
void CEchoGals::KillDaffyDuck(WORD wPipeIndex)	
{
	if (NULL != m_pDaffyDucks[wPipeIndex])
	{
		delete m_pDaffyDucks[wPipeIndex];
		m_pDaffyDucks[wPipeIndex] = NULL;
	}

}	// KillDaffyDuck


//
// Get the daffy duck for a given pipe index
//
CDaffyDuck *CEchoGals::GetDaffyDuck(WORD wPipeIndex)
{
	if (wPipeIndex >= GetNumPipes())
		return NULL;

	return m_pDaffyDucks[wPipeIndex];
}	


//
// Update the 64 bit DMA counter for this channel, allowing
// for DSP position counter wraparound.  
//
void CEchoGals::UpdateDmaPos( WORD wPipeIndex )
{
	DWORD dwDspPos;
	DWORD dwDelta;
	
	dwDspPos = GetDspCommObject()->GetAudioPosition( wPipeIndex );
	dwDelta = dwDspPos - m_dwLastDspPos[ wPipeIndex ];
	
	m_ullDmaPos[ wPipeIndex ] += dwDelta;
	m_dwLastDspPos[ wPipeIndex ] = dwDspPos;

} // UpdateDmaPos


void CEchoGals::ResetDmaPos(WORD wCh)
{
	m_ullDmaPos[ wCh ] = 0;
	m_dwLastDspPos[ wCh ] = 0;

	//
	// There may still be mappings in the daffy duck; if so,
	// tell them to reset their DMA positions starting at zero
	//
	if (NULL != m_pDaffyDucks[wCh])
		m_pDaffyDucks[wCh]->ResetStartPos();
}



#ifdef SIMPHONICS_EXTENSIONS

/*-----------------------------------------------------------------------------

 CEchoGals::SetMixerUberGain

 Set an array of gains to simultaneously set all monitor, virtual output,
 and output bus gains, as per the SimPhonics specifications.

 The structure of the array is as follows:


					Monitor gains

	0		I0->O0	I1->O0	...	I15->O0
	16		I0->O1	I1->O1	...	I15->O1
	...
	240 	I0->O15	I1->O15	...	I15->O15
	
	
					Virtual output gains

	256	V0->O0	V1->O0	...	V15->O0
	272	V0->O1	V1->O1	...	V15->O1
	...
	498	V0->O15	V1->O15	...	V15->O15
	

					Master output bus gains
		
	512	B0			B1			...	B15
	
	
	Values are signed bytes, ranging from +6 to -128 dB.
	
	Right now, this is hard-coded for Layla24v - 16 inputs,
	16 output busses, 16 virtual outputs.

-----------------------------------------------------------------------------*/

#define LAYLA24V_CHANNELS	16

ECHOSTATUS CEchoGals::SetLayla24vUberGain(LAYLA24V_UBER_GAIN *pLug)
{
	if (GetDspCommObject())
		GetDspCommObject()->SetLayla24vUberGain(pLug);
					
	return ECHOSTATUS_OK;
	
}	// SetLayla24vMixerUberGain

#endif // SIMPHONICS_EXTENSIONS