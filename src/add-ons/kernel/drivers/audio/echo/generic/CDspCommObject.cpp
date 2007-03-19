// ****************************************************************************
//
//  	CDspCommObject.cpp
//
//		Implementation file for EchoGals generic driver DSP interface class.
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

#ifdef DSP_56361
#include "LoaderDSP.c"
#endif

#define COMM_PAGE_PHYS_BYTES	((sizeof(DspCommPage)+PAGE_SIZE-1)/PAGE_SIZE)*PAGE_SIZE


/****************************************************************************

	Construction and destruction

 ****************************************************************************/

//===========================================================================
//
// Overload new & delete so memory for this object is allocated
//	from non-paged memory.
//
//===========================================================================

PVOID CDspCommObject::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CDspCommObject::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;
	
}	// PVOID CDspCommObject::operator new( size_t Size )


VOID  CDspCommObject::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF( ("CDspCommObject::operator delete memory free "
								 "failed\n") );
	}
}	// VOID  CDspCommObject::operator delete( PVOID pVoid )


//===========================================================================
//
// Constructor
//
//===========================================================================

CDspCommObject::CDspCommObject
(
	PDWORD		pdwDspRegBase,				// Virtual ptr to DSP registers
	PCOsSupport	pOsSupport
)
{
	INT32	i;

	ECHO_ASSERT(pOsSupport );
	
	//
	// Init all the basic stuff
	//
	strcpy( m_szCardName, "??????" );
	m_pOsSupport = pOsSupport;				// Ptr to OS Support methods & data
	m_pdwDspRegBase = pdwDspRegBase;		// Virtual addr DSP's register base
	m_bBadBoard = TRUE;						// Set TRUE until DSP loaded
	m_pwDspCode = NULL;						// Current DSP code not loaded
	m_byDigitalMode = DIGITAL_MODE_NONE;
	m_wInputClock = ECHO_CLOCK_INTERNAL;
	m_wOutputClock = ECHO_CLOCK_WORD;
	m_ullLastLoadAttemptTime = (ULONGLONG)(DWORD)(0L - DSP_LOAD_ATTEMPT_PERIOD);	// force first load to go

#ifdef MIDI_SUPPORT	
	m_ullNextMidiWriteTime = 0;
#endif

	//
	// Create the DSP comm page - this is the area of memory read and written by
	// the DSP via bus mastering
	//
	ECHOSTATUS Status;
	DWORD dwSegmentSize;
	PHYS_ADDR PhysAddr;
	
	Status = pOsSupport->AllocPhysPageBlock(	COMM_PAGE_PHYS_BYTES,
															m_pDspCommPageBlock);
	if (ECHOSTATUS_OK != Status)
	{
		ECHO_DEBUGPRINTF( ("CDspCommObject::CDspCommObject DSP comm page "
								 "memory allocation failed\n") );
		return;
	}
	
	m_pDspCommPage = (PDspCommPage) pOsSupport->
													GetPageBlockVirtAddress( m_pDspCommPageBlock );
	
	pOsSupport->GetPageBlockPhysSegment(m_pDspCommPageBlock,
													0,
													PhysAddr,
													dwSegmentSize);
	m_dwCommPagePhys = PhysAddr;
	
	//
	// Init the comm page
	//
	m_pDspCommPage->dwCommSize = SWAP( sizeof( DspCommPage ) );
													// Size of DSP comm page
	
	m_pDspCommPage->dwHandshake = 0xffffffff;
	m_pDspCommPage->dwMidiOutFreeCount = SWAP( (DWORD) DSP_MIDI_OUT_FIFO_SIZE );

	for ( i = 0; i < DSP_MAXAUDIOINPUTS; i++ )
		m_pDspCommPage->InLineLevel[ i ] = 0x00;
													// Set line levels so we don't blast
													// any inputs on startup
	memset( m_pDspCommPage->byMonitors,
			  GENERIC_TO_DSP(ECHOGAIN_MUTED),
			  MONITOR_ARRAY_SIZE );			// Mute all monitors

	memset( m_pDspCommPage->byVmixerLevel,
			  GENERIC_TO_DSP(ECHOGAIN_MUTED),
			  VMIXER_ARRAY_SIZE );			// Mute all virtual mixer levels
			  
#ifdef DIGITAL_INPUT_AUTO_MUTE_SUPPORT

	m_fDigitalInAutoMute = TRUE;

#endif

}	// CDspCommObject::CDspCommObject


//===========================================================================
//
// Destructor
//
//===========================================================================

CDspCommObject::~CDspCommObject()
{
	//
	// Go to sleep
	//
	GoComatose();

	//
	// Free the comm page
	//
	if ( NULL != m_pDspCommPageBlock )
	{
		m_pOsSupport->FreePhysPageBlock( COMM_PAGE_PHYS_BYTES,
													m_pDspCommPageBlock);
	}

	ECHO_DEBUGPRINTF( ( "CDspCommObject::~CDspCommObject() is toast!\n" ) );

}	// CDspCommObject::~CDspCommObject()




/****************************************************************************

	Firmware loading functions

 ****************************************************************************/

//===========================================================================
//
// ASIC status check - some cards have one or two ASICs that need to be 
// loaded.  Once that load is complete, this function is called to see if
// the load was successful. 
//
// If this load fails, it does not necessarily mean that the hardware is
// defective - the external box may be disconnected or turned off.
//
//===========================================================================

BOOL CDspCommObject::CheckAsicStatus()
{
	DWORD	dwAsicStatus;
	DWORD	dwReturn;

	//
	// Always succeed if this card doesn't have an ASIC
	//
	if ( !m_bHasASIC )
	{
		m_bASICLoaded = TRUE;
		return TRUE;
	}
			
	// Send the vector command
	m_bASICLoaded = FALSE;
	SendVector( DSP_VC_TEST_ASIC );	

	// The DSP will return a value to indicate whether or not the 
	// ASIC is currently loaded
	dwReturn = Read_DSP( &dwAsicStatus );
	if ( ECHOSTATUS_OK != dwReturn )
	{
		ECHO_DEBUGPRINTF(("CDspCommObject::CheckAsicStatus - failed on Read_DSP\n"));
		ECHO_DEBUGBREAK();
		return FALSE;
	}

#ifdef ECHO_DEBUG
	if ( (dwAsicStatus != ASIC_LOADED) && (dwAsicStatus != ASIC_NOT_LOADED) )
	{
		ECHO_DEBUGBREAK(); 
	}
#endif
	
	if ( dwAsicStatus == ASIC_LOADED )
		m_bASICLoaded = TRUE;

	return m_bASICLoaded;

}	// BOOL CDspCommObject::CheckAsicStatus()


//===========================================================================
//
//	Load ASIC code - done after the DSP is loaded
//
//===========================================================================

BOOL CDspCommObject::LoadASIC
(
	DWORD	dwCmd,
	PBYTE	pCode,
	DWORD	dwSize
)
{
	DWORD i;
#ifdef _WIN32
	DWORD dwChecksum = 0;
#endif

	ECHO_DEBUGPRINTF(("CDspCommObject::LoadASIC\n"));

	if ( !m_bHasASIC )
		return TRUE;

#ifdef _DEBUG
	ULONGLONG	ullStartTime, ullCurTime;
	m_pOsSupport->OsGetSystemTime( &ullStartTime );
#endif

	// Send the "Here comes the ASIC" command
	if ( ECHOSTATUS_OK != Write_DSP( dwCmd ) )
		return FALSE;

	// Write length of ASIC file in bytes
	if ( ECHOSTATUS_OK != Write_DSP( dwSize ) )
		return FALSE;

	for ( i = 0; i < dwSize; i++ )
	{
#ifdef _WIN32
		dwChecksum += pCode[i];
#endif	
	
		if ( ECHOSTATUS_OK != Write_DSP( pCode[ i ] ) )
		{
			ECHO_DEBUGPRINTF(("\tfailed on Write_DSP\n"));
			return FALSE;
		}
	}

#ifdef _DEBUG
	m_pOsSupport->OsGetSystemTime( &ullCurTime );
	ECHO_DEBUGPRINTF( ("CDspCommObject::LoadASIC took %ld usec.\n",
							(ULONG) ( ullCurTime - ullStartTime ) ) );
	ECHO_DEBUGPRINTF(("ASIC load OK\n"));
#endif

#if defined(_WIN32) && (DBG)
	DbgPrint("--- ASIC checksum is 0x%lx\n",dwChecksum);
#endif

	return TRUE;

}	// BOOL CDspCommObject::LoadASIC( DWORD dwCmd, PBYTE pCode, DWORD dwSize )


