// ****************************************************************************
//
//		CEchoGals.cpp
//
//		Implementation file for the CEchoGals driver class.
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

	CEchoGals construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CEchoGals::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CEchoGals::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;

}	// PVOID CEchoGals::operator new( size_t Size )


VOID  CEchoGals::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF(("CEchoGals::operator delete memory free failed\n"));
	}
}	// VOID  CEchoGals::operator delete( PVOID pVoid )


//===========================================================================
//
// Constructor
//
//===========================================================================

CEchoGals::CEchoGals
(
	PCOsSupport pOsSupport
)
{
	ECHO_ASSERT(pOsSupport );

	m_pOsSupport = pOsSupport;		// Ptr to OS Support methods & data

	m_wFlags = ECHOGALS_FLAG_BADBOARD;
				  
	m_dwLockedSampleRate	= 44100;

}	// CEchoGals::CEchoGals()


//===========================================================================
//
// Destructor
//
//===========================================================================

CEchoGals::~CEchoGals()
{
	//
	// Free stuff
	//
	delete m_pDspCommObject;	// This powers down the DSP 

	//
	// Clean up the ducks
	//
	WORD i;
	
	for (i = 0; i < ECHO_MAXAUDIOPIPES; i++)
	{
		if (NULL != m_DaffyDucks[i])
			delete m_DaffyDucks[i];
	}
	
	//
	//	Clean up the mixer client list
	//
	while (NULL != m_pMixerClients)
	{
		ECHO_MIXER_CLIENT *pDeadClient;

		pDeadClient = m_pMixerClients;
		m_pMixerClients = pDeadClient->pNext;	
		OsFreeNonPaged(pDeadClient);
	}
	
	ECHO_DEBUGPRINTF( ( "CEchoGals::~CEchoGals() is toast!\n" ) );
	
}	// CEchoGals::~CEchoGals()




/****************************************************************************

	CEchoGals setup and hardware initialization

 ****************************************************************************/
 
//===========================================================================
//
// AssignResources does just what you'd expect.  
//
// Note that pvSharedMemory is a logical pointer - that is, the driver
// will dereference this pointer to access the memory-mapped regisers on
// the DSP.  The caller needs to take the physical address from the PCI
// config space and map it.
//
//===========================================================================

