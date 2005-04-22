// ****************************************************************************
//
//  	CDarlaDspCommObject.cpp
//
//		Implementation file for Darla20 DSP interface class.
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
#include "CDarlaDspCommObject.h"

#include "Darla20DSP.c"


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Constructor
//
//===========================================================================

CDarlaDspCommObject::CDarlaDspCommObject
(
	PDWORD		pdwRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
) : CGdDspCommObject( pdwRegBase, pOsSupport )
{
	strcpy( m_szCardName, "Darla" );
	m_pdwDspRegBase = pdwRegBase;		// Virtual addr DSP's register base
	
	m_wNumPipesOut = 8;
	m_wNumPipesIn = 2;
	m_wNumBussesOut = 8;
	m_wNumBussesIn = 2;
	m_wFirstDigitalBusOut = 8;
	m_wFirstDigitalBusIn = 2;
	
	m_fHasVmixer = FALSE;

	m_wNumMidiOut = 0;					// # MIDI out channels
	m_wNumMidiIn = 0;						// # MIDI in  channels
	
	m_pwDspCodeToLoad = pwDarla20DSP;
	
	//
	// Since this card has no ASIC, mark it as loaded so everything works OK
	//
	m_bASICLoaded = TRUE;

}	// CDarlaDspCommObject::CDarlaDspCommObject( DWORD dwPhysRegBase )


//===========================================================================
//
// Destructor
//
//===========================================================================

CDarlaDspCommObject::~CDarlaDspCommObject()
{
}	// CDarlaDspCommObject::~CDarlaDspCommObject()

// **** DarlaDspCommObject.cpp ****