//===========================================================================
//
// InstallResidentLoader
//
// Install the resident loader for 56361 DSPs;  The resident loader
// is on the EPROM on the board for 56301 DSP.
//
// The resident loader is a tiny little program that is used to load
// the real DSP code.
//
//===========================================================================

#ifdef DSP_56361

ECHOSTATUS CDspCommObject::InstallResidentLoader()
{
	DWORD			dwAddress;
	DWORD			dwIndex;
	INT32			iNum;
	INT32			i;
	DWORD			dwReturn;
	PWORD			pCode;

	ECHO_DEBUGPRINTF( ("CDspCommObject::InstallResidentLoader\n") );
	
	//
	// 56361 cards only!
	//
	if (DEVICE_ID_56361 != m_pOsSupport->GetDeviceId() )
		return ECHOSTATUS_OK;

	//
	// Look to see if the resident loader is present.  If the resident loader
	// is already installed, host flag 5 will be on.
	//
	DWORD dwStatus;
	dwStatus = GetDspRegister( CHI32_STATUS_REG );
	if ( 0 != (dwStatus & CHI32_STATUS_REG_HF5 ) )
	{
		ECHO_DEBUGPRINTF(("\tResident loader already installed; status is 0x%lx\n",
								dwStatus));
		return ECHOSTATUS_OK;
	}
	//
	// Set DSP format bits for 24 bit mode
	//
	SetDspRegister( CHI32_CONTROL_REG,
						 GetDspRegister( CHI32_CONTROL_REG ) | 0x900 );

	//---------------------------------------------------------------------------
	//
	// Loader
	//
	// The DSP code is an array of 16 bit words.  The array is divided up into
	// sections.  The first word of each section is the size in words, followed
	// by the section type.
	//
	// Since DSP addresses and data are 24 bits wide, they each take up two
	// 16 bit words in the array.
	//
	// This is a lot like the other loader loop, but it's not a loop,
	// you don't write the memory type, and you don't write a zero at the end.
	//
	//---------------------------------------------------------------------------

	pCode = pwLoaderDSP;
	//
	// Skip the header section; the first word in the array is the size of 
	//	the first section, so the first real section of code is pointed to 
	//	by pCode[0].
	//
	dwIndex = pCode[ 0 ];
	//
	// Skip the section size, LRS block type, and DSP memory type
	//
	dwIndex += 3;
	//	
	// Get the number of DSP words to write
	//
	iNum = pCode[ dwIndex++ ];
	//
	// Get the DSP address for this block; 24 bits, so build from two words
	//
	dwAddress = ( pCode[ dwIndex ] << 16 ) + pCode[ dwIndex + 1 ];
	dwIndex += 2;
	//	
	// Write the count to the DSP
	//
	dwReturn = Write_DSP( (DWORD) iNum );
	if ( dwReturn != 0 )
	{
		ECHO_DEBUGPRINTF( ("CDspCommObject::InstallResidentLoader: Failed to "
								 "write word count!\n") );
		return ECHOSTATUS_DSP_DEAD;
	}

	// Write the DSP address
	dwReturn = Write_DSP( dwAddress );
	if ( dwReturn != 0 )
	{
		ECHO_DEBUGPRINTF( ("CDspCommObject::InstallResidentLoader: Failed to "
								 "write DSP address!\n") );
		return ECHOSTATUS_DSP_DEAD;
	}


	// Write out this block of code to the DSP
	for ( i = 0; i < iNum; i++) // 
	{
		DWORD	dwData;

		dwData = ( pCode[ dwIndex ] << 16 ) + pCode[ dwIndex + 1 ];
		dwReturn = Write_DSP( dwData );
		if ( dwReturn != 0 )
		{
			ECHO_DEBUGPRINTF( ("CDspCommObject::InstallResidentLoader: Failed to "
									 "write DSP code\n") );
			return ECHOSTATUS_DSP_DEAD;
		}

		dwIndex+=2;
	}
	
	//
	// Wait for flag 5 to come up
	//
	BOOL			fSuccess;
	ULONGLONG 	ullCurTime,ullTimeout;

	m_pOsSupport->OsGetSystemTime( &ullCurTime );
	ullTimeout = ullCurTime + 10000L;		// 10m.s.
	fSuccess = FALSE;
	do
	{
		m_pOsSupport->OsSnooze(50);	// Give the DSP some time;
														// no need to hog the CPU
		
		dwStatus = GetDspRegister( CHI32_STATUS_REG );
		if (0 != (dwStatus & CHI32_STATUS_REG_HF5))
		{
			fSuccess = TRUE;
			break;
		}
		
		m_pOsSupport->OsGetSystemTime( &ullCurTime );

	} while (ullCurTime < ullTimeout);
	
	if (FALSE == fSuccess)
	{
		ECHO_DEBUGPRINTF(("\tResident loader failed to set HF5\n"));
		return ECHOSTATUS_DSP_DEAD;
	}
		
	ECHO_DEBUGPRINTF(("\tResident loader successfully installed\n"));

	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS CDspCommObject::InstallResidentLoader()

#endif // DSP_56361


//===========================================================================
//
// LoadDSP
//
// This loads the DSP code.
//
//===========================================================================

ECHOSTATUS CDspCommObject::LoadDSP
(
	PWORD	pCode					// Ptr to DSP object code
)
{
	DWORD			dwAddress;
	DWORD			dwIndex;
	INT32			iNum;
	INT32			i;
	DWORD			dwReturn;
	ULONGLONG	ullTimeout, ullCurTime;
	ECHOSTATUS	Status;

	ECHO_DEBUGPRINTF(("CDspCommObject::LoadDSP\n"));
	if ( m_pwDspCode == pCode )
	{
		ECHO_DEBUGPRINTF( ("\tDSP is already loaded!\n") );
		return ECHOSTATUS_FIRMWARE_LOADED;
	}
	m_bBadBoard = TRUE;		// Set TRUE until DSP loaded
	m_pwDspCode = NULL;		// Current DSP code not loaded
	m_bASICLoaded = FALSE;	// Loading the DSP code will reset the ASIC
	
	ECHO_DEBUGPRINTF(("CDspCommObject::LoadDSP  Set m_bBadBoard to TRUE\n"));
	
	//
	//	If this board requires a resident loader, install it.
	//
#ifdef DSP_56361
	InstallResidentLoader();
#endif

	// Send software reset command	
	if ( ECHOSTATUS_OK != SendVector( DSP_VC_RESET ) )
	{
		m_pOsSupport->EchoErrorMsg(
			"CDspCommObject::LoadDsp SendVector DSP_VC_RESET failed",
			"Critical Failure" );
		return ECHOSTATUS_DSP_DEAD;
	}

	// Delay 10us
	m_pOsSupport->OsSnooze( 10L );

	// Wait 10ms for HF3 to indicate that software reset is complete	
	m_pOsSupport->OsGetSystemTime( &ullCurTime );
	ullTimeout = ullCurTime + 10000L;		// 10m.s.

	// wait for HF3 to be set
wait_for_hf3:
	
	if ( GetDspRegister( CHI32_STATUS_REG ) & CHI32_STATUS_REG_HF3 )
			goto set_dsp_format_bits;
		m_pOsSupport->OsGetSystemTime( &ullCurTime );
		if ( ullCurTime > ullTimeout)
		{
			ECHO_DEBUGPRINTF( ("CDspCommObject::LoadDSP Timeout waiting for "
									 "CHI32_STATUS_REG_HF3\n") );
			m_pOsSupport->EchoErrorMsg(
				"CDspCommObject::LoadDSP SendVector DSP_VC_RESET failed",
				"Critical Failure" );
			return ECHOSTATUS_DSP_TIMEOUT;
		}
	goto wait_for_hf3;


	// Set DSP format bits for 24 bit mode now that soft reset is done
set_dsp_format_bits:
		SetDspRegister( CHI32_CONTROL_REG,
						 GetDspRegister( CHI32_CONTROL_REG ) | (DWORD) 0x900 );

	//---------------------------------------------------------------------------
	// Main loader loop
	//---------------------------------------------------------------------------

	dwIndex = pCode[ 0 ];

	for (;;)
	{
		INT32	iBlockType;
		INT32	iMemType;

		// Total Block Size
		dwIndex++;
		
		// Block Type
		iBlockType = pCode[ dwIndex ];
		if ( iBlockType == 4 )  // We're finished
			break;

		dwIndex++;

		// Memory Type  P=0,X=1,Y=2
		iMemType = pCode[ dwIndex ]; 
		dwIndex++;
		
		// Block Code Size
		iNum = pCode[ dwIndex ];
		dwIndex++;
		if ( iNum == 0 )			// We're finished
			break;
	
 		// Start Address
		dwAddress = ( (DWORD) pCode[ dwIndex ] << 16 ) + pCode[ dwIndex + 1 ];
//		ECHO_DEBUGPRINTF( ("\tdwAddress %lX\n", dwAddress) );
		dwIndex += 2;
		
		dwReturn = Write_DSP( (DWORD)iNum );
		if ( dwReturn != 0 )
		{
			ECHO_DEBUGPRINTF(("LoadDSP - failed to write number of DSP words\n"));
			return ECHOSTATUS_DSP_DEAD;
		}

		dwReturn = Write_DSP( dwAddress );
		if ( dwReturn != 0 )
		{
			ECHO_DEBUGPRINTF(("LoadDSP - failed to write DSP address\n"));
			return ECHOSTATUS_DSP_DEAD;
		}

		dwReturn = Write_DSP( (DWORD)iMemType );
		if ( dwReturn != 0 )
		{
			ECHO_DEBUGPRINTF(("LoadDSP - failed to write DSP memory type\n"));
			return ECHOSTATUS_DSP_DEAD;
		}

		// Code
		for ( i = 0; i < iNum; i++ )
		{
			DWORD	dwData;

			dwData = ( (DWORD) pCode[ dwIndex ] << 16 ) + pCode[ dwIndex + 1 ];
			dwReturn = Write_DSP( dwData );
			if ( dwReturn != 0 )
			{
				ECHO_DEBUGPRINTF(("LoadDSP - failed to write DSP data\n"));
				return ECHOSTATUS_DSP_DEAD;
			}
	
			dwIndex += 2;
		}
//		ECHO_DEBUGPRINTF( ("\tEnd Code Block\n") );
	}
	dwReturn = Write_DSP( 0 );					// We're done!!!
	if ( dwReturn != 0 )
	{
		ECHO_DEBUGPRINTF(("LoadDSP: Failed to write final zero\n"));
		return ECHOSTATUS_DSP_DEAD;
	}
		

	// Delay 10us
	m_pOsSupport->OsSnooze( 10L );

	m_pOsSupport->OsGetSystemTime( &ullCurTime );
	ullTimeout  = ullCurTime + 500000L;		// 1/2 sec. timeout

	while ( ullCurTime <= ullTimeout) 
	{
		//
		// Wait for flag 4 - indicates that the DSP loaded OK
		//
		if ( GetDspRegister( CHI32_STATUS_REG ) & CHI32_STATUS_REG_HF4 )
		{
			SetDspRegister( CHI32_CONTROL_REG,
								 GetDspRegister( CHI32_CONTROL_REG ) & ~0x1b00 );

			dwReturn = Write_DSP( DSP_FNC_SET_COMMPAGE_ADDR );
			if ( dwReturn != 0 )
			{
				ECHO_DEBUGPRINTF(("LoadDSP - Failed to write DSP_FNC_SET_COMMPAGE_ADDR\n"));
				return ECHOSTATUS_DSP_DEAD;
			}
				
			dwReturn = Write_DSP( m_dwCommPagePhys );
			if ( dwReturn != 0 )
			{
				ECHO_DEBUGPRINTF(("LoadDSP - Failed to write comm page address\n"));
				return ECHOSTATUS_DSP_DEAD;
			}

			//
			// Get the serial number via slave mode.
			// This is triggered by the SET_COMMPAGE_ADDR command.
			//	We don't actually use the serial number but we have to get
			//	it as part of the DSP init vodoo.
			//
			Status = ReadSn();
			if ( ECHOSTATUS_OK != Status )
			{
				ECHO_DEBUGPRINTF(("LoadDSP - Failed to read serial number\n"));
				return Status;
			}

			m_pwDspCode = pCode;			// Show which DSP code loaded
			m_bBadBoard = FALSE;			// DSP OK
			
			ECHO_DEBUGPRINTF(("CDspCommObject::LoadDSP  Set m_bBadBoard to FALSE\n"));			
		
			return ECHOSTATUS_OK;
		}
		
		m_pOsSupport->OsGetSystemTime( &ullCurTime );
	}
	
	ECHO_DEBUGPRINTF( ("LoadDSP: DSP load timed out waiting for HF4\n") );	
	
	return ECHOSTATUS_DSP_TIMEOUT;

}	// DWORD	CDspCommObject::LoadDSP




//===========================================================================
//
// LoadFirmware takes care of loading the DSP and any ASIC code.
//
//===========================================================================

ECHOSTATUS CDspCommObject::LoadFirmware()
{
	ECHOSTATUS	dwReturn;
	ULONGLONG	ullRightNow;

	// Sanity check
	if ( NULL == m_pwDspCodeToLoad || NULL == m_pDspCommPage )
	{
		ECHO_DEBUGBREAK();
		return ECHOSTATUS_NO_MEM;
	}
	
	//
	// Even if the external box is off, an application may still try
	// to repeatedly open the driver, causing multiple load attempts and 
	// making the machine spend lots of time in the kernel.  If the ASIC is not
	// loaded, this code will gate the loading attempts so it doesn't happen
	// more than once per second.
	//
	m_pOsSupport->OsGetSystemTime(&ullRightNow);
	if ( 	(FALSE == m_bASICLoaded) &&
			(DSP_LOAD_ATTEMPT_PERIOD > (ullRightNow - m_ullLastLoadAttemptTime)) )
		return ECHOSTATUS_ASIC_NOT_LOADED;
	
	//
	// Update the timestamp
	//
	m_ullLastLoadAttemptTime = ullRightNow;

	//
	// See if the ASIC is present and working - only if the DSP is already loaded
	//	
	if (NULL != m_pwDspCode)
	{
		dwReturn = CheckAsicStatus();
		if (TRUE == dwReturn)
			return ECHOSTATUS_OK;
		
		//
		// ASIC check failed; force the DSP to reload
		//	
		m_pwDspCode = NULL;
	}

	//
	// Try and load the DSP
	//
	dwReturn = LoadDSP( m_pwDspCodeToLoad );
	if ( 	(ECHOSTATUS_OK != dwReturn) && 
			(ECHOSTATUS_FIRMWARE_LOADED != dwReturn) )
	{
		return dwReturn;
	}
	
	ECHO_DEBUGPRINTF(("DSP load OK\n"));

	//
	// Load the ASIC if the DSP load succeeded; LoadASIC will
	// always return TRUE for cards that don't have an ASIC.
	//
	dwReturn = LoadASIC();
	if ( FALSE == dwReturn )
	{
		dwReturn = ECHOSTATUS_ASIC_NOT_LOADED;
	}
	else
	{
		//
		// ASIC load was successful
		//
		RestoreDspSettings();

		dwReturn = ECHOSTATUS_OK;
	}
	
	return dwReturn;
	
}	// BOOL CDspCommObject::LoadFirmware()


//===========================================================================
//
// This function is used to read back the serial number from the DSP;
// this is triggered by the SET_COMMPAGE_ADDR command.
//
// Only some early Echogals products have serial numbers in the ROM;
// the serial number is not used, but you still need to do this as
// part of the DSP load process.
//
//===========================================================================

ECHOSTATUS CDspCommObject::ReadSn()
{
	INT32			j;
	DWORD			dwSn[ 6 ];
	ECHOSTATUS	Status;

	ECHO_DEBUGPRINTF( ("CDspCommObject::ReadSn\n") );
	for ( j = 0; j < 5; j++ )
	{
		Status = Read_DSP( &dwSn[ j ] );
		if ( Status != 0 )
		{
			ECHO_DEBUGPRINTF( ("\tFailed to read serial number word %ld\n",
									 j) );
			return ECHOSTATUS_DSP_DEAD;
		}
	}
	ECHO_DEBUGPRINTF( ("\tRead serial number %08lx %08lx %08lx %08lx %08lx\n",
							 dwSn[0], dwSn[1], dwSn[2], dwSn[3], dwSn[4]) );
	return ECHOSTATUS_OK;
	
}	// DWORD	CDspCommObject::ReadSn




//===========================================================================
//
//	This is called after LoadFirmware to restore old gains, meters on, 
// monitors, etc.
//
//===========================================================================

void CDspCommObject::RestoreDspSettings()
{
	ECHO_DEBUGPRINTF(("RestoreDspSettings\n"));
	ECHO_DEBUGPRINTF(("\tControl reg is 0x%lx\n",SWAP(m_pDspCommPage->dwControlReg) ));

	if ( !CheckAsicStatus() )
		return;

	m_pDspCommPage->dwHandshake = 0xffffffff;		
	
#ifdef MIDI_SUPPORT
	m_ullNextMidiWriteTime = 0;
#endif
	
	SetSampleRate();
	if ( 0 != m_wMeterOnCount )
	{
		SendVector( DSP_VC_METERS_ON );
	}

	SetInputClock( m_wInputClock );
	SetOutputClock( m_wOutputClock );
		
	if ( !WaitForHandshake() )
	{
		return;
	}
	UpdateAudioOutLineLevel();

	if ( !WaitForHandshake() )
		return;
	UpdateAudioInLineLevel();

	if ( HasVmixer() )
	{
		if ( !WaitForHandshake() )
			return;
		UpdateVmixerLevel();
	}
	
	if ( !WaitForHandshake() )
		return;

	ClearHandshake();
	SendVector( DSP_VC_UPDATE_FLAGS );
	
	ECHO_DEBUGPRINTF(("RestoreDspSettings done\n"));	
	
}	// void CDspCommObject::RestoreDspSettings()




/****************************************************************************

	DSP utilities

 ****************************************************************************/

//===========================================================================
//
// Write_DSP writes a 32-bit value to the DSP; this is used almost 
// exclusively for loading the DSP.
//
//===========================================================================

ECHOSTATUS CDspCommObject::Write_DSP
(
	DWORD dwData				// 32 bit value to write to DSP data register
)
{
	DWORD 		dwStatus;
	ULONGLONG 	ullCurTime, ullTimeout;

//	ECHO_DEBUGPRINTF(("Write_DSP\n"));
	
	m_pOsSupport->OsGetSystemTime( &ullCurTime );
	ullTimeout = ullCurTime + 10000000L;		// 10 sec.
	while ( ullTimeout >= ullCurTime ) 
	{
		dwStatus = GetDspRegister( CHI32_STATUS_REG );
		if ( ( dwStatus & CHI32_STATUS_HOST_WRITE_EMPTY ) != 0 )
		{
			SetDspRegister( CHI32_DATA_REG, dwData );
//			ECHO_DEBUGPRINTF(("Write DSP: 0x%x", dwData));
			return ECHOSTATUS_OK;
		}
		m_pOsSupport->OsGetSystemTime( &ullCurTime );
	}

	m_bBadBoard = TRUE;		// Set TRUE until DSP re-loaded
	
	ECHO_DEBUGPRINTF(("CDspCommObject::Write_DSP  Set m_bBadBoard to TRUE\n"));
		
	return ECHOSTATUS_DSP_TIMEOUT;
	
}	// ECHOSTATUS CDspCommObject::Write_DSP




//===========================================================================
//
// Read_DSP reads a 32-bit value from the DSP; this is used almost 
// exclusively for loading the DSP and checking the status of the ASIC.
//
//===========================================================================

ECHOSTATUS CDspCommObject::Read_DSP
(
	DWORD *pdwData				// Ptr to 32 bit value read from DSP data register
)
{
	DWORD 		dwStatus;
	ULONGLONG	ullCurTime, ullTimeout;

//	ECHO_DEBUGPRINTF(("Read_DSP\n"));
	m_pOsSupport->OsGetSystemTime( &ullCurTime );

	ullTimeout = ullCurTime + READ_DSP_TIMEOUT;	
	while ( ullTimeout >= ullCurTime )
	{
		dwStatus = GetDspRegister( CHI32_STATUS_REG );
		if ( ( dwStatus & CHI32_STATUS_HOST_READ_FULL ) != 0 )
		{
			*pdwData = GetDspRegister( CHI32_DATA_REG );
//			ECHO_DEBUGPRINTF(("Read DSP: 0x%x\n", *pdwData));
			return ECHOSTATUS_OK;
		}
		m_pOsSupport->OsGetSystemTime( &ullCurTime );
	}

	m_bBadBoard = TRUE;		// Set TRUE until DSP re-loaded
	
	ECHO_DEBUGPRINTF(("CDspCommObject::Read_DSP  Set m_bBadBoard to TRUE\n"));	
	
	return ECHOSTATUS_DSP_TIMEOUT;
}	// ECHOSTATUS CDspCommObject::Read_DSP



//===========================================================================
//
// Much of the interaction between the DSP and the driver is done via vector
// commands; SendVector writes a vector command to the DSP.  Typically,
// this causes the DSP to read or write fields in the comm page.
//
// Returns ECHOSTATUS_OK if sent OK.
//
//===========================================================================

ECHOSTATUS CDspCommObject::SendVector
(
	DWORD dwCommand				// 32 bit command to send to DSP vector register
)
{
	ULONGLONG	ullTimeout;
	ULONGLONG	ullCurTime;

//
// Turn this on if you want to see debug prints for every vector command
//
#if 0
//#ifdef ECHO_DEBUG
	char *	pszCmd;
	switch ( dwCommand )
	{
		case DSP_VC_ACK_INT :
			pszCmd = "DSP_VC_ACK_INT";
			break;
		case DSP_VC_SET_VMIXER_GAIN :
			pszCmd = "DSP_VC_SET_VMIXER_GAIN";
			break;
		case DSP_VC_START_TRANSFER :
			pszCmd = "DSP_VC_START_TRANSFER";
			break;
		case DSP_VC_METERS_ON :
			pszCmd = "DSP_VC_METERS_ON";
			break;
		case DSP_VC_METERS_OFF :
			pszCmd = "DSP_VC_METERS_OFF";
			break;
		case DSP_VC_UPDATE_OUTVOL :
			pszCmd = "DSP_VC_UPDATE_OUTVOL";
			break;
		case DSP_VC_UPDATE_INGAIN :
			pszCmd = "DSP_VC_UPDATE_INGAIN";
			break;
		case DSP_VC_ADD_AUDIO_BUFFER :
			pszCmd = "DSP_VC_ADD_AUDIO_BUFFER";
			break;
		case DSP_VC_TEST_ASIC :
			pszCmd = "DSP_VC_TEST_ASIC";
			break;
		case DSP_VC_UPDATE_CLOCKS :
			pszCmd = "DSP_VC_UPDATE_CLOCKS";
			break;
		case DSP_VC_SET_LAYLA_SAMPLE_RATE :
			if ( GetCardType() == LAYLA )
				pszCmd = "DSP_VC_SET_LAYLA_RATE";
			else if ( GetCardType() == GINA || GetCardType() == DARLA )
				pszCmd = "DSP_VC_SET_GD_AUDIO_STATE";
			else
				pszCmd = "DSP_VC_WRITE_CONTROL_REG";
			break;
		case DSP_VC_MIDI_WRITE :
			pszCmd = "DSP_VC_MIDI_WRITE";
			break;
		case DSP_VC_STOP_TRANSFER :
			pszCmd = "DSP_VC_STOP_TRANSFER";
			break;
		case DSP_VC_UPDATE_FLAGS :
			pszCmd = "DSP_VC_UPDATE_FLAGS";
			break;
		case DSP_VC_RESET :
			pszCmd = "DSP_VC_RESET";
			break;
		default :
			pszCmd = "?????";
			break;
	}

	ECHO_DEBUGPRINTF( ("SendVector: %s dwCommand %s (0x%x)\n",
								GetCardName(),
								pszCmd,
								dwCommand) );				
#endif

	m_pOsSupport->OsGetSystemTime( &ullCurTime );
	ullTimeout = ullCurTime + 100000L;		// 100m.s.

	//
	// Wait for the "vector busy" bit to be off
	//
	while ( ullCurTime <= ullTimeout) 
	{
		DWORD dwReg;

		dwReg = GetDspRegister( CHI32_VECTOR_REG );
		if ( 0 == (dwReg & CHI32_VECTOR_BUSY) )
		{
			SetDspRegister( CHI32_VECTOR_REG, dwCommand );
			
			return ECHOSTATUS_OK;
		}
		m_pOsSupport->OsGetSystemTime( &ullCurTime );
	}

	ECHO_DEBUGPRINTF( ("\tPunked out on SendVector\n") );
	ECHO_DEBUGBREAK();
	return ECHOSTATUS_DSP_TIMEOUT;
	
}	// ECHOSTATUS CDspCommObject::SendVector



//===========================================================================
//
//	Some vector commands involve the DSP reading or writing data to and
// from the comm page; if you send one of these commands to the DSP,
// it will complete the command and then write a non-zero value to
// the dwHandshake field in the comm page.  This function waits for the 
// handshake to show up.
//
//===========================================================================

BOOL CDspCommObject::WaitForHandshake()
{
	ULONGLONG ullDelta;
	ULONGLONG ullStartTime,ullTime;
	
	//
	// Wait up to three milliseconds for the handshake from the DSP 
	//
	m_pOsSupport->OsGetSystemTime( &ullStartTime );
	do
	{
		// Look for the handshake value
		if ( 0 != GetHandshakeFlag() )
		{
			return TRUE;
		}

		// Give the DSP time to access the comm page
		m_pOsSupport->OsSnooze( 2 );
		
		m_pOsSupport->OsGetSystemTime(&ullTime);
		ullDelta = ullTime - ullStartTime;
	} while (ullDelta < (ULONGLONG) HANDSHAKE_TIMEOUT);

	ECHO_DEBUGPRINTF( ("CDspCommObject::WaitForHandshake: Timeout waiting "
								"for DSP\n") );
	ECHO_DEBUGBREAK();
	return FALSE;
	
}		// DWORD	CDspCommObject::WaitForHandshake()




/****************************************************************************

	Transport methods

 ****************************************************************************/

//===========================================================================
//
// StartTransport starts transport for a set of pipes
//
//===========================================================================

ECHOSTATUS CDspCommObject::StartTransport
(
	PCChannelMask	pChannelMask			// Pipes to start
)
{
	ECHO_DEBUGPRINTF( ("StartTransport\n") );

	//
	// Wait for the previous command to complete
	//
	if ( !WaitForHandshake() )
		return ECHOSTATUS_DSP_DEAD;

	//
	// Write the appropriate fields in the comm page
	//
	m_pDspCommPage->cmdStart.Clear();
	m_pDspCommPage->cmdStart = *pChannelMask;
	if ( !m_pDspCommPage->cmdStart.IsEmpty() )
	{
		//
		// Clear the handshake and send the vector command
		//
		ClearHandshake();
		SendVector( DSP_VC_START_TRANSFER );

		//
		// Keep track of which pipes are transporting
		//
		m_cmActive += *pChannelMask;

		return ECHOSTATUS_OK;
	}		// if this monkey is being started

	ECHO_DEBUGPRINTF( ("CDspCommObject::StartTransport: No pipes to start!\n") );
	return ECHOSTATUS_INVALID_CHANNEL;
	
}	// ECHOSTATUS CDspCommObject::StartTransport


//===========================================================================
//
// StopTransport pauses transport for a set of pipes
//
//===========================================================================

ECHOSTATUS CDspCommObject::StopTransport
(
	PCChannelMask	pChannelMask
)
{
	ECHO_DEBUGPRINTF(("StopTransport\n"));

	//
	// Wait for the last command to finish
	//
	if ( !WaitForHandshake() )
		return ECHOSTATUS_DSP_DEAD;

	//
	// Write to the comm page
	//
	m_pDspCommPage->cmdStop.Clear();
	m_pDspCommPage->cmdStop = *pChannelMask;
	m_pDspCommPage->cmdReset.Clear();
	if ( !m_pDspCommPage->cmdStop.IsEmpty() )
	{
		//
		// Clear the handshake and send the vector command
		//
		ClearHandshake();
		SendVector( DSP_VC_STOP_TRANSFER );

		//
		// Keep track of which pipes are transporting
		//
		m_cmActive -= *pChannelMask;

		return ECHOSTATUS_OK;
	}		// if this monkey is being started

	ECHO_DEBUGPRINTF( ("CDspCommObject::StopTransport: No pipes to stop!\n") );
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CDspCommObject::StopTransport


//===========================================================================
//
// ResetTransport resets transport for a set of pipes
//
//===========================================================================

ECHOSTATUS CDspCommObject::ResetTransport
(
	PCChannelMask	pChannelMask
)
{
	ECHO_DEBUGPRINTF(("ResetTransport\n"));

	//
	// Wait for the last command to finish
	//
	if ( !WaitForHandshake() )
		return ECHOSTATUS_DSP_DEAD;

	//
	// Write to the comm page
	//
	m_pDspCommPage->cmdStop.Clear();
	m_pDspCommPage->cmdReset.Clear();
	m_pDspCommPage->cmdStop = *pChannelMask;
	m_pDspCommPage->cmdReset = *pChannelMask;
	if ( !m_pDspCommPage->cmdReset.IsEmpty() )
	{
		//
		// Clear the handshake and send the vector command
		//
		ClearHandshake();
		SendVector( DSP_VC_STOP_TRANSFER );

		//
		// Keep track of which pipes are transporting
		//
		m_cmActive -= *pChannelMask;

		return ECHOSTATUS_OK;
	}		// if this monkey is being started

	ECHO_DEBUGPRINTF( ("CDspCommObject::ResetTransport: No pipes to reset!\n") );
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CDspCommObject::ResetTransport


//===========================================================================
//
// This tells the DSP where to start reading the scatter-gather list
// for a given pipe.
//
//===========================================================================

void CDspCommObject::SetAudioDuckListPhys
(
	WORD	wPipeIndex,			// Pipe index
	DWORD dwNewPhysAdr		// Physical address asserted on the PCI bus
)
{
	if (wPipeIndex < GetNumPipes() )
	{
		m_pDspCommPage->DuckListPhys[ wPipeIndex ].PhysAddr = 
																		SWAP( dwNewPhysAdr );
	}
}	// void CDspCommObject::SetAudioDuckListPhys



//===========================================================================
//
// Get a mask with active pipes
//
//===========================================================================

void CDspCommObject::GetActivePipes
(
	PCChannelMask	pChannelMask
)
{
	pChannelMask->Clear();
	*pChannelMask += m_cmActive;
}	// void CDspCommObject::GetActivePipes()


//===========================================================================
//
//	Set the audio format for a pipe
//
//===========================================================================

ECHOSTATUS CDspCommObject::SetAudioFormat
(
	WORD 							wPipeIndex,
	PECHOGALS_AUDIOFORMAT	pFormat
)
{
	WORD wDspFormat = DSP_AUDIOFORM_SS_16LE;
	
	ECHO_DEBUGPRINTF(("CDspCommObject::SetAudioFormat - pipe %d  bps %d  channels %d\n",
							wPipeIndex,pFormat->wBitsPerSample,pFormat->wDataInterleave));

	//
	// Check the pipe number
	//
	if (wPipeIndex >= GetNumPipes() )
	{
		ECHO_DEBUGPRINTF( ("CDspCommObject::SetAudioFormat: Invalid pipe"
								 "%d\n",
								 wPipeIndex) );
		return ECHOSTATUS_INVALID_CHANNEL;
	}

	//
	// Look for super-interleave
	//
	if (pFormat->wDataInterleave > 2)
	{
		switch (pFormat->wBitsPerSample)
		{
			case 16 :
				wDspFormat = DSP_AUDIOFORM_SUPER_INTERLEAVE_16LE;
				break;
				
			case 24 :
				wDspFormat = DSP_AUDIOFORM_SUPER_INTERLEAVE_24LE;
				break;
				
			case 32 :
				wDspFormat = DSP_AUDIOFORM_SUPER_INTERLEAVE_32LE;
				break;
		}
		
		wDspFormat |= pFormat->wDataInterleave;
	}
	else
	{
		//
		// For big-endian data, only 32 bit mono->mono samples and 32 bit stereo->stereo
		// are supported
		//
		if (pFormat->byDataAreBigEndian)
		{
			
			switch ( pFormat->wDataInterleave )
			{
				case 1 :
					wDspFormat = DSP_AUDIOFORM_MM_32BE;
					break;
					
#ifdef STEREO_BIG_ENDIAN32_SUPPORT
				case 2 :
					wDspFormat = DSP_AUDIOFORM_SS_32BE;
					break;
#endif

			}
		}
		else
		{
			//
			// Check for 32 bit little-endian mono->mono case
			//
			if ( 	(1 == pFormat->wDataInterleave) &&
					(32 == pFormat->wBitsPerSample) &&
					(0 == pFormat->byMonoToStereo) )
			{
				wDspFormat = DSP_AUDIOFORM_MM_32LE;
			}
			else
			{
				//
				// Handle the other little-endian formats
				//
				switch (pFormat->wBitsPerSample)
				{
					case 8 :
						if (2 == pFormat->wDataInterleave)
							wDspFormat = DSP_AUDIOFORM_SS_8;
						else
							wDspFormat = DSP_AUDIOFORM_MS_8;

						break;
				
					default :		
					case 16 :
						if (2 == pFormat->wDataInterleave)
							wDspFormat = DSP_AUDIOFORM_SS_16LE;
						else
							wDspFormat = DSP_AUDIOFORM_MS_16LE;
						break;	
					
					case 24 :
						if (2 == pFormat->wDataInterleave)
							wDspFormat = DSP_AUDIOFORM_SS_24LE;
						else
							wDspFormat = DSP_AUDIOFORM_MS_24LE;
						break;					
					
					case 32 :
						if (2 == pFormat->wDataInterleave)
							wDspFormat = DSP_AUDIOFORM_SS_32LE;
						else
							wDspFormat = DSP_AUDIOFORM_MS_32LE;
						break;					
				}
				
			} // check other little-endian formats
		
		} // not big endian data
		
	} // not super-interleave
	
	m_pDspCommPage->wAudioFormat[wPipeIndex] = SWAP( wDspFormat );	
	
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CDspCommObject::SetAudioFormat


//===========================================================================
//
//	Get the audio format for a pipe
//
//===========================================================================

ECHOSTATUS CDspCommObject::GetAudioFormat
( 
	WORD 							wPipeIndex,
	PECHOGALS_AUDIOFORMAT	pFormat
)
{
	if (wPipeIndex >= GetNumPipes() )
	{
		ECHO_DEBUGPRINTF( ("CDspCommObject::GetAudioFormat: Invalid pipe %d\n",
								 wPipeIndex) );

		return ECHOSTATUS_INVALID_CHANNEL;
	}

	pFormat->byDataAreBigEndian = 0;	// true for most of the formats
	pFormat->byMonoToStereo = 0;
	
	switch (SWAP(m_pDspCommPage->wAudioFormat[wPipeIndex]))
	{
		case DSP_AUDIOFORM_MS_8 :
			pFormat->wDataInterleave = 1;
			pFormat->wBitsPerSample = 8;
			pFormat->byMonoToStereo = 1;
			break;
			
		case DSP_AUDIOFORM_MS_16LE :
			pFormat->wDataInterleave = 1;
			pFormat->wBitsPerSample = 16;
			pFormat->byMonoToStereo = 1;			
			break;
			
		case DSP_AUDIOFORM_SS_8 :
			pFormat->wDataInterleave = 2;
			pFormat->wBitsPerSample = 8;
			break;

		case DSP_AUDIOFORM_SS_16LE :
			pFormat->wDataInterleave = 2;
			pFormat->wBitsPerSample = 16;
			break;
		
		case DSP_AUDIOFORM_SS_32LE :
			pFormat->wDataInterleave = 2;
			pFormat->wBitsPerSample = 32;
			break;			
		
		case DSP_AUDIOFORM_MS_32LE :
			pFormat->byMonoToStereo = 1;			
			// fall through

		case DSP_AUDIOFORM_MM_32LE :
			pFormat->wDataInterleave = 1;
			pFormat->wBitsPerSample = 32;
			break;			
			
		case DSP_AUDIOFORM_MM_32BE :
			pFormat->wDataInterleave = 1;
			pFormat->wBitsPerSample = 32;
			pFormat->byDataAreBigEndian = 1;
			break;			
			
		case DSP_AUDIOFORM_SS_32BE :
			pFormat->wDataInterleave = 2;
			pFormat->wBitsPerSample = 32;
			pFormat->byDataAreBigEndian = 1;
			break;			
		
	}
	
	return ECHOSTATUS_OK;
	
}	// void CDspCommObject::GetAudioFormat



/****************************************************************************

	Mixer methods

 ****************************************************************************/

//===========================================================================
//
// SetPipeOutGain - set the gain for a single output pipe
//
//===========================================================================

ECHOSTATUS CDspCommObject::SetPipeOutGain
( 
	WORD 	wPipeOut, 
	WORD	wBusOut,
	INT32	iGain,
	BOOL 	fImmediate
)
{
	if ( wPipeOut < m_wNumPipesOut )
	{
		//
		// Wait for the handshake
		//
		if ( !WaitForHandshake() )
			return ECHOSTATUS_DSP_DEAD;
			
		//
		// Save the new value
		//
		iGain = GENERIC_TO_DSP(iGain);
		m_pDspCommPage->OutLineLevel[ wPipeOut ] = (BYTE) iGain;

		/*
		ECHO_DEBUGPRINTF( ("CDspCommObject::SetPipeOutGain: Out pipe %d "
								 "= 0x%lx\n",
								 wPipeOut,
								 iGain) );
		*/

		//
		// If fImmediate is true, then do the gain setting right now.
		// If you want to do a batch of gain settings all at once, it's
		// more efficient to call this several times and then only set
		// fImmediate for the last one; then the DSP picks up all of
		// them at once.
		//								 
		if (fImmediate)
		{
			return UpdateAudioOutLineLevel();
		}

		return ECHOSTATUS_OK;		

	}

	ECHO_DEBUGPRINTF( ("CDspCommObject::SetPipeOutGain: Invalid out pipe "
							 "%d\n",
							 wPipeOut) );
	ECHO_DEBUGBREAK();	 
							 
	return ECHOSTATUS_INVALID_CHANNEL;
	
}	// SetPipeOutGain


//===========================================================================
//
// GetPipeOutGain returns the current gain for an output pipe.  This isn't
// really used as the mixer code in CEchoGals stores logical values for
// these, but it's here for completeness.
//
//===========================================================================

ECHOSTATUS CDspCommObject::GetPipeOutGain
( 
	WORD 	wPipeOut, 
	WORD 	wBusOut,
	INT32 &iGain
)
{
	if (wPipeOut < m_wNumPipesOut)
	{
		iGain = (INT32) (char) m_pDspCommPage->OutLineLevel[ wPipeOut ];
		iGain = DSP_TO_GENERIC(8);
		return ECHOSTATUS_OK;		
	}

	ECHO_DEBUGPRINTF( ("CDspCommObject::GetPipeOutGain: Invalid out pipe "
							 "%d\n",
							 wPipeOut) );
							 
	return ECHOSTATUS_INVALID_CHANNEL;
	
}	// GetPipeOutGain

	

//===========================================================================
//
// Set input bus gain - iGain is in units of 0.5 dB
//
//===========================================================================

ECHOSTATUS CDspCommObject::SetBusInGain( WORD wBusIn, INT32 iGain)
{
	if (wBusIn > m_wNumBussesIn)
		return ECHOSTATUS_INVALID_CHANNEL;

	//
	// Wait for the handshake (OK even if ASIC is not loaded)
	//
	if ( !WaitForHandshake() )
		return ECHOSTATUS_DSP_DEAD;
		
	//
	// Adjust the gain value
	//		
	iGain += GL20_INPUT_GAIN_MAGIC_NUMBER;
	
	//
	// Put it in the comm page
	//
	m_pDspCommPage->InLineLevel[wBusIn] = (BYTE) iGain;

	return UpdateAudioInLineLevel();
}	


//===========================================================================
//
// Get the input bus gain in units of 0.5 dB
//
//===========================================================================

ECHOSTATUS CDspCommObject::GetBusInGain( WORD wBusIn, INT32 &iGain)
{
	if (wBusIn > m_wNumBussesIn)
		return ECHOSTATUS_INVALID_CHANNEL;
		
	iGain = m_pDspCommPage->InLineLevel[wBusIn];
	iGain -= GL20_INPUT_GAIN_MAGIC_NUMBER;

	return ECHOSTATUS_OK;
}	


//===========================================================================
//
//	Set the nominal level for an input or output bus
//
// bState TRUE			-10 nominal level
// bState FALSE		+4 nominal level
//
//===========================================================================

ECHOSTATUS CDspCommObject::SetNominalLevel
(
	WORD	wBus,
	BOOL	bState
)
{
	//
	// Check the pipe index
	//
	if (wBus < (m_wNumBussesOut + m_wNumBussesIn))
	{
		//
		// Wait for the handshake (OK even if ASIC is not loaded)
		//
		if ( !WaitForHandshake() )
			return ECHOSTATUS_DSP_DEAD;

		//
		// Set the nominal bit
		//		
		if ( bState )
			m_pDspCommPage->cmdNominalLevel.SetIndexInMask( wBus );
		else
			m_pDspCommPage->cmdNominalLevel.ClearIndexInMask( wBus );

		return UpdateAudioOutLineLevel();
	}

	ECHO_DEBUGPRINTF( ("CDspCommObject::SetNominalOutLineLevel Invalid "
							 "index %d\n",
							 wBus ) );
	return ECHOSTATUS_INVALID_CHANNEL;

}	// ECHOSTATUS CDspCommObject::SetNominalLevel


//===========================================================================
//
//	Get the nominal level for an input or output bus
//
// bState TRUE			-10 nominal level
// bState FALSE		+4 nominal level
//
//===========================================================================

ECHOSTATUS CDspCommObject::GetNominalLevel
(
	WORD	wBus,
	PBYTE pbyState
)
{

	if (wBus < (m_wNumBussesOut + m_wNumBussesIn))
	{
		*pbyState = (BYTE)
			m_pDspCommPage->cmdNominalLevel.TestIndexInMask( wBus );
		return ECHOSTATUS_OK;
	}

	ECHO_DEBUGPRINTF( ("CDspCommObject::GetNominalLevel Invalid "
							 "index %d\n",
							 wBus ) );
	return ECHOSTATUS_INVALID_CHANNEL;
}	// ECHOSTATUS CDspCommObject::GetNominalLevel


//===========================================================================
//
//	Set the monitor level from an input bus to an output bus.
//
//===========================================================================

ECHOSTATUS CDspCommObject::SetAudioMonitor
(
	WORD	wBusOut,	// output bus
	WORD	wBusIn,	// input bus
	INT32	iGain,
	BOOL 	fImmediate
)
{
	/*
	ECHO_DEBUGPRINTF( ("CDspCommObject::SetAudioMonitor: "
							 "Out %d in %d Gain %d (0x%x)\n",
							 wBusOut, wBusIn, iGain, iGain) );
	*/

	//
	// The monitor array is a one-dimensional array;
	// compute the offset into the array
	//
	WORD	wOffset = ComputeAudioMonitorIndex( wBusOut, wBusIn );

	//
	// Wait for the offset
	//	
	if ( !WaitForHandshake() )
		return ECHOSTATUS_DSP_DEAD;

	//
	// Write the gain value to the comm page
	//
	iGain = GENERIC_TO_DSP(iGain);
	m_pDspCommPage->byMonitors[ wOffset ] = (BYTE) (iGain);
	
	//
	// If fImmediate is set, do the command right now
	//
	if (fImmediate)
	{
		return UpdateAudioOutLineLevel();
	}
	
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CDspCommObject::SetAudioMonitor


//===========================================================================
//
// SetMetersOn turns the meters on or off.  If meters are turned on, the
// DSP will write the meter and clock detect values to the comm page
// at about 30 Hz.
//
//===========================================================================

ECHOSTATUS CDspCommObject::SetMetersOn
(
	BOOL bOn
)
{
	if ( bOn )
	{
		if ( 0 == m_wMeterOnCount )
		{
			SendVector( DSP_VC_METERS_ON );
		}
		m_wMeterOnCount++;
	}
	else
	{
		INT32	iDevice;
	
		if ( m_wMeterOnCount == 0 )
			return ECHOSTATUS_OK;

		if ( 0 == --m_wMeterOnCount )
		{
			SendVector( DSP_VC_METERS_OFF );
			
			for ( iDevice = 0; iDevice < DSP_MAXPIPES; iDevice++ )
			{
				BYTE muted;

				muted = (BYTE) GENERIC_TO_DSP(ECHOGAIN_MUTED);
				m_pDspCommPage->VUMeter[ iDevice ]   = muted;
				m_pDspCommPage->PeakMeter[ iDevice ] = muted;
			}
		}
	}
	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS CDspCommObject::SetMetersOn


//===========================================================================
//
// Tell the DSP to read and update output, nominal & monitor levels 
//	in comm page.
//
//===========================================================================

ECHOSTATUS CDspCommObject::UpdateAudioOutLineLevel()
{
	//ECHO_DEBUGPRINTF( ( "CDspCommObject::UpdateAudioOutLineLevel:\n" ) );
	
	if (FALSE == m_bASICLoaded)
		return ECHOSTATUS_ASIC_NOT_LOADED;
	
	ClearHandshake();
	return( SendVector( DSP_VC_UPDATE_OUTVOL ) );
	
}	// ECHOSTATUS CDspCommObject::UpdateAudioOutLineLevel()


//===========================================================================
//
// Tell the DSP to read and update input levels in comm page
//
//===========================================================================

ECHOSTATUS CDspCommObject::UpdateAudioInLineLevel()
{
	//ECHO_DEBUGPRINTF( ( "CDspCommObject::UpdateAudioInLineLevel:\n" ) );
	
	if (FALSE == m_bASICLoaded)
		return ECHOSTATUS_ASIC_NOT_LOADED;

	ClearHandshake();
	return( SendVector( DSP_VC_UPDATE_INGAIN ) );
}		// ECHOSTATUS CDspCommObject::UpdateAudioInLineLevel()


//===========================================================================
//
// Tell the DSP to read and update virtual mixer levels 
//	in comm page.  This method is overridden by cards that actually 
// support a vmixer.
//
//===========================================================================

ECHOSTATUS CDspCommObject::UpdateVmixerLevel()
{
	ECHO_DEBUGPRINTF(("CDspCommObject::UpdateVmixerLevel\n"));
	return ECHOSTATUS_NOT_SUPPORTED;
}	// ECHOSTATUS CDspCommObject::UpdateVmixerLevel()


//===========================================================================
//
// Tell the DSP to change the input clock
//
//===========================================================================

ECHOSTATUS CDspCommObject::SetInputClock(WORD wClock)
{
	//
	// Wait for the last command
	//
	if (!WaitForHandshake())
		return ECHOSTATUS_DSP_DEAD;

	ECHO_DEBUGPRINTF( ("CDspCommObject::SetInputClock:\n") );		
		
	//
	// Write to the comm page
	//
	m_pDspCommPage->wInputClock = SWAP(wClock);
	
	//
	// Clear the handshake and send the command
	//
	ClearHandshake();
	ECHOSTATUS Status = SendVector(DSP_VC_UPDATE_CLOCKS);
	
	return Status;

}	// ECHOSTATUS CDspCommObject::SetInputClock


//===========================================================================
//
// Tell the DSP to change the output clock - Layla20 only
//
//===========================================================================

ECHOSTATUS CDspCommObject::SetOutputClock(WORD wClock)
{

	return ECHOSTATUS_CLOCK_NOT_SUPPORTED;
	
}	// ECHOSTATUS CDspCommObject::SetOutputClock


//===========================================================================
//
// Fill out an ECHOGALS_METERS struct using the current values in the 
// comm page.  This method is overridden for vmixer cards.
//
//===========================================================================

ECHOSTATUS CDspCommObject::GetAudioMeters
(
	PECHOGALS_METERS	pMeters
)
{
	pMeters->iNumPipesOut = 0;
	pMeters->iNumPipesIn = 0;

	//
	//	Output 
	// 
	DWORD dwCh = 0;
	WORD 	i;

	pMeters->iNumBussesOut = (INT32) m_wNumBussesOut;
	for (i = 0; i < m_wNumBussesOut; i++)
	{
		pMeters->iBusOutVU[i] = 
			DSP_TO_GENERIC( ((INT32) (INT8) m_pDspCommPage->VUMeter[ dwCh ]) );

		pMeters->iBusOutPeak[i] = 
			DSP_TO_GENERIC( ((INT32) (INT8) m_pDspCommPage->PeakMeter[ dwCh ]) );
		
		dwCh++;
	}

	pMeters->iNumBussesIn = (INT32) m_wNumBussesIn;	
	for (i = 0; i < m_wNumBussesIn; i++)
	{
		pMeters->iBusInVU[i] = 
			DSP_TO_GENERIC( ((INT32) (INT8) m_pDspCommPage->VUMeter[ dwCh ]) );
		pMeters->iBusInPeak[i] = 
			DSP_TO_GENERIC( ((INT32) (INT8) m_pDspCommPage->PeakMeter[ dwCh ]) );
		
		dwCh++;
	}
	
	return ECHOSTATUS_OK;
	
} // GetAudioMeters


#ifdef DIGITAL_INPUT_AUTO_MUTE_SUPPORT

//===========================================================================
//
// Digital input auto-mute - Gina24, Layla24, and Mona only
//
//===========================================================================

ECHOSTATUS CDspCommObject::GetDigitalInputAutoMute(BOOL &fAutoMute)
{
	fAutoMute = m_fDigitalInAutoMute;	
	
	ECHO_DEBUGPRINTF(("CDspCommObject::GetDigitalInputAutoMute %d\n",fAutoMute));
	
	return ECHOSTATUS_OK;
}

ECHOSTATUS CDspCommObject::SetDigitalInputAutoMute(BOOL fAutoMute)
{
	ECHO_DEBUGPRINTF(("CDspCommObject::SetDigitalInputAutoMute %d\n",fAutoMute));
	
	//
	// Store the flag
	//
	m_fDigitalInAutoMute = fAutoMute;
	
	//
	// Re-set the input clock to the current value - indirectly causes the 
	// auto-mute flag to be sent to the DSP
	//
	SetInputClock(m_wInputClock);
	
	return ECHOSTATUS_OK;
}

#endif // DIGITAL_INPUT_AUTO_MUTE_SUPPORT




/****************************************************************************

	Power management

 ****************************************************************************/

//===========================================================================
//
// Tell the DSP to go into low-power mode
//
//===========================================================================

ECHOSTATUS CDspCommObject::GoComatose()
{
	ECHO_DEBUGPRINTF(("CDspCommObject::GoComatose\n"));

	if (NULL != m_pwDspCode)
	{
		//
		// Make LoadFirmware do a complete reload
		//	
		m_pwDspCode = NULL;
		
		//
		// Make sure that the sample rate get re-set on wakeup
		// (really only for Indigo and Mia)
		//
		m_pDspCommPage->dwControlReg = 0;
		
		//
		// Put the DSP to sleep
		//
		return SendVector(DSP_VC_GO_COMATOSE);
	}
	
	return ECHOSTATUS_OK;

}	// end of GoComatose



#ifdef MIDI_SUPPORT

/****************************************************************************

	MIDI

 ****************************************************************************/

//===========================================================================
//
// Send a buffer full of MIDI data to the DSP
//
//===========================================================================

ECHOSTATUS CDspCommObject::WriteMidi
(
	PBYTE		pData,						// Ptr to data buffer
	DWORD		dwLength,					// How many bytes to write
	PDWORD	pdwActualCt					// Return how many actually written
)
{
	DWORD 		dwWriteCount,dwHandshake,dwStatus;
	BYTE			*pOutBuffer;
	
	
	//
	// Return immediately if the handshake flag is clar
	//
	dwHandshake = GetHandshakeFlag();
	if (0 == dwHandshake)
	{
		ECHO_DEBUGPRINTF(("CDCO::WriteMidi - can't write - handshake %ld\n",dwHandshake));
		
		*pdwActualCt = 0;
		return ECHOSTATUS_BUSY;
	}
	
	//
	// Return immediately if HF4 is clear - HF4 indicates that it is safe
	// to write MIDI output data
	//
	dwStatus = GetDspRegister( CHI32_STATUS_REG );
	if ( 0 == (dwStatus & CHI32_STATUS_REG_HF4 ) )
	{
		ECHO_DEBUGPRINTF(("CDCO::WriteMidi - can't write - dwStatus 0x%lx\n",dwStatus));
		
		*pdwActualCt = 0;
		return ECHOSTATUS_BUSY;
	}
	
	
	//
	// Copy data to the comm page; limit to the amount of space in the DSP output
	// FIFO and in the comm page
	//
	dwWriteCount = dwLength;
	if (dwWriteCount > (CP_MIDI_OUT_BUFFER_SIZE - 1))
	{
		dwWriteCount = CP_MIDI_OUT_BUFFER_SIZE - 1;
	}

	ECHO_DEBUGPRINTF(("WriteMidi - dwWriteCount %ld\n",dwWriteCount));
			
	*pdwActualCt = dwWriteCount;	// Save the # of bytes written for the caller

	pOutBuffer = m_pDspCommPage->byMidiOutData;
	*pOutBuffer = (BYTE) dwWriteCount;
	
	pOutBuffer++;
	
	OsCopyMemory(pOutBuffer,pData,dwWriteCount);

	//
	// Send the command to the DSP
	//	 
	ClearHandshake();
	m_pDspCommPage->dwMidiOutFreeCount = 0;
	SendVector( DSP_VC_MIDI_WRITE );
	
	//
	// Save the current time - used to detect if MIDI out is currently busy
	//
	ULONGLONG ullTime;

	m_pOsSupport->OsGetSystemTime( &ullTime );
	m_ullMidiOutTime = ullTime;
														
	return ECHOSTATUS_OK;
	
}		// ECHOSTATUS CDspCommObject::WriteMidi


//===========================================================================
//
// Called from the interrupt handler - get a MIDI input byte
//
//===========================================================================

ECHOSTATUS CDspCommObject::ReadMidi
(
	WORD 		wIndex,				// Buffer index
	DWORD &	dwData				// Return data
)
{
	if ( wIndex >= CP_MIDI_IN_BUFFER_SIZE )
		return ECHOSTATUS_INVALID_INDEX;

	//
	// Get the data
	//	
	dwData = SWAP( m_pDspCommPage->wMidiInData[ wIndex ] );

	//
	// Timestamp for the MIDI input activity indicator
	//
	ULONGLONG ullTime;

	m_pOsSupport->OsGetSystemTime( &ullTime );
	m_ullMidiInTime = ullTime;

	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS CDspCommObject::ReadMidi


ECHOSTATUS CDspCommObject::SetMidiOn( BOOL bOn )
{
	if ( bOn )
	{
		if ( 0 == m_wMidiOnCount )
		{
			if ( !WaitForHandshake() )
				return ECHOSTATUS_DSP_DEAD;

			m_pDspCommPage->dwFlags |= SWAP( (DWORD) DSP_FLAG_MIDI_INPUT );
			
			ClearHandshake();
			SendVector( DSP_VC_UPDATE_FLAGS );
		}
		m_wMidiOnCount++;
	}
	else
	{
		if ( m_wMidiOnCount == 0 )
			return ECHOSTATUS_OK;

		if ( 0 == --m_wMidiOnCount )
		{
			if ( !WaitForHandshake() )
				return ECHOSTATUS_DSP_DEAD;
				
			m_pDspCommPage->dwFlags &= SWAP( (DWORD) ~DSP_FLAG_MIDI_INPUT );
			
			ClearHandshake();
			SendVector( DSP_VC_UPDATE_FLAGS );
		}
	}

	return ECHOSTATUS_OK;

}	// ECHOSTATUS CDspCommObject::SetMidiOn


//===========================================================================
//
// Detect MIDI output activity
//
//===========================================================================

BOOL CDspCommObject::IsMidiOutActive()
{
	ULONGLONG	ullCurTime;

	m_pOsSupport->OsGetSystemTime( &ullCurTime );
	return( ( ( ullCurTime - m_ullMidiOutTime ) > MIDI_ACTIVITY_TIMEOUT_USEC ) ? FALSE : TRUE );
	
}	// BOOL CDspCommObject::IsMidiOutActive()


#endif // MIDI_SUPPORT



// **** CDspCommObject.cpp ****
