// ****************************************************************************
//
//		CEchoGals_mixer.cpp
//
//		Implementation file for the CEchoGals driver class (mixer functions).
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

#undef ECHO_DEBUGPRINTF
#define ECHO_DEBUGPRINTF(x)

/****************************************************************************

	CEchoGals mixer client management

 ****************************************************************************/

//===========================================================================
//
// Open the mixer - create a mixer client structure for this client and 
// return the cookie.  The cookie uniquely identifies this client to the 
// mixer driver. 
//
// Valid cookies are non-zero.  If you get a zero cookie, the open failed
// somehow.
//
// Clients can change mixer controls without calling OpenMixer first; it just
// means that they can't track control changes made by other clients.
//
//===========================================================================

ECHOSTATUS CEchoGals::OpenMixer(NUINT &Cookie)
{
	ECHO_MIXER_CLIENT *pTemp;

	if (m_fMixerDisabled)
		return ECHOSTATUS_MIXER_DISABLED;

	//
	// If the cookie is non-zero, then use the specified value
	//
	if (0 != Cookie)
	{
		pTemp = m_pMixerClients;
		while (pTemp != NULL)
		{
			if (Cookie == pTemp->Cookie)
			{
				ECHO_DEBUGPRINTF(("CEchoGals::OpenMixer - cookie 0x%lx already in use\n",
										Cookie));
				return ECHOSTATUS_BAD_COOKIE;
			}
			
			pTemp = pTemp->pNext;
		}
	}
	else
	{
		//
		// Make a new cookie
		//
		ULONGLONG ullTime;

		m_pOsSupport->OsGetSystemTime(&ullTime);
		Cookie = (NUINT) ullTime;
		if (0 == Cookie)
			Cookie = 1;
		
		//
		// Look through the existing mixer client list to ensure that this
		// cookie is unique.
		//
		pTemp = m_pMixerClients;
		while (pTemp != NULL)
		{
			if (Cookie == pTemp->Cookie)
			{
				//
				// Oops, someone is already using this cookie.  Increment 
				// the new cookie and try again.
				//
				Cookie++;
				pTemp = m_pMixerClients;			
			}
		
			pTemp = pTemp->pNext;
		}
		
	}
	

	//
	// Allocate the mixer client structure
	//
	ECHO_MIXER_CLIENT *pClient = NULL;
	ECHOSTATUS			Status;
	
	Status = OsAllocateNonPaged(sizeof(ECHO_MIXER_CLIENT),(void **) &pClient);
	if (NULL == pClient)
	{
		Cookie = 0;
		return Status;		
	}

	
	//
	// Store the new cookie and the new mixer client
	//
	pClient->Cookie = Cookie;
	pClient->pNext = m_pMixerClients;
	m_pMixerClients = pClient;
	
	return ECHOSTATUS_OK;
	
}	// OpenMixer


//===========================================================================
//
// Find a mixer client that matches a cookie
//
//===========================================================================

ECHO_MIXER_CLIENT *CEchoGals::GetMixerClient(NUINT Cookie)
{
	ECHO_MIXER_CLIENT *pTemp;
	
	pTemp = m_pMixerClients;
	while (NULL != pTemp)
	{
		if (Cookie == pTemp->Cookie)
			break;
			
		pTemp = pTemp->pNext;
	}
	
	return pTemp;
	
}	// GetMixerClient


//===========================================================================
//
// Close the mixer - free the mixer client structure
//
//===========================================================================

ECHOSTATUS CEchoGals::CloseMixer(NUINT Cookie)
{
	//
	//	Search the linked list and remove the client that matches the cookie
	// 
	ECHO_MIXER_CLIENT *pTemp;
	
	pTemp = m_pMixerClients;
	if (NULL == pTemp)
		return ECHOSTATUS_BAD_COOKIE;
		
	//
	// Head of the list?
	//		
	if (pTemp->Cookie == Cookie)
	{
		m_pMixerClients = pTemp->pNext;
		OsFreeNonPaged(pTemp);
		
		return ECHOSTATUS_OK;
	}
		
	//
	// Not the head of the list; search the list
	//
	while (NULL != pTemp->pNext)
	{
		if (Cookie == pTemp->pNext->Cookie)
		{
			ECHO_MIXER_CLIENT *pDeadClient;
			
			pDeadClient = pTemp->pNext;
			pTemp->pNext = pDeadClient->pNext;
			OsFreeNonPaged(pDeadClient);
			
			return ECHOSTATUS_OK;
		}
			
		pTemp = pTemp->pNext;
	}

	//
	// No soup for you!
	//
	return ECHOSTATUS_BAD_COOKIE;

} // CloseMixer	


//===========================================================================
//
// IsMixerOpen - returns true if at least one client has the mixer open
//
//===========================================================================

BOOL CEchoGals::IsMixerOpen()
{
	if (NULL == m_pMixerClients)
		return FALSE;
		
	return TRUE;
	
}	// IsMixerOpen


//===========================================================================
//
// This function is called when a mixer control changes; add the change
//	to the queue for each client.
//
// Here's what the wCh1 and wCh2 parameters represent, based on the wType
// parameter:
//
// wType				wCh1				wCh2
// -----				----				----
// ECHO_BUS_OUT	Output bus		Ignored
// ECHO_BUS_IN		Input bus		Ignored
// ECHO_PIPE_OUT	Output pipe		Output bus
// ECHO_MONITOR	Input bus		Output bus
//
// ECHO_PIPE_IN is not used right now.
//
//===========================================================================

ECHOSTATUS CEchoGals::MixerControlChanged
(
	WORD	wType,		// One of the ECHO_CHANNEL_TYPES
	WORD  wParameter, // One of the MXN_* values
	WORD 	wCh1,			// Depends on the wType value
	WORD  wCh2			// Also depends on wType value
)
{
	ECHO_MIXER_CLIENT *pClient = m_pMixerClients;
	PMIXER_NOTIFY		pNotify;
	
	if (m_fMixerDisabled)
		return ECHOSTATUS_MIXER_DISABLED;
	
	//
	// Go through all the clients and store this control change
	//
	while (NULL != pClient)
	{
		//
		// Search the circular buffer for this client and see if 
		// this control change is already stored
		//
		DWORD				dwIndex,dwCount;
		BOOL				fFound;
		
		dwCount = pClient->dwCount;
		dwIndex = pClient->dwTail;
		fFound = FALSE;
		while (dwCount > 0)
		{
			pNotify = pClient->Notifies + dwIndex;
			if (	(pNotify->wType == wType) &&
					(pNotify->wParameter == wParameter) &&
					(pNotify->u.wPipeOut == wCh1) && 	// can use any union member her
					(pNotify->wBusOut == wCh2))
			{
				//
				// Found this notify already in the circular buffer 
				//
				fFound = TRUE;
				break;
			}
			dwIndex++;
			dwIndex &= MAX_MIXER_NOTIFIES - 1;
			dwCount--;
		}
		
		//
		// If the notify was not found, add this notify to the circular buffer if 
		// there is enough room.
		//
		if ( 	(FALSE == fFound) &&
				(pClient->dwCount != MAX_MIXER_NOTIFIES))
		{
			pNotify = pClient->Notifies + pClient->dwHead;
			
			pNotify->wType 		= wType;
			pNotify->wParameter 	= wParameter;
			
			if (ECHO_BUS_OUT == wType)
			{
				pNotify->u.wPipeOut = ECHO_CHANNEL_UNUSED;
				pNotify->wBusOut = wCh1;
			}
			else
			{
				pNotify->u.wPipeOut	= wCh1;		// can use any union member here also
				pNotify->wBusOut	= wCh2;
			}
			
			pClient->dwCount += 1;
			pClient->dwHead = (pClient->dwHead + 1) & (MAX_MIXER_NOTIFIES - 1);
		}
		
		pClient = pClient->pNext;
	}
	
	return ECHOSTATUS_OK;
	
}	// MixerControlChanged


//===========================================================================
//
// This method is called when a client wants to know what controls have 
// changed.
//
//===========================================================================

ECHOSTATUS CEchoGals::GetControlChanges
(
	PMIXER_MULTI_NOTIFY	pNotifies,
	NUINT	MixerCookie
)
{
	//
	// Match the cookie
	//
	ECHO_MIXER_CLIENT *pClient = GetMixerClient(MixerCookie);
	
	if (NULL == pClient)
	{
		pNotifies->dwCount = 0;
		return ECHOSTATUS_BAD_COOKIE;		
	}
	
	//
	// Copy mixer notifies
	//
	PMIXER_NOTIFY	pDest,pSrc;
	DWORD				dwNumClientNotifies,dwNumReturned;
	
	dwNumClientNotifies = pNotifies->dwCount;
	pDest = pNotifies->Notifies;
	dwNumReturned = 0;
	while ( (dwNumClientNotifies > 0) && (pClient->dwCount > 0))
	{
		pSrc = pClient->Notifies + pClient->dwTail;
	
		OsCopyMemory(pDest,pSrc,sizeof(MIXER_NOTIFY));
		
		pDest++;

		pClient->dwTail = (pClient->dwTail + 1) & (MAX_MIXER_NOTIFIES - 1);
		pClient->dwCount -= 1;

		dwNumClientNotifies--;

		dwNumReturned++;
	}
	
	pNotifies->dwCount = dwNumReturned;
	
	return ECHOSTATUS_OK;

}	// GetControlChanges




/****************************************************************************

	CEchoGals mixer control

 ****************************************************************************/

//===========================================================================
//
// Process mixer function - service a single mixer function
//
//===========================================================================

ECHOSTATUS CEchoGals::ProcessMixerFunction
(
	PMIXER_FUNCTION	pMixerFunction,
	INT32 &					iRtnDataSz
)
{
	ECHOSTATUS			Status = ECHOSTATUS_OK;
	
	if (m_fMixerDisabled)
		return ECHOSTATUS_MIXER_DISABLED;
	
	switch ( pMixerFunction->iFunction )
	{
		case MXF_GET_CAPS :
			Status = GetCapabilities( &pMixerFunction->Data.Capabilities );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_CAPS  Status %ld\n", Status) );
			break;

		case MXF_GET_LEVEL :
			Status = GetAudioLineLevel( pMixerFunction);
			/*
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_LEVEL  Status %ld\n", Status) );
			*/
			break;

		case MXF_SET_LEVEL :
			Status = SetAudioLineLevel( pMixerFunction);
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_LEVEL  Status %ld\n", Status) );
			break;

		case MXF_GET_NOMINAL :
			Status = GetAudioNominal( pMixerFunction);
			/*
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_NOMINAL  Status %ld\n", Status) );
			*/
			break;

		case MXF_SET_NOMINAL :
			Status = SetAudioNominal( pMixerFunction);
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_NOMINAL  Status %ld\n", Status) );
			break;

		case MXF_GET_MONITOR :
			Status = GetAudioMonitor( pMixerFunction->Channel.wChannel,
											  pMixerFunction->Data.Monitor.wBusOut,
											  pMixerFunction->Data.Monitor.Data.iLevel );
			/*	  
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_MONITOR  Status %ld\n", Status) );
			*/
			break;

		case MXF_SET_MONITOR :
			Status = SetAudioMonitor( pMixerFunction->Channel.wChannel,
											  pMixerFunction->Data.Monitor.wBusOut,
											  pMixerFunction->Data.Monitor.Data.iLevel );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_MONITOR  Status %ld\n", Status) );
			break;

		case MXF_GET_CLOCK_DETECT :
			Status = GetInputClockDetect( pMixerFunction->Data.dwClockDetectBits );
			break;
	
		case MXF_GET_INPUT_CLOCK :
			Status = GetInputClock( pMixerFunction->Data.wClock );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_INPUT_CLOCK  Status %ld\n", Status) );
			break;			

		case MXF_SET_INPUT_CLOCK :
			Status = SetInputClock( pMixerFunction->Data.wClock );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_INPUT_CLOCK  Status %ld\n", Status) );
			break;			


		case MXF_GET_OUTPUT_CLOCK :
			Status = GetOutputClock( pMixerFunction->Data.wClock );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_OUTPUT_CLOCK  Status %ld\n", Status) );
			break;			

		case MXF_SET_OUTPUT_CLOCK :
			Status = SetOutputClock( pMixerFunction->Data.wClock );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_OUTPUT_CLOCK  Status %ld\n", Status) );
			break;			


		case MXF_GET_METERS :
		
			if (NULL != GetDspCommObject())
			{
				Status = GetDspCommObject()->
								GetAudioMeters( &pMixerFunction->Data.Meters );				
			}
			else
			{
				Status = ECHOSTATUS_DSP_DEAD;
			}

			//ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
			// 						 "MXF_GET_METERS  Status %ld\n", Status) );
			break;

		case MXF_GET_METERS_ON :
			Status = GetMetersOn( pMixerFunction->Data.bMetersOn );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_METERS  Status %ld\n", Status) );
			break;

		case MXF_SET_METERS_ON :
			Status = SetMetersOn( pMixerFunction->Data.bMetersOn );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_METERS_ON  Status %ld\n", Status) );
			break;

		case MXF_GET_PROF_SPDIF :
			if ( ECHOSTATUS_DSP_DEAD == IsProfessionalSpdif() )
			{
				Status = ECHOSTATUS_DSP_DEAD;				
			}
			else
			{
				pMixerFunction->Data.bProfSpdif = IsProfessionalSpdif();
			}
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_PROF_SPDIF  Pro S/PDIF: 0x%x  Status %ld\n", 
									 pMixerFunction->Data.bProfSpdif,
									 Status) );
			break;

		case MXF_SET_PROF_SPDIF :
			SetProfessionalSpdif( pMixerFunction->Data.bProfSpdif );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_PROF_SPDIF  Pro S/PDIF: 0x%x  Status %ld\n", 
									 pMixerFunction->Data.bProfSpdif,
									 Status) );
			break;

		case MXF_GET_MUTE :
			Status = GetAudioMute(pMixerFunction);
			/*
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_MUTE  Status %ld\n", Status) );
			*/
			break;

		case MXF_SET_MUTE :
			Status = SetAudioMute(pMixerFunction);
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_MUTE  Status %ld\n", Status) );
			break;

		case MXF_GET_MONITOR_MUTE :
			Status = 
				GetAudioMonitorMute( pMixerFunction->Channel.wChannel,
										   pMixerFunction->Data.Monitor.wBusOut,
										   pMixerFunction->Data.Monitor.Data.bMuteOn );
			/*
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_MONITOR_MUTE  Status %ld\n", Status) );
			*/
			break;

		case MXF_SET_MONITOR_MUTE :
			Status = 
				SetAudioMonitorMute( pMixerFunction->Channel.wChannel,
										   pMixerFunction->Data.Monitor.wBusOut,
										   pMixerFunction->Data.Monitor.Data.bMuteOn );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_MONITOR_MUTE  Status %ld\n", Status) );
			break;

		case MXF_GET_MONITOR_PAN :
			Status = 
				GetAudioMonitorPan( pMixerFunction->Channel.wChannel,
										  pMixerFunction->Data.Monitor.wBusOut,
										  pMixerFunction->Data.Monitor.Data.iPan);
			/*

			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_MONITOR_PAN  Status %ld\n", Status) );
			*/									 
									 
			break;
		case MXF_SET_MONITOR_PAN :
			Status = 
				SetAudioMonitorPan( pMixerFunction->Channel.wChannel,
										  pMixerFunction->Data.Monitor.wBusOut,
										  pMixerFunction->Data.Monitor.Data.iPan );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_MONITOR_PAN  Status %ld\n", Status) );
			break;

		case MXF_GET_FLAGS :
			pMixerFunction->Data.wFlags = GetFlags();
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_FLAGS 0x%x  Status %ld\n",
									 pMixerFunction->Data.wFlags,
									 Status) );
			break;
		case MXF_SET_FLAGS :
			SetFlags( pMixerFunction->Data.wFlags );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_FLAGS 0x%x  Status %ld\n",
									 pMixerFunction->Data.wFlags,
									 Status) );
			break;
		case MXF_CLEAR_FLAGS :
			ClearFlags( pMixerFunction->Data.wFlags );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_CLEAR_FLAGS 0x%x  Status %ld\n",
									 pMixerFunction->Data.wFlags,
									 Status) );
			break;

		case MXF_GET_SAMPLERATE_LOCK :
			GetAudioLockedSampleRate( pMixerFunction->Data.dwLockedSampleRate );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_SAMPLERATE_LOCK 0x%lx  Status %ld\n",
									 pMixerFunction->Data.dwLockedSampleRate,
									 Status) );
			break;

		case MXF_SET_SAMPLERATE_LOCK :
			SetAudioLockedSampleRate( pMixerFunction->Data.dwLockedSampleRate );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_SAMPLERATE_LOCK 0x%lx  Status %ld\n",
									 pMixerFunction->Data.dwLockedSampleRate,
									 Status) );
			break;

		case MXF_GET_SAMPLERATE :
		
			GetAudioSampleRate( &pMixerFunction->Data.dwSampleRate );

			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_SAMPLERATE 0x%lx  Status %ld\n",
									 pMixerFunction->Data.dwSampleRate,
									 Status) );
			break;

#ifdef MIDI_SUPPORT			

		case MXF_GET_MIDI_IN_ACTIVITY :
			pMixerFunction->Data.bMidiActive = m_MidiIn.IsActive();

			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_MIDI_IN_ACTIVITY %s "
									 "Status %ld\n",
									 ( pMixerFunction->Data.bMidiActive ) 
										? "ACTIVE" : "INACTIVE",
									 Status) );
			break;


		case MXF_GET_MIDI_OUT_ACTIVITY :
			pMixerFunction->Data.bMidiActive =
				GetDspCommObject()->IsMidiOutActive();

			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_MIDI_OUT_ACTIVITY %s "
									 "Status %ld\n",
									 ( pMixerFunction->Data.bMidiActive ) 
										? "ACTIVE" : "INACTIVE",
									 Status) );
			break;

#endif // MIDI_SUPPORT


		case MXF_GET_DIGITAL_MODE :
			Status = ECHOSTATUS_OK;
			pMixerFunction->Data.iDigMode = GetDigitalMode();
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_DIGITAL_MODE %s "
									 "Status %ld\n",
									 ( DIGITAL_MODE_SPDIF_RCA == 
											pMixerFunction->Data.iDigMode )
										? "S/PDIF RCA" 
										: ( DIGITAL_MODE_SPDIF_OPTICAL == 
											pMixerFunction->Data.iDigMode )
												? "S/PDIF Optical" : "ADAT",
									 Status) );
			break;


		case MXF_SET_DIGITAL_MODE :
			Status = SetDigitalMode( (BYTE) pMixerFunction->Data.iDigMode );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_DIGITAL_MODE %s "
									 "Status %ld\n",
									 ( DIGITAL_MODE_SPDIF_RCA == 
											pMixerFunction->Data.iDigMode )
										? "S/PDIF RCA" 
										: ( DIGITAL_MODE_SPDIF_OPTICAL == 
											pMixerFunction->Data.iDigMode )
												? "S/PDIF Optical" : "ADAT",
									 Status) );
			break;


		case MXF_GET_PAN :
			Status = GetAudioPan( pMixerFunction);
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_PAN  Status %ld\n", Status) );
			break;

		case MXF_SET_PAN :
			Status = SetAudioPan( pMixerFunction);
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_PAN  Status %ld\n", Status) );
			break;
			
#ifdef DIGITAL_INPUT_AUTO_MUTE_SUPPORT

		case MXF_GET_DIG_IN_AUTO_MUTE	:
			Status =  GetDigitalInAutoMute( pMixerFunction );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_GET_DIG_IN_AUTO_MUTE  Status %ld\n", Status) );
			break;
			

		case MXF_SET_DIG_IN_AUTO_MUTE	:
			Status = SetDigitalInAutoMute( pMixerFunction );
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "MXF_SET_DIG_IN_AUTO_MUTE  Status %ld\n", Status) );
			break;

#endif // DIGITAL_INPUT_AUTO_MUTE_SUPPORT

		case MXF_GET_AUDIO_LATENCY :
			GetAudioLatency( &(pMixerFunction->Data.Latency) );
			break;
			
#ifdef PHANTOM_POWER_CONTROL
		
		case MXF_GET_PHANTOM_POWER :
			GetPhantomPower( &(pMixerFunction->Data.fPhantomPower) );
			break;
			
		case MXF_SET_PHANTOM_POWER :
			SetPhantomPower( pMixerFunction->Data.fPhantomPower );
			break;
			
#endif

		default :
			iRtnDataSz = 0;
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerFunction: "
									 "Function %ld not supported\n",
									 pMixerFunction->iFunction) );
			return ECHOSTATUS_NOT_SUPPORTED;
	}

	pMixerFunction->RtnStatus = Status;
	iRtnDataSz = sizeof( MIXER_FUNCTION );

	return Status;

}	// ECHOSTATUS CEchoGals::ProcessMixerFunction




//===========================================================================
//
// Process multiple mixer functions
//
//===========================================================================

ECHOSTATUS CEchoGals::ProcessMixerMultiFunction
(
	PMIXER_MULTI_FUNCTION	pMixerFunctions,	// Request from mixer
	INT32 &							iRtnDataSz			// # bytes returned (if any)
)
{
	ECHOSTATUS			Status = ECHOSTATUS_NOT_SUPPORTED;
	PMIXER_FUNCTION	pMixerFunction;
	INT32					iRtn, nCard, i;
	
	if (m_fMixerDisabled)
		return ECHOSTATUS_MIXER_DISABLED;

	iRtnDataSz = sizeof( MIXER_MULTI_FUNCTION ) - sizeof( MIXER_FUNCTION );
	pMixerFunction = &pMixerFunctions->MixerFunction[ 0 ];
	nCard = pMixerFunction->Channel.wCardId;
	for ( i = 0; i < pMixerFunctions->iCount; i++ )
	{
		pMixerFunction = &pMixerFunctions->MixerFunction[ i ];
		if ( nCard != pMixerFunction->Channel.wCardId )
		{
			ECHO_DEBUGPRINTF( ("CEchoGals::ProcessMixerMultiFunction: "
									 "All functions MUST be for the same card "
									 "exp %ld act %d!\n",
									 nCard,
									 pMixerFunction->Channel.wCardId) );
			return ECHOSTATUS_NOT_SUPPORTED;
		}

		Status = ProcessMixerFunction(pMixerFunction,iRtn);
		iRtnDataSz += iRtn;
	}

	return Status;

}	// ECHOSTATUS CEchoGals::ProcessMixerMultiFunction




//===========================================================================
//
// Typically, if you are writing a console, you will be polling the driver
// to get the current peak and VU meters, clock detect bits, and 
// control changes.  GetPolledStuff will fill out an ECHO_POLLED_STUFF
// structure with all of those things.
//
//===========================================================================

ECHOSTATUS CEchoGals::GetPolledStuff
(
	ECHO_POLLED_STUFF *pPolledStuff,
	NUINT MixerCookie
)
{
	ECHO_MIXER_CLIENT *pClient;
	CDspCommObject *pDCO = GetDspCommObject();
	
	if (m_fMixerDisabled)
		return ECHOSTATUS_MIXER_DISABLED;
	
	if (NULL == pDCO)
		return ECHOSTATUS_DSP_DEAD;
		
	//
	// Fill out the non-client-specific portions of the struct
	//
	pDCO->GetAudioMeters(&(pPolledStuff->Meters));
	GetInputClockDetect(pPolledStuff->dwClockDetectBits);
	
	//
	// If there is a matching client, fill out the number
	// of notifies
	//
	pClient = GetMixerClient(MixerCookie);
	if (NULL == pClient)
		pPolledStuff->dwNumPendingNotifies = 0;
	else	
		pPolledStuff->dwNumPendingNotifies = pClient->dwCount;
	
	return ECHOSTATUS_OK;
	
}	// GetPolledStuff


//===========================================================================
//
//	Get the pan setting for an output pipe (virtual outputs only)
//
//===========================================================================

ECHOSTATUS CEchoGals::GetAudioPan
(
	PMIXER_FUNCTION	pMF
)
{
	WORD 			wPipe;
	WORD 			wBus;
	ECHOSTATUS 	Status;
	
	if ( 	(pMF->Channel.dwType != ECHO_PIPE_OUT) ||
			( !HasVmixer() ) )
		return ECHOSTATUS_INVALID_CHANNEL;
		
	wPipe = pMF->Channel.wChannel;
	wBus = pMF->Data.PipeOut.wBusOut;
	Status = m_PipeOutCtrl.GetPan(wPipe,
											wBus,
											pMF->Data.PipeOut.Data.iPan);

	return Status;

}	// ECHOSTATUS CEchoGals::GetAudioPan


//===========================================================================
//
//	Set the pan for an output pipe (virtual outputs only)
//
//===========================================================================

ECHOSTATUS CEchoGals::SetAudioPan
(
	PMIXER_FUNCTION	pMF
)
{
	WORD 			wPipe;
	WORD 			wBus;
	ECHOSTATUS 	Status;
	
	if ( 	(pMF->Channel.dwType != ECHO_PIPE_OUT) ||
			( !HasVmixer() ) )
		return ECHOSTATUS_INVALID_CHANNEL;
		
	wPipe = pMF->Channel.wChannel;
	wBus = pMF->Data.PipeOut.wBusOut;
	Status = m_PipeOutCtrl.SetPan(wPipe,
											wBus,
											pMF->Data.PipeOut.Data.iPan);

	return Status;
	
}	// ECHOSTATUS CEchoGals::SetAudioPan




/****************************************************************************

	CEchoGals clock control
	
	The input clock is the sync source - is the audio for this card running
	off of the internal clock, synced to word clock, etc.
	
	Output clock is only supported on Layla20 - Layla20 can transmit either
	word or super clock.

 ****************************************************************************/

//===========================================================================
//
// Get input and output clocks - just return the stored value
//
//===========================================================================

ECHOSTATUS CEchoGals::GetInputClock(WORD &wClock)
{
	if ( NULL == GetDspCommObject() )
		return ECHOSTATUS_DSP_DEAD;

	wClock = GetDspCommObject()->GetInputClock();
	
	return ECHOSTATUS_OK;
}	


ECHOSTATUS CEchoGals::GetOutputClock(WORD &wClock)
{
	if ( NULL == GetDspCommObject() )
		return ECHOSTATUS_DSP_DEAD;

	wClock = GetDspCommObject()->GetOutputClock();
	
	return ECHOSTATUS_OK;
}	


//===========================================================================
//
// Set input and output clocks - pass it down to the comm page and
// store the control change.
//
//===========================================================================

ECHOSTATUS CEchoGals::SetInputClock(WORD wClock)
{
	ECHOSTATUS Status;

	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;

	ECHO_DEBUGPRINTF( ("CEchoGals::SetInputClock: ") );
	
	Status = GetDspCommObject()->SetInputClock( wClock );

	if (ECHOSTATUS_OK == Status)
	{
		MixerControlChanged( ECHO_NO_CHANNEL_TYPE,
									MXN_INPUT_CLOCK);
	}
	return Status;

}	// SetInputClock


ECHOSTATUS CEchoGals::SetOutputClock(WORD wClock)
{
	ECHOSTATUS Status;

	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;

	ECHO_DEBUGPRINTF( ("CEchoGals::SetOutputClock: ") );
	
	Status = GetDspCommObject()->SetOutputClock( wClock );

	if (ECHOSTATUS_OK == Status)
	{
		MixerControlChanged( ECHO_NO_CHANNEL_TYPE,
									MXN_OUTPUT_CLOCK);
	}
	return Status;

}	


//===========================================================================
//
// Get the currently detected clock bits - default method.  Overridden by 
// derived card classes.
//
//===========================================================================

ECHOSTATUS CEchoGals::GetInputClockDetect(DWORD &dwClockDetectBits)
{
	dwClockDetectBits = ECHO_CLOCK_INTERNAL;

	return ECHOSTATUS_OK;
}	


//===========================================================================
//
// Set the locked sample rate
//
//===========================================================================