ECHOSTATUS CEchoGals::AssignResources
(
	PVOID		pvSharedMemory,		// Ptr to DSP registers
	PCHAR		pszCardName				// Caller gets from registry
)
{
	//
	//	Use ECHO_ASSERT(to be sure this isn't called twice!
	//
	ECHO_ASSERT(NULL == m_pvSharedMemory );

	//
	//	Check and store the parameters
	//
	ECHO_ASSERT(pvSharedMemory );
	if ( NULL == pszCardName ) 
	{
		return( ECHOSTATUS_BAD_CARD_NAME );
	}
	m_pvSharedMemory = pvSharedMemory;	// Shared memory addr assigned by PNP

	//
	//	Copy card name & make sure we don't overflow our buffer
	//
	strncpy( m_szCardInstallName, pszCardName, ECHO_MAXNAMELEN-1 );
	m_szCardInstallName[ ECHO_MAXNAMELEN-1 ] = 0;
	
	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS CEchoGals::AssignResources


//===========================================================================
//
// InitHw is device-specific and so does nothing here; it is overridden
// in the derived classes.
//
//	The correct sequence is: 
//
//	Construct the appropriate CEchoGals-derived object for the card
// Call AssignResources
// Call InitHw
//
//===========================================================================

ECHOSTATUS CEchoGals::InitHw()
{
	//
	//	Use ECHO_ASSERT to be sure AssignResources was called!
	//
	ECHO_ASSERT(m_pvSharedMemory );

	return ECHOSTATUS_OK;
	
} // ECHOSTATUS CEchoGals::InitHw()


//===========================================================================
//
// This method initializes classes to control the mixer controls for
// the busses and pipes.  This is a protected method; it is called 
// from each of the CEchoGals derived classes once the DSP is up
// and running.
//
//===========================================================================

ECHOSTATUS CEchoGals::InitLineLevels()
{
	ECHOSTATUS	Status = ECHOSTATUS_OK;
	WORD			i;
	
	m_fMixerDisabled = TRUE;

	if ( 	(NULL == GetDspCommObject()) || 
			(GetDspCommObject()->IsBoardBad() ) )
	{
		return ECHOSTATUS_DSP_DEAD;
	}
	
	//
	// Do output busses first since output pipes & monitors 
	// depend on output bus values
	//
	for ( i = 0; i < GetNumBussesOut(); i++ )
	{
		m_BusOutLineLevels[ i ].Init( i, this );
	}
	
	Status = m_PipeOutCtrl.Init(this);
	if (ECHOSTATUS_OK != Status)
		return Status;

	Status = m_MonitorCtrl.Init(this);
	if (ECHOSTATUS_OK != Status)
		return Status;		

	for ( i = 0; i < GetNumBussesIn(); i++ )
	{
		m_BusInLineLevels[ i ].Init(	i,	this );
	}
	
	m_fMixerDisabled = FALSE;

	return Status;

}	// ECHOSTATUS CEchoGals::InitLineLevels()




/******************************************************************************

 CEchoGals interrupt handler functions

 ******************************************************************************/

//===========================================================================
//
// This isn't the interrupt handler itself; rather, the OS-specific layer
// of the driver has an interrupt handler that calls this function.
//
//===========================================================================

ECHOSTATUS CEchoGals::ServiceIrq(BOOL &fMidiReceived)
{
	CDspCommObject *pDCO;
	
	//
	// Read the DSP status register and see if this DSP 
	// generated this interrupt
	//
	fMidiReceived = FALSE;
	
	pDCO = GetDspCommObject();
	if ( pDCO->GetStatusReg() & CHI32_STATUS_IRQ )
	{

#ifdef MIDI_SUPPORT		

		//
		// If this was a MIDI input interrupt, get the MIDI input data
		//
		DWORD dwMidiInCount;

		pDCO->ReadMidi( 0, dwMidiInCount );	  // The count is at index 0
		if ( 0 != dwMidiInCount )
		{
			m_MidiIn.ServiceIrq();
			fMidiReceived = TRUE;
		}

#endif // MIDI_SUPPORT		
		
		//
		//	Clear the hardware interrupt
		//
		pDCO->AckInt();
		
		return ECHOSTATUS_OK;
	}

	//
	// This interrupt line must be shared
	//
	/*
	ECHO_DEBUGPRINTF( ("CEchoGals::ServiceIrq() %s\tInterrupt not ours!\n",
							 GetDeviceName()) );
	*/
	
	return ECHOSTATUS_IRQ_NOT_OURS;

}	// ECHOSTATUS CEchoGals::ServiceIrq()




/****************************************************************************

	Character strings for the ECHOSTATUS return values - 
	useful for debugging.

 ****************************************************************************/

//
// pStatusStrs is used if you want to print out a friendlier version of
// the various ECHOSTATUS codes.
//
char *	pStatusStrs[ECHOSTATUS_LAST] =
{
	"ECHOSTATUS_OK",
	"ECHOSTATUS_BAD_FORMAT",
	"ECHOSTATUS_BAD_BUFFER_SIZE",
	"ECHOSTATUS_CANT_OPEN",
	"ECHOSTATUS_CANT_CLOSE",
	"ECHOSTATUS_CHANNEL_NOT_OPEN",
	"ECHOSTATUS_BUSY",
	"ECHOSTATUS_BAD_LEVEL",
	"ECHOSTATUS_NO_MIDI",
	"ECHOSTATUS_CLOCK_NOT_SUPPORTED",
	"ECHOSTATUS_CLOCK_NOT_AVAILABLE",
	"ECHOSTATUS_BAD_CARDID",
	"ECHOSTATUS_NOT_SUPPORTED",
	"ECHOSTATUS_BAD_NOTIFY_SIZE",
	"ECHOSTATUS_INVALID_PARAM",
	"ECHOSTATUS_NO_MEM",
	"ECHOSTATUS_NOT_SHAREABLE",
	"ECHOSTATUS_FIRMWARE_LOADED",
	"ECHOSTATUS_DSP_DEAD",
	"ECHOSTATUS_DSP_TIMEOUT",
	"ECHOSTATUS_INVALID_CHANNEL",
	"ECHOSTATUS_CHANNEL_ALREADY_OPEN",
	"ECHOSTATUS_DUCK_FULL",
	"ECHOSTATUS_INVALID_INDEX",
	"ECHOSTATUS_BAD_CARD_NAME",
	"ECHOSTATUS_IRQ_NOT_OURS",
	"",
	"",
	"",
	"",
	"",
	"ECHOSTATUS_BUFFER_OVERFLOW",
	"ECHOSTATUS_OPERATION_CANCELED",
	"ECHOSTATUS_EVENT_NOT_OPEN",
	"ECHOSTATUS_ASIC_NOT_LOADED"
	"ECHOSTATUS_DIGITAL_MODE_NOT_SUPPORTED",
	"ECHOSTATUS_RESERVED",
	"ECHOSTATUS_BAD_COOKIE",
	"ECHOSTATUS_MIXER_DISABLED",
	"ECHOSTATUS_NO_SUPER_INTERLEAVE",
	"ECHOSTATUS_DUCK_NOT_WRAPPED"
};



// *** CEchoGals.cpp ***

 
 
 
 
 