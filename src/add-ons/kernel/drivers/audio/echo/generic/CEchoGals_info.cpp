// ****************************************************************************
//
//		CEchoGals_info.cpp
//
//		Implementation file for the CEchoGals driver class (info functions).
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


/****************************************************************************

	CEchoGals informational methods

 ****************************************************************************/

//===========================================================================
//
// GetBaseCapabilities is used by the CEchoGals-derived classes to
// help fill out the ECHOGALS_CAPS struct.
//
//===========================================================================

ECHOSTATUS CEchoGals::GetBaseCapabilities
(
	PECHOGALS_CAPS	pCapabilities
)
{
	CChannelMask	DigMask, ChMask, AdatMask;
	WORD				i;

	//
	//	Test only for no DSP object.  We can still get capabilities even if
	//	board is bad.
	//
	if ( NULL == GetDspCommObject() )
		return ECHOSTATUS_DSP_DEAD;

	memset( pCapabilities, 0, sizeof(ECHOGALS_CAPS) );
	strcpy( pCapabilities->szName, m_szCardInstallName );
	
	pCapabilities->wCardType 		= GetDspCommObject()->GetCardType();
	pCapabilities->wNumPipesOut 	= GetNumPipesOut();
	pCapabilities->wNumPipesIn 	= GetNumPipesIn();
	pCapabilities->wNumBussesOut 	= GetNumBussesOut();
	pCapabilities->wNumBussesIn	= GetNumBussesIn();
	
	pCapabilities->wFirstDigitalBusOut 	= GetFirstDigitalBusOut();
	pCapabilities->wFirstDigitalBusIn 	= GetFirstDigitalBusIn();
	
	pCapabilities->wNumMidiOut = GetDspCommObject()->GetNumMidiOutChannels();
	pCapabilities->wNumMidiIn = GetDspCommObject()->GetNumMidiInChannels();

	pCapabilities->dwOutClockTypes = 0;
	pCapabilities->dwInClockTypes = ECHO_CLOCK_BIT_INTERNAL;;

	//
	// Add the controls to the output pipes that are 
	// the same for all cards	
	// 
	for (i = 0; i < GetNumPipesOut(); i++)
	{
		pCapabilities->dwPipeOutCaps[i] = 	ECHOCAPS_GAIN | 
														ECHOCAPS_MUTE;
	}
	
	//
	// Add controls to the input busses that are the same for all cards
	//
	WORD wFirstDigitalBusIn = GetFirstDigitalBusIn();
	for (i = 0; i < GetNumBussesIn(); i++)
	{
		pCapabilities->dwBusInCaps[i] =	ECHOCAPS_PEAK_METER |
													ECHOCAPS_VU_METER;
		if (i >= wFirstDigitalBusIn)
			pCapabilities->dwBusInCaps[i] |= ECHOCAPS_DIGITAL;
	}
	
	//
	// And the output busses
	//
	WORD	wFirstDigitalBusOut = GetFirstDigitalBusOut();
	for (i = 0; i < GetNumBussesOut(); i++)
	{
		pCapabilities->dwBusOutCaps[i] = ECHOCAPS_PEAK_METER |
													ECHOCAPS_VU_METER	|
													ECHOCAPS_GAIN |
													ECHOCAPS_MUTE;
		if (i >= wFirstDigitalBusOut)
			pCapabilities->dwBusOutCaps[i] |= ECHOCAPS_DIGITAL;
	}
	
	//
	// Digital modes & vmixer flag
	//
	pCapabilities->dwDigitalModes = GetDspCommObject()->GetDigitalModes();
	pCapabilities->fHasVmixer = GetDspCommObject()->HasVmixer();
	
	//
	//	If this is not a vmixer card, output pipes are hard-wired to output busses.
	// Mark those pipes that are hard-wired to digital busses as digital pipes
	//
	if (GetDspCommObject()->HasVmixer())
	{
		pCapabilities->wFirstDigitalPipeOut = pCapabilities->wNumPipesOut;
	}
	else
	{
		pCapabilities->wFirstDigitalPipeOut = pCapabilities->wFirstDigitalBusOut;

		
		for (	i = pCapabilities->wFirstDigitalPipeOut;
			  	i < GetNumPipesOut(); 
			  	i++)
		{
			pCapabilities->dwPipeOutCaps[i] |= ECHOCAPS_DIGITAL;
		}

	}
	
	//
	// Input pipes are the same, vmixer or no vmixer
	//
	pCapabilities->wFirstDigitalPipeIn = pCapabilities->wFirstDigitalBusIn;

	for (	i = pCapabilities->wFirstDigitalPipeIn;
		  	i < GetNumPipesIn(); 
		  	i++)
	{
		pCapabilities->dwPipeInCaps[i] |= ECHOCAPS_DIGITAL;
	}
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CEchoGals::GetBaseCapabilities


//===========================================================================
//
// MakePipeIndex is a utility function; it takes a pipe number and an
// input or output flag and returns the pipe index.  Refer to 
// EchoGalsXface.h for more information.
//
//===========================================================================

WORD CEchoGals::MakePipeIndex(WORD wPipe,BOOL fInput)
{
	if (fInput)
		return wPipe + GetNumPipesOut();
	
	return wPipe;	
	
}	// MakePipeIndex


//===========================================================================
//
// Get the card name as an ASCII zero-terminated string
//
//===========================================================================

CONST PCHAR CEchoGals::GetDeviceName()
{ 
	return m_szCardInstallName;
}


//===========================================================================
//
// Get numbers of pipes and busses
//
//===========================================================================

WORD CEchoGals::GetNumPipesOut()
{
	if (NULL == GetDspCommObject())
		return 0;
		
	return GetDspCommObject()->GetNumPipesOut();
}

WORD CEchoGals::GetNumPipesIn()
{
	if (NULL == GetDspCommObject())
		return 0;
		
	return GetDspCommObject()->GetNumPipesIn();
}

WORD CEchoGals::GetNumBussesOut()
{
	if (NULL == GetDspCommObject())
		return 0;
		
	return GetDspCommObject()->GetNumBussesOut();
}

WORD CEchoGals::GetNumBussesIn()
{
	if (NULL == GetDspCommObject())
		return 0;
		
	return GetDspCommObject()->GetNumBussesIn();
}

WORD CEchoGals::GetNumBusses()
{
	if (NULL == GetDspCommObject())
		return 0;
		
	return GetDspCommObject()->GetNumBusses();
}

WORD CEchoGals::GetNumPipes()
{
	if (NULL == GetDspCommObject())
		return 0;
		
	return GetDspCommObject()->GetNumPipes();
}

WORD CEchoGals::GetFirstDigitalBusOut()
{
	if (NULL == GetDspCommObject())
		return 0;
		
	return GetDspCommObject()->GetFirstDigitalBusOut();
}

WORD CEchoGals::GetFirstDigitalBusIn()
{
	if (NULL == GetDspCommObject())
		return 0;
		
	return GetDspCommObject()->GetFirstDigitalBusIn();
}

BOOL CEchoGals::HasVmixer()
{
	if (NULL == GetDspCommObject())
		return 0;
		
	return GetDspCommObject()->HasVmixer();
}


//===========================================================================
//
//	Get access to the DSP comm object
//
//===========================================================================

PCDspCommObject CEchoGals::GetDspCommObject()
{
	return m_pDspCommObject;	
}

