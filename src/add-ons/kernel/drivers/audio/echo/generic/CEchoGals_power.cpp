// ****************************************************************************
//
//		CEchoGals_power.cpp
//
//		Power management functions for the CEchoGals driver class.
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


//===========================================================================
//
// Tell the hardware to go into a low-power state - the converters are 
// shut off and the DSP powers down.
//
//===========================================================================

ECHOSTATUS CEchoGals::GoComatose()
{
	//
	// Pass the call through to the DSP comm object
	//
	return GetDspCommObject()->GoComatose();

} // GoComatose


//===========================================================================
//
// Tell the hardware to wake up - go back to the full-power state
//
// You can call WakeUp() if you just want to be sure
// that the firmware is loaded OK
//
//===========================================================================

ECHOSTATUS CEchoGals::WakeUp()
{
	//
	// Load the firmware
	//
	ECHOSTATUS Status;
	CDspCommObject *pDCO = GetDspCommObject();
	
	Status = pDCO->LoadFirmware();

// fixme need to not do this if the DSP was already awake
	if (ECHOSTATUS_OK == Status)
	{
		//
		// For any pipes that have daffy ducks, 
		// reset the pointer to the first PLE in the comm page
		//
		WORD wPipeIndex;
		
		for (wPipeIndex = 0; wPipeIndex < GetNumPipes(); wPipeIndex++)
		{
			if (NULL != m_DaffyDucks[wPipeIndex])
			{
				DWORD dwPhysStartAddr;
				
				dwPhysStartAddr = m_DaffyDucks[wPipeIndex]->GetPhysStartAddr();
				pDCO->SetAudioDuckListPhys(wPipeIndex,dwPhysStartAddr);
													
			}
		}
	}

	return Status;
	

} // WakeUp


