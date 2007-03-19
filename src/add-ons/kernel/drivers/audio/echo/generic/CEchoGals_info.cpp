// ****************************************************************************
//
//		CEchoGals_info.cpp
//
//		Implementation file for the CEchoGals driver class (info functions).
//		Set editor tabs to 3 for your viewing pleasure.
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
	pCapabilities->dwInClockTypes = ECHO_CLOCK_BIT_INTERNAL;

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