ECHOSTATUS CEchoGals::SetAudioLockedSampleRate
(
	DWORD		dwSampleRate
)
{
	ECHOSTATUS	Status;
							 
	Status = QueryAudioSampleRate( dwSampleRate );
	if ( ECHOSTATUS_OK != Status )
		return Status;

	if (0 != (ECHOGALS_FLAG_SAMPLE_RATE_LOCKED & GetFlags()))
	{
		GetDspCommObject()->SetSampleRate( dwSampleRate );
		m_dwSampleRate = dwSampleRate;
	}

	m_dwLockedSampleRate = dwSampleRate;

	return ECHOSTATUS_OK;

}	// ECHOSTATUS CEchoGals::SetAudioLockedSampleRate


//===========================================================================
//
// Get the locked sample rate
//
//===========================================================================

ECHOSTATUS CEchoGals::GetAudioLockedSampleRate
(
	DWORD	   &dwSampleRate
)
{
	dwSampleRate = m_dwLockedSampleRate;
		
	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS CEchoGals::GetAudioLockedSampleRate



#ifdef DIGITAL_INPUT_AUTO_MUTE_SUPPORT

//===========================================================================
//
// Get the digital input auto mute flag from the comm page
//
//===========================================================================

ECHOSTATUS CEchoGals::GetDigitalInAutoMute(PMIXER_FUNCTION pMixerFunction)
{
	BOOL fAutoMute;
	
	if (0 == (m_wFlags & ECHOGALS_ROFLAG_DIGITAL_IN_AUTOMUTE))
	{
		pMixerFunction->Data.fDigitalInAutoMute = FALSE;
		return ECHOSTATUS_NOT_SUPPORTED;
	}

	GetDspCommObject()->GetDigitalInputAutoMute(	fAutoMute );
	pMixerFunction->Data.fDigitalInAutoMute = fAutoMute;
	
	return ECHOSTATUS_OK;		
	
} // GetDigitalInAutoMute


//===========================================================================
//
// Set the digital input auto mute flag
//
//===========================================================================

ECHOSTATUS CEchoGals::SetDigitalInAutoMute(PMIXER_FUNCTION pMixerFunction)
{
	BOOL fAutoMute;
	
	if (0 == (m_wFlags & ECHOGALS_ROFLAG_DIGITAL_IN_AUTOMUTE))
		return ECHOSTATUS_NOT_SUPPORTED;
	
	fAutoMute = pMixerFunction->Data.fDigitalInAutoMute;
	GetDspCommObject()->SetDigitalInputAutoMute( fAutoMute );	
	
	return ECHOSTATUS_OK;		
	
} // SetDigitalInAutoMute

#endif // DIGITAL_INPUT_AUTO_MUTE_SUPPORT


//===========================================================================
//
// Get the gain for an output bus, input bus, or output pipe.
//
// Gain levels are in units of dB * 256.
//
//===========================================================================

ECHOSTATUS CEchoGals::GetAudioLineLevel
(
	PMIXER_FUNCTION	pMF
)
{
	WORD 			wPipe;
	WORD 			wBus;
	ECHOSTATUS 	Status;

	switch (pMF->Channel.dwType)
	{
		case ECHO_BUS_OUT :
			
			wBus = pMF->Channel.wChannel;
			
			if (wBus < GetNumBussesOut())
			{
				pMF->Data.iLevel = m_BusOutLineLevels[wBus].GetGain();				
				Status = ECHOSTATUS_OK;
			}
			else
			{
				Status = ECHOSTATUS_INVALID_CHANNEL;
			}

			break;
 
		case ECHO_BUS_IN :

			wBus = pMF->Channel.wChannel;
			if (wBus < GetNumBussesIn())
			{
				pMF->Data.iLevel = m_BusInLineLevels[wBus].GetGain();				
				Status = ECHOSTATUS_OK;
			}
			else
			{
				Status = ECHOSTATUS_INVALID_CHANNEL;
			}
			break;
		
		case ECHO_PIPE_OUT :
		
			wPipe = pMF->Channel.wChannel;
			wBus = pMF->Data.PipeOut.wBusOut;
 			Status = m_PipeOutCtrl.GetGain(	wPipe,
														wBus,
														pMF->Data.PipeOut.Data.iLevel);
			break;

		default:
			Status = ECHOSTATUS_INVALID_PARAM;
			break;
	}

	return Status;

}	// ECHOSTATUS CEchoGals::GetAudioLineLevel


//===========================================================================
//
// Utility function to check that a setting is within the correct range.
//
//===========================================================================

ECHOSTATUS CheckSetting(INT32 iValue,INT32 iMin,INT32 iMax)
{
	if ( (iValue > iMax) || (iValue < iMin))
		return ECHOSTATUS_INVALID_PARAM;
	
	return ECHOSTATUS_OK;	
		
}	// CheckSetting


//===========================================================================
//
// Set the gain for an output bus, input bus, or output pipe.
//
// Gain levels are in units of dB * 256.
//
//===========================================================================

ECHOSTATUS CEchoGals::SetAudioLineLevel
(
	PMIXER_FUNCTION	pMF
)
{
	WORD 			wPipe;
	WORD 			wBus;
	ECHOSTATUS 	Status;
	INT32 			iLevel;

	switch (pMF->Channel.dwType)
	{
		case ECHO_BUS_OUT :

			wBus = pMF->Channel.wChannel;
			iLevel = pMF->Data.iLevel;
			
			Status = CheckSetting(iLevel,ECHOGAIN_MINOUT,ECHOGAIN_MAXOUT);
			if (ECHOSTATUS_OK != Status)
				break;
			
			Status = m_BusOutLineLevels[wBus].SetGain(iLevel);
			break;
			
		case ECHO_BUS_IN :
		
			wBus = pMF->Channel.wChannel;
			iLevel = pMF->Data.iLevel;

			Status = CheckSetting(iLevel,ECHOGAIN_MININP,ECHOGAIN_MAXINP);
			if (ECHOSTATUS_OK != Status)
				break;

			Status = m_BusInLineLevels[wBus].SetGain(iLevel);
			break;
			
		case ECHO_PIPE_OUT :

			wPipe = pMF->Channel.wChannel;
			wBus = pMF->Data.PipeOut.wBusOut;
			iLevel = pMF->Data.PipeOut.Data.iLevel;

			Status = CheckSetting(iLevel,ECHOGAIN_MINOUT,ECHOGAIN_MAXOUT);
			if (ECHOSTATUS_OK != Status)
				break;

 			Status = m_PipeOutCtrl.SetGain(	wPipe,
														wBus,
														iLevel);
			break;
			
		default:
			Status = ECHOSTATUS_INVALID_PARAM;
			break;
	}

	return Status;
	
}	// ECHOSTATUS CEchoGals::SetAudioLineLevel


//===========================================================================
//
// Get the nominal level for an output or input bus.  The nominal level is
// also referred to as the +4/-10 switch.
//
//===========================================================================

ECHOSTATUS CEchoGals::GetAudioNominal
(
	PMIXER_FUNCTION	pMF
)
{
	BYTE					byNominal;
	ECHOSTATUS			Status;
	CDspCommObject *	pDspCommObj = GetDspCommObject();
	WORD					wCh;

	if ( NULL == pDspCommObj || pDspCommObj->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;
		
	switch (pMF->Channel.dwType)
	{
		case ECHO_BUS_OUT :
			wCh = pMF->Channel.wChannel;
			break;
			
		case ECHO_BUS_IN :
			wCh = pMF->Channel.wChannel + GetNumBussesOut();
			break;
		
		default :
			return ECHOSTATUS_INVALID_CHANNEL;
	}
		
	Status = pDspCommObj->GetNominalLevel( wCh, &byNominal );

	if ( ECHOSTATUS_OK != Status )
		return Status;

	pMF->Data.iNominal = ( byNominal ) ? -10 : 4;

	return ECHOSTATUS_OK;
}	// ECHOSTATUS CEchoGals::GetAudioNominal


//===========================================================================
//
// Set the nominal level for an output or input bus.  The nominal level is
// also referred to as the +4/-10 switch.
//
//===========================================================================

ECHOSTATUS CEchoGals::SetAudioNominal
(
	PMIXER_FUNCTION	pMF
)
{
	ECHOSTATUS	Status;
	WORD			wCh;
	INT32			iNominal;

	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;
	
	switch (pMF->Channel.dwType)
	{
		case ECHO_BUS_OUT :
			wCh = pMF->Channel.wChannel;
			break;
			
		case ECHO_BUS_IN :
			wCh = pMF->Channel.wChannel + GetNumBussesOut();
			break;
		
		default :
			return ECHOSTATUS_INVALID_CHANNEL;
	}

	iNominal = pMF->Data.iNominal;
	
	if ((iNominal!= -10) && (iNominal != 4))
		return ECHOSTATUS_INVALID_PARAM;
	
	Status =
		GetDspCommObject()->SetNominalLevel( wCh,
														( iNominal == -10 ) );

	if ( ECHOSTATUS_OK != Status )
		return Status;

	Status = MixerControlChanged( (WORD) pMF->Channel.dwType,
											MXN_NOMINAL,
											pMF->Channel.wChannel);
	return Status;

}	// ECHOSTATUS CEchoGals::SetAudioNominal


//===========================================================================
//
// Set the mute for an output bus, input bus, or output pipe.
//
//===========================================================================

ECHOSTATUS CEchoGals::SetAudioMute
(
		PMIXER_FUNCTION	pMF
)
{
	WORD 			wPipe;
	WORD 			wBus;
	ECHOSTATUS 	Status;
	BOOL			bMute;
	
	switch (pMF->Channel.dwType)
	{
		case ECHO_BUS_OUT :

			wBus = pMF->Channel.wChannel;
			bMute = pMF->Data.bMuteOn;
			Status = m_BusOutLineLevels[wBus].SetMute(bMute);
			break;
			
		case ECHO_BUS_IN :
	
			wBus = pMF->Channel.wChannel;
			bMute = pMF->Data.bMuteOn;
			Status = m_BusInLineLevels[wBus].SetMute(bMute);
			break;
			
		case ECHO_PIPE_OUT :

			wPipe = pMF->Channel.wChannel;
			wBus = pMF->Data.PipeOut.wBusOut;
			bMute = pMF->Data.PipeOut.Data.bMuteOn;
 			Status = m_PipeOutCtrl.SetMute(	wPipe,
														wBus,
														bMute);
			break;
			
		default:
			Status = ECHOSTATUS_INVALID_PARAM;
			break;
	}

	return Status;
}	// ECHOSTATUS CEchoGals::SetAudioMute


//===========================================================================
//
// Get the mute for an output bus, input bus, or output pipe.
//
//===========================================================================

ECHOSTATUS CEchoGals::GetAudioMute
(
		PMIXER_FUNCTION	pMF
)
{
	WORD 			wPipe;
	WORD 			wBus;
	ECHOSTATUS 	Status;

	switch (pMF->Channel.dwType)
	{
		case ECHO_BUS_OUT :
			
			wBus = pMF->Channel.wChannel;
			
			if (wBus < GetNumBussesOut())
			{
				pMF->Data.bMuteOn = m_BusOutLineLevels[wBus].IsMuteOn();
				Status = ECHOSTATUS_OK;
			}
			else
			{
				Status = ECHOSTATUS_INVALID_CHANNEL;
			}

			break;
 
		case ECHO_BUS_IN :

			wBus = pMF->Channel.wChannel;
			
			if (wBus < GetNumBussesIn())
			{
				pMF->Data.bMuteOn = m_BusInLineLevels[wBus].IsMuteOn();
				Status = ECHOSTATUS_OK;
			}
			else
			{
				Status = ECHOSTATUS_INVALID_CHANNEL;
			}
			break;
		
		case ECHO_PIPE_OUT :
		
			wPipe = pMF->Channel.wChannel;
			wBus = pMF->Data.PipeOut.wBusOut;
 			Status = m_PipeOutCtrl.GetMute(	wPipe,
														wBus,
														pMF->Data.PipeOut.Data.bMuteOn);
			break;

		default:
			Status = ECHOSTATUS_INVALID_PARAM;
			break;
	}

	return Status;

}	// ECHOSTATUS CEchoGals::GetAudioMute


//===========================================================================
//
// Get the monitor gain for a single input bus mixed to a single output bus.
//
//===========================================================================

ECHOSTATUS CEchoGals::GetAudioMonitor
(
	WORD	wBusIn,
	WORD	wBusOut,
	INT32 &	iGain
)
{
	if ( wBusIn  >= GetNumBussesIn() ||
		  wBusOut >= GetNumBussesOut() )
	{
		return ECHOSTATUS_INVALID_INDEX;
	}
	
	//
	// Get the monitor value
	//
	m_MonitorCtrl.GetGain(wBusIn,wBusOut,iGain);
	
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CEchoGals::GetAudioMonitor


//===========================================================================
//
// Set the monitor gain for a single input bus mixed to a single output bus.
//
//===========================================================================

ECHOSTATUS CEchoGals::SetAudioMonitor
(
	WORD	wBusIn,
	WORD	wBusOut,
	INT32	iGain
)
{
	ECHOSTATUS	Status;

	if ( wBusIn  >= GetNumBussesIn() ||
		  wBusOut >= GetNumBussesOut() )
	{
		return ECHOSTATUS_INVALID_INDEX;
	}
	
	Status = CheckSetting(iGain,ECHOGAIN_MINOUT,ECHOGAIN_MAXOUT);
	if (ECHOSTATUS_OK == Status)
	{
		//
		// Set the monitor gain
		// 
		m_MonitorCtrl.SetGain(wBusIn,wBusOut,iGain);
	}

	return Status;
	
}	// ECHOSTATUS CEchoGals::SetAudioMonitor


//===========================================================================
//
//	Helper functions for doing log conversions on pan values
//
// The parameter iNum is a fixed point 32 bit number in 16.16 format;
// that is, 16 bits of integer and 16 bits of decimal
// To convert a float number to fixed point, simply multiply by 2^16 and round
//
// Valid range for iNum is from +1.0 (0x10000) to 0.
//
//===========================================================================

#define FIXED_BASE		16							// 16 bits of fraction
#define FIXED_ONE_HALF	((INT32) 0x00008000)	// 0.5 in 16.16 format
#define COEFF_A2			((INT32) 0xffffa9ac)	// -.3372223
#define COEFF_A1			((INT32) 0x0000ff8a)	//  .9981958
#define COEFF_A0			((INT32) 0xffff5661)	// -.6626105

#define DB_CONVERT		0x60546		// 6.02... in 16.16

// Note use of double precision here to prevent overflow
static INT32 FixedMult( INT32 iNum1, INT32 iNum2 )
{
	LONGLONG llNum;

	llNum = (LONGLONG) iNum1 * (LONGLONG) iNum2;

	return (INT32) (llNum >> FIXED_BASE);
}	// INT32 FixedMult( INT32 iNum1, INT32 iNum2 )


static INT32 log2( INT32 iNum )
{
	INT32 iNumShifts;
	INT32 iTemp;

	// log2 is undefined for zero, so return -infinity (or close enough)
	if ( 0 == iNum )
		return ECHOGAIN_MUTED;

	// Step 1 - Normalize and save the number of shifts
	// Keep shifting iNum up until iNum > 0.5
	iNumShifts = 0;
	while ( iNum < FIXED_ONE_HALF )
	{
		iNumShifts++;
		iNum <<= 1;
	}

	// Step 2 - Calculate LOG2 by polynomial approximation.  8 bit accuracy.
	//
	// LOG2(x) = 4.0* (-.3372223 x*x + .9981958 x - .6626105)
	//                           a2             a1            a0
	// where  0.5 <= x < 1.0
	//

	// Compute polynomial sum
	iTemp = FixedMult( iNum, iNum );				// iTemp now has iNum squared
	iTemp = FixedMult( iTemp, COEFF_A2 );
	iTemp += FixedMult( iNum, COEFF_A1 );
	iTemp += COEFF_A0;

	// Multiply by four
	iTemp <<= 2;

	// Account for the normalize shifts
	iTemp -= ( iNumShifts << FIXED_BASE );

	return( iTemp );
}	// INT32 log2( INT32 iNum )


//
//	Convert pan value to Db X 256
//	Pan value is 0 - MAX_MIXER_PAN
//
INT32 PanToDb( INT32 iPan )
{
	if ( iPan >= ( MAX_MIXER_PAN - 1 ) )
		return( 0 );
	if ( iPan <= 1 )
		return( ECHOGAIN_MUTED );
	//
	//	Convert pan to 16.16
	//
	iPan = ( iPan << 16 ) / MAX_MIXER_PAN;
	//
	//	Take the log
	//
	iPan = log2( iPan );
	//
	// To convert to decibels*256, just multiply by the conversion factor
	//
	iPan = FixedMult( iPan << 8, DB_CONVERT );
	//
	// To round, add one half and then mask off the fractional bits
	//
	iPan = ( iPan + FIXED_ONE_HALF ) >> FIXED_BASE;
	return( iPan );
}	// INT32 PanToDb( INT32 iPan )


//===========================================================================
//
// Set the monitor pan
//
//	For this to work effectively, both the input and output channels must
//	both either be odd or even.  Thus even channel numbers are for the
//	left channel and odd channel numbers are for the right channel.
//	Pan values will be computed for both channels.
//
// iPan ranges from 0 (hard left) to MAX_MIXER_PAN (hard right)
//
//===========================================================================

ECHOSTATUS CEchoGals::SetAudioMonitorPan
(
	WORD	wBusIn,
	WORD	wBusOut,
	INT32	iPan						// New pan
)
{
	ECHOSTATUS Status;

	if ( wBusIn  >= GetNumBussesIn() ||
		  wBusOut >= GetNumBussesOut() )
	{
		return ECHOSTATUS_INVALID_INDEX;
	}
	
	Status = CheckSetting(iPan,0,MAX_MIXER_PAN);
	if (ECHOSTATUS_OK == Status)
	{
		//
		// Set the pan
		// 
		m_MonitorCtrl.SetPan(wBusIn,wBusOut,iPan);
	}

	return Status;
		
}	// ECHOSTATUS CEchoGals::SetAudioMonitorPan


//===========================================================================
//
// Get the monitor pan
//
//===========================================================================

ECHOSTATUS CEchoGals::GetAudioMonitorPan
(
	WORD	wBusIn,
	WORD	wBusOut,
	INT32 &	iPan						// Returns current pan (0 - MAX_MIXER_PAN)
)
{
	if ( wBusIn  >= GetNumBussesIn() ||
		  wBusOut >= GetNumBussesOut() )
	{
		return ECHOSTATUS_INVALID_INDEX;
	}
	
	//
	// Get the pan
	// 
	m_MonitorCtrl.GetPan(wBusIn,wBusOut,iPan);
	
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CEchoGals::GetAudioMonitorPan


//===========================================================================
//
// Set the monitor mute
//
//===========================================================================

ECHOSTATUS CEchoGals::SetAudioMonitorMute
(
	WORD	wBusIn,
	WORD	wBusOut,
	BOOL	bMute						// New state
)
{
	if ( wBusIn  >= GetNumBussesIn() ||
		  wBusOut >= GetNumBussesOut() )
	{
		return ECHOSTATUS_INVALID_INDEX;
	}
	
	//
	// Set the mute
	// 
	m_MonitorCtrl.SetMute(wBusIn,wBusOut,bMute);
		
	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS CEchoGals::SetAudioMonitorMute


//===========================================================================
//
// Get the monitor mute
//
//===========================================================================

ECHOSTATUS CEchoGals::GetAudioMonitorMute
(
	WORD	wBusIn,
	WORD	wBusOut,
	BOOL 	&bMute	  				// Returns current state
)
{
	if ( wBusIn  >= GetNumBussesIn() ||
		  wBusOut >= GetNumBussesOut() )
	{
		return ECHOSTATUS_INVALID_INDEX;
	}
	
	//
	// Get the mute
	// 
	m_MonitorCtrl.GetMute(wBusIn,wBusOut,bMute);
	
	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS CEchoGals::GetAudioMonitorMute


//===========================================================================
//
// Set the S/PDIF output format to professional or consumer
//
//===========================================================================

void CEchoGals::SetProfessionalSpdif( BOOL bNewStatus )
{
	ECHO_DEBUGPRINTF(("CEchoGals::SetProfessionalSpdif %d\n",bNewStatus));
	
	if ( NULL != GetDspCommObject() )
	{
		GetDspCommObject()->SetProfessionalSpdif( bNewStatus );
		MixerControlChanged( ECHO_NO_CHANNEL_TYPE,
									MXN_SPDIF );
	}
}	// void CEchoGals::SetProfessionalSpdif( BOOL bNewStatus )


//===========================================================================
//
// Set driver flags
//
//	See ECHOGALS_FLAG_??? definitions in EchoGalsXface.h
//
//===========================================================================

ECHOSTATUS CEchoGals::SetFlags
(
	WORD	wFlags
)
{
	//
	// Mask off the read-only flags so they don't change
	//
	wFlags &= ECHOGALS_FLAG_WRITABLE_MASK;

	//
	// Set the flags & mark the flags as changed
	//
	m_wFlags |= wFlags;

	MixerControlChanged(	ECHO_NO_CHANNEL_TYPE,
								MXN_FLAGS );

	return ECHOSTATUS_OK;

}	// ECHOSTATUS CEchoGals::SetFlags


//===========================================================================
//
// Clear driver flags
//
//	See ECHOGALS_FLAG_??? definitions in EchoGalsXface.h
//
//===========================================================================

ECHOSTATUS CEchoGals::ClearFlags
(
	WORD	wFlags
)
{
	//
	// Mask off the read-only flags so they don't change
	//
	wFlags &= ECHOGALS_FLAG_WRITABLE_MASK;

	//
	// Clear the flags & mark the flags as changed
	//
	m_wFlags &= ~wFlags;

	MixerControlChanged(	ECHO_NO_CHANNEL_TYPE,
								MXN_FLAGS );

	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS CEchoGals::ClearFlags


//===========================================================================
//
// Set the digital mode - currently for Gina24, Layla24, and Mona
//
//===========================================================================

ECHOSTATUS CEchoGals::SetDigitalMode
(
	BYTE byDigitalMode
)
{
	ECHOSTATUS	Status;
	BYTE			byPreviousDigitalMode;

	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;

	if ( 0 == GetDspCommObject()->GetDigitalModes() )
		return ECHOSTATUS_DIGITAL_MODE_NOT_SUPPORTED;

	if ( TRUE == GetDspCommObject()->IsTransportActive() ) 
	{
		ECHO_DEBUGPRINTF( ( 	"CEchoGals::SetDigitalMode()  Cannot set the digital "
									"mode while transport is running\n"));
		return ECHOSTATUS_BUSY;
	}
	byPreviousDigitalMode = GetDspCommObject()->GetDigitalMode();
	Status = GetDspCommObject()->SetDigitalMode( byDigitalMode );
	MixerControlChanged( ECHO_NO_CHANNEL_TYPE,
								MXN_DIGITAL_MODE );
	MixerControlChanged( ECHO_NO_CHANNEL_TYPE, 
								MXN_INPUT_CLOCK );

	//
	//	If we successfully changed the digital mode from or to ADAT, then 
	//	make sure all output, input and monitor levels are updated by the
	//	DSP comm object.
	//
	if ( ECHOSTATUS_OK == Status &&
		  byPreviousDigitalMode != byDigitalMode &&
		  ( DIGITAL_MODE_ADAT == byPreviousDigitalMode ||
		    DIGITAL_MODE_ADAT == byDigitalMode ) )
	{
		WORD	i, j,wBus,wPipe;

		for ( i = 0; i < GetNumBussesIn(); i++ )
		{
			for ( j = 0; j < GetNumBussesOut(); j += 2 )
			{
				m_MonitorCtrl.SetGain(i,j,ECHOGAIN_UPDATE,FALSE);
			}
		}

		for ( wBus = 0; wBus < GetNumBussesOut(); wBus++)
		{
			for ( wPipe = 0; wPipe < GetNumPipesOut(); wPipe++)
			{
				m_PipeOutCtrl.SetGain(wPipe,wBus,ECHOGAIN_UPDATE,FALSE);
			}
		}

		for ( i = 0; i < GetNumBussesOut(); i++ )
		{
			m_BusOutLineLevels[ i ].SetGain(ECHOGAIN_UPDATE,FALSE);
		}

		for ( i = 0; i < GetNumBussesIn(); i++ )
		{
			m_BusInLineLevels[ i ].SetGain( ECHOGAIN_UPDATE );
		}
		
		//
		// Now set them all at once, since all the
		// fImmediate parameters were set to FALSE.  Do the output
		// bus _and_ the output pipe in case this is a vmixer card.
		//
		m_BusOutLineLevels[0].SetGain(ECHOGAIN_UPDATE,TRUE);
		m_PipeOutCtrl.SetGain(0,0,ECHOGAIN_UPDATE,TRUE);
	}
	
	//
	// If the card has just been put in ADAT mode, it is possible
	// that the locked sample rate is greater than 48KHz, which is not allowed
	// in ADAT mode.  If this happens, change the locked rate to 48.
	//
	if ( 	(DIGITAL_MODE_ADAT == byDigitalMode) &&
			(m_wFlags & ECHOGALS_FLAG_SAMPLE_RATE_LOCKED) &&
			(m_dwLockedSampleRate > 48000) )
	{
		m_dwLockedSampleRate = 48000;
	}
	
	return Status;
	
}	// ECHOSTATUS CEchoGals::SetDigitalMode( ... )


/*

The output bus gain controls aren't actually implemented in the hardware;
insted they are virtual controls created by the generic code.

The signal sent to an output bus is a mix of the monitors and output pipes
routed to that bus; the output bus gain is therefore implemented by tweaking
each appropriate monitor and output pipe gain.

*/


//===========================================================================
//
// Adjust all the monitor levels for a particular output bus
//
// For efficiency, fImmediate is set to FALSE in the call
// to SetGain; all the monitor and pipe out gains are
// sent to the DSP at once by AdjustPipesOutForBusOut.
//
//===========================================================================

ECHOSTATUS CEchoGals::AdjustMonitorsForBusOut(WORD wBusOut)
{
	WORD wBusIn;
	
	//
	// Poke the monitors
	//
	for (wBusIn = 0; wBusIn < GetNumBussesIn(); wBusIn++)
	{
		m_MonitorCtrl.SetGain(wBusIn,wBusOut,ECHOGAIN_UPDATE,FALSE);
	}
	
	return ECHOSTATUS_OK;
	
}	// AdjustMonitorsForBusOut

	
//===========================================================================
//
// Adjust all the pipe out levels for a particular output bus
//
// For efficiency, fImmediate is set to FALSE in the call
// to SetGain; all the monitor and pipe out gains are
// sent to the DSP at once by AdjustPipesOutForBusOut.
//
//===========================================================================

ECHOSTATUS CEchoGals::AdjustPipesOutForBusOut(WORD wBusOut,INT32 iBusOutGain)
{
	ECHO_DEBUGPRINTF(("CEchoGals::AdjustPipesOutForBusOut wBusOut %d  iBusOutGain %ld\n",
							wBusOut,
							iBusOutGain));
					
	//
	// Round down to the nearest even bus
	//
	wBusOut &= 0xfffe;							
							
	m_PipeOutCtrl.SetGain(wBusOut,wBusOut,ECHOGAIN_UPDATE,FALSE);
	wBusOut++;
	m_PipeOutCtrl.SetGain(wBusOut,wBusOut,ECHOGAIN_UPDATE,TRUE);	
	
	return ECHOSTATUS_OK;
	
}	// AdjustPipesOutForBusOut



//===========================================================================
//
// GetAudioLatency - returns the latency for a single pipe
//
//===========================================================================

void CEchoGals::GetAudioLatency(ECHO_AUDIO_LATENCY *pLatency)
{
	DWORD dwLatency;

	if (FALSE == pLatency->wIsInput)
	{
		if (pLatency->wPipe >= GetFirstDigitalBusOut())
			dwLatency = m_wDigitalOutputLatency;
		else
			dwLatency = m_wAnalogOutputLatency;	
	}
	else
	{
		if (pLatency->wPipe >= GetFirstDigitalBusIn())
			dwLatency = m_wDigitalInputLatency;
		else
			dwLatency = m_wAnalogInputLatency;	
	}
	
	pLatency->dwLatency = dwLatency;

}	// GetAudioLatency
