// ****************************************************************************
//
//		CEchoGals_transport.cpp
//
//		Audio transport methods for the CEchoGals driver class.
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

#pragma optimize("",off)


/******************************************************************************

 Functions for opening and closing pipes

 ******************************************************************************/

//===========================================================================
//
// OpenAudio is used to reserve audio pipes for your exclusive use.  The call
// will fail if someone else has already opened the pipes.  Calling OpenAudio
// is the first step if you want to play or record.
//
// If the fCheckHardware flag is true, then the open will fail
// if the DSP and ASIC firmware have not been loaded (usually means
// your external box is turned off).
//
//===========================================================================

ECHOSTATUS CEchoGals::OpenAudio
(
	PECHOGALS_OPENAUDIOPARAMETERS	pOpenParameters,	// Info on pipe
	PWORD									pwPipeIndex,		// Pipe index ptr
	BOOL									fCheckHardware,
	CDaffyDuck							*pDuck
)
{
	CChannelMask	cmMask;
	WORD				wPipeMax, wPipe, wPipeIndex, i, wWidth;
	ECHOSTATUS 		Status;

	ECHO_DEBUGPRINTF( ("CEchoGals::OpenAudio: %s %u "
							 "PipeWidth %d "
							 "Cyclic %u \n",
							 ( pOpenParameters->Pipe.bIsInput ) ? "Input" : "Output",
								pOpenParameters->Pipe.nPipe,
								pOpenParameters->Pipe.wInterleave,
								pOpenParameters->bIsCyclic) );

	*pwPipeIndex = (WORD) -1;		// So it's never undefined

	//
	// Make sure the hardware is OK
	//
	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
	{
		ECHO_DEBUGPRINTF( ("\tECHOSTATUS_DSP_DEAD\n") );
		return ECHOSTATUS_DSP_DEAD;
	}

	//
	// Make sure the DSP & ASIC are up and running
	//	 - only if fCheckHardware is true
	//
	if (fCheckHardware)
	{
		Status = GetDspCommObject()->LoadFirmware();

		if ( ECHOSTATUS_OK != Status )
			return Status;
	}

	//
	// Validate the pipe number
	//
	wPipe = pOpenParameters->Pipe.nPipe;
	wWidth = pOpenParameters->Pipe.wInterleave;
	
	if ( pOpenParameters->Pipe.bIsInput )
	{
		wPipeIndex = wPipe + GetNumPipesOut();
		wPipeMax = GetNumPipesIn();
	}
	else
	{
		wPipeIndex = wPipe;
		wPipeMax = GetNumPipesOut();
	}
	
	if ( ( wPipe + wWidth ) > wPipeMax )
	{
		ECHO_DEBUGPRINTF( ("\tECHOSTATUS_INVALID_CHANNEL\n") );
		return ECHOSTATUS_INVALID_CHANNEL;
	}
	
	//
	// If the width is more than two, make sure that this card
	// can handle super interleave
	//
	if (	(0 == (m_wFlags & ECHOGALS_ROFLAG_SUPER_INTERLEAVE_OK)) &&
			(wWidth > 2)
		)
	{
		ECHO_DEBUGPRINTF( ("\tECHOSTATUS_NO_SUPER_INTERLEAVE\n") );
		return ECHOSTATUS_NO_SUPER_INTERLEAVE;
	}
	
	//
	// See if the specified pipes are already open
	//
	for ( i = 0; i < pOpenParameters->Pipe.wInterleave; i++ )
	{
		cmMask.SetIndexInMask( wPipeIndex + i );
	}
	
	if ( m_cmAudioOpen.Test( &cmMask ) )
	{
		ECHO_DEBUGPRINTF( ("OpenAudio - ECHOSTATUS_CHANNEL_ALREADY_OPEN - m_cmAudioOpen 0x%x   cmMask 0x%x\n",
							m_cmAudioOpen.GetMask(),cmMask.GetMask()) );
		return ECHOSTATUS_CHANNEL_ALREADY_OPEN;
	}

#ifdef AUTO_DUCK_ALLOCATE
	//
	// Make a daffy duck
	//
	if (NULL == pDuck)
	{
		pDuck = CDaffyDuck::MakeDaffyDuck(m_pOsSupport);

		if (NULL == pDuck)
			return ECHOSTATUS_NO_MEM;
	}

	SetDaffyDuck( wPipeIndex, pDuck );	
	
#else

	//
	// Use the specified duck if one was passed in
	//
	if (NULL != pDuck)
		SetDaffyDuck( wPipeIndex, pDuck );	
		
#endif
		
	
	//
	// Reset the 64-bit DMA position
	//	
	ResetDmaPos(wPipeIndex);
	GetDspCommObject()->ResetPipePosition(wPipeIndex);

	//
	// Prep stuff
	//
	m_cmAudioOpen += cmMask;
	if ( pOpenParameters->bIsCyclic )
		m_cmAudioCyclic += cmMask;
	m_Pipes[ wPipeIndex ] = pOpenParameters->Pipe;	
	*pwPipeIndex = wPipeIndex;
	m_ProcessId[ wPipeIndex ] = pOpenParameters->ProcessId;
	Reset( wPipeIndex );
	
	ECHO_DEBUGPRINTF( ("OpenAudio - ECHOSTATUS_OK - m_cmAudioOpen 0x%x\n",m_cmAudioOpen.GetMask()) );
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CEchoGals::OpenAudio


//===========================================================================
//
// CloseAudio is, naturally, the inverse of OpenAudio.
//
//===========================================================================

ECHOSTATUS CEchoGals::CloseAudio
(
	PECHOGALS_CLOSEAUDIOPARAMETERS	pCloseParameters,
	BOOL										fFreeDuck
)
{
	CChannelMask	cmMask;
	ECHOSTATUS		Status;
	WORD				i;
	WORD				wPipeIndex;
	
	wPipeIndex = pCloseParameters->wPipeIndex;

	ECHO_DEBUGPRINTF( ("CEchoGals::CloseAudio: Pipe %u  ",
							 wPipeIndex) );

	Status = VerifyAudioOpen( wPipeIndex );
	if ( ECHOSTATUS_OK != Status )
		return Status;

	for ( i = 0;
			i < m_Pipes[ wPipeIndex ].wInterleave;
			i++ )
	{
		cmMask.SetIndexInMask( wPipeIndex + i );
	}
	
	Reset( wPipeIndex );
	
	//
	// Free the scatter-gather list
	//
	if (NULL != m_DaffyDucks[wPipeIndex])
	{
		if (fFreeDuck)
			delete m_DaffyDucks[wPipeIndex];
	
		m_DaffyDucks[wPipeIndex] = NULL;
	}

	m_cmAudioOpen -= cmMask;
	m_cmAudioCyclic -= cmMask;

	m_ProcessId[ wPipeIndex ] = NULL;
	m_Pipes[ wPipeIndex ].wInterleave = 0;
	
	ECHO_DEBUGPRINTF( ("CloseAudio - ECHOSTATUS_OK - m_cmAudioOpen 0x%x\n",m_cmAudioOpen.GetMask()) );
	return ECHOSTATUS_OK;

}	// ECHOSTATUS CEchoGals::CloseAudio


//===========================================================================
//
// VerifyAudioOpen is a utility function; it tells you if
// a pipe is open or not.
//
//===========================================================================

ECHOSTATUS CEchoGals::VerifyAudioOpen
(
	WORD		wPipeIndex
)
{
	CChannelMask	cmMask;

	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
	{
		ECHO_DEBUGPRINTF( ("\tECHOSTATUS_DSP_DEAD\n") );
		return ECHOSTATUS_DSP_DEAD;
	}

	cmMask.SetIndexInMask( wPipeIndex );
	if ( !( m_cmAudioOpen.Test( &cmMask ) ) )
	{
		ECHO_DEBUGPRINTF( ("VerifyAudioOpen - ECHOSTATUS_CHANNEL_NOT_OPEN - wPipeIndex %d - m_cmAudioOpen 0x%x - cmMask 0x%x\n",
							wPipeIndex,m_cmAudioOpen.GetMask(),cmMask.GetMask()) );
		return ECHOSTATUS_CHANNEL_NOT_OPEN;
	}

	return ECHOSTATUS_OK;
	
}	// ECHOSTATUS CEchoGals::VerifyAudioOpen


//===========================================================================
//
// GetActivePipes tells you which pipes are currently active; that is, which 
// pipes are currently playing or recording.
//
//===========================================================================

ECHOSTATUS CEchoGals::GetActivePipes
(
	PCChannelMask	pChannelMask
)
{
	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;

	GetDspCommObject()->GetActivePipes( pChannelMask );
	return ECHOSTATUS_OK;
}	// void CEchoGals::GetActivePipes()


//===========================================================================
//
// Just like GetActivePipes, but this one tells you which pipes are currently
// open.
//
//===========================================================================

ECHOSTATUS CEchoGals::GetOpenPipes
(
	PCChannelMask	pChannelMask
)
{
	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;

	*pChannelMask = m_cmAudioOpen;
	return ECHOSTATUS_OK;

}	// void CEchoGals::GetOpenPipes()




/******************************************************************************

 Functions for setting audio formats and the sample rate

 ******************************************************************************/

//===========================================================================
//
// Validate an audio format.
//
// For comments on audio formats, refer to the definition of 
// ECHOGALS_AUDIOFORMAT.
// 
//===========================================================================

ECHOSTATUS CEchoGals::QueryAudioFormat
(
	WORD							wPipeIndex,
	PECHOGALS_AUDIOFORMAT	pAudioFormat
)
{
	ECHOSTATUS Status = ECHOSTATUS_OK;

	ECHO_DEBUGPRINTF( ("CEchoGals::QueryAudioFormat:\n") );
	
	//
	// If this pipe is open, make sure that this audio format
	// does not exceed the stored pipe width
	//
	WORD wInterleave = pAudioFormat->wDataInterleave;
	WORD wStoredPipeWidth = m_Pipes[ wPipeIndex ].wInterleave;

	if (0 != wStoredPipeWidth)
	{
		if (wInterleave > wStoredPipeWidth)
		{
			ECHO_DEBUGPRINTF(("CEchoGals::QueryAudioFormat - pipe was opened "
									"with a width of %d; interleave of %d invalid.\n",
									wStoredPipeWidth,
									pAudioFormat->wDataInterleave));
			return ECHOSTATUS_BAD_FORMAT;			
		}
	}
	
	//
	// Check for super interleave (i.e. interleave > 2)
	//
	if (wInterleave > 2)
	{
		//
		// Make sure the card is capable of super interleave
		//
		if (0 == (m_wFlags & ECHOGALS_ROFLAG_SUPER_INTERLEAVE_OK))
			return ECHOSTATUS_NO_SUPER_INTERLEAVE;
	
		//
		// Interleave must be even & data must be little endian
		//
		if (	(0 != pAudioFormat->byDataAreBigEndian) ||
			  	(0 != (wInterleave & 1)	)
			)
			return ECHOSTATUS_BAD_FORMAT;
		
		//
		// 16, 24, or 32 bit samples are required
		//	
		if ( 	(32 != pAudioFormat->wBitsPerSample) &&
				(24 != pAudioFormat->wBitsPerSample) &&
				(16 != pAudioFormat->wBitsPerSample) )
			return ECHOSTATUS_BAD_FORMAT;
		
					
		//
		// Make sure that this interleave factor on this pipe
		// does not exceed the number of pipes for the card
		//	
		WORD wMaxPipe;
		
		if (wPipeIndex >= GetNumPipesOut())
		{
			wMaxPipe = GetNumPipesIn();
			wPipeIndex = wPipeIndex - GetNumPipesOut();
		}
		else
		{
			wMaxPipe = GetNumPipesOut();
		}
		
		if ( (wPipeIndex + wInterleave) > wMaxPipe)
			return ECHOSTATUS_BAD_FORMAT;
		
		return ECHOSTATUS_OK;	
	}
	
	//
	// Check the interleave
	//
	if ( 	(1 != pAudioFormat->wDataInterleave) &&
			(2 != pAudioFormat->wDataInterleave) )

	{
		ECHO_DEBUGPRINTF(	("CEchoGals::QueryAudioFormat - interleave %d not allowed\n",
								pAudioFormat->wDataInterleave));
		return ECHOSTATUS_BAD_FORMAT;		
	}
	
	//
	//	If the big endian flag is set, the data must be mono or stereo interleave,
	// 32 bits wide, left justified data.  Only the upper 24 bits are used.
	//
	if (pAudioFormat->byDataAreBigEndian)
	{
		//
		// Must have 32 bits per sample
		//
		if (pAudioFormat->wBitsPerSample != 32)
		{
			ECHO_DEBUGPRINTF(("CEchoGals::QueryAudioFormat - Only 32 bits per"
									" sample supported for big-endian data\n"));
			return ECHOSTATUS_BAD_FORMAT;
		}
		
		//
		// Mono or stereo only
		//
		switch (pAudioFormat->wDataInterleave)
		{

#ifdef STEREO_BIG_ENDIAN32_SUPPORT

			case 1 :
			case 2 :
				break;
#else

			case 1 :
				break;

#endif				
			
			default :
				ECHO_DEBUGPRINTF(("CEchoGals::QueryAudioFormat - Interleave of %d"
										" not allowed for big-endian data\n",
										pAudioFormat->wDataInterleave));
				return ECHOSTATUS_BAD_FORMAT;
		}
		
		return ECHOSTATUS_OK;
	}
	
	//
	// Check bits per sample
	//
	switch ( pAudioFormat->wBitsPerSample )
	{
		case 8 :
		case 16 :
		case 24 :
		case 32 :
			break;
		
		default :
			ECHO_DEBUGPRINTF(
				("CEchoGals::QueryAudioFormat: No valid format "
				 "specified, bits per sample %d\n",
				 pAudioFormat->wBitsPerSample) );
			Status = ECHOSTATUS_BAD_FORMAT;
			break;
	}

	return Status;

}	// ECHOSTATUS CEchoGals::QueryAudioFormat


//===========================================================================
//
// SetAudioFormat sets the format of the audio data in host memory
// for this pipe.
//
//===========================================================================

ECHOSTATUS CEchoGals::SetAudioFormat
(
	WORD							wPipeIndex,
	PECHOGALS_AUDIOFORMAT	pAudioFormat
)
{
	ECHOSTATUS	Status;

	ECHO_DEBUGPRINTF( ("CEchoGals::SetAudioFormat: "
							 "for pipe %d\n",
							 wPipeIndex) );

	//
	// Make sure this pipe is open
	//
	Status = VerifyAudioOpen( wPipeIndex );
	if ( ECHOSTATUS_OK != Status )
		return Status;

	//
	// Check the format
	//
	Status = QueryAudioFormat( wPipeIndex, pAudioFormat );
	if ( ECHOSTATUS_OK != Status )
		return Status;
	
	//
	// Set the format
	//
	Status = GetDspCommObject()->SetAudioFormat( wPipeIndex, pAudioFormat );
	
	return Status;
	
}	// ECHOSTATUS CEchoGals::SetAudioFormat - single pipe


//===========================================================================
//
// This call lets you set the audio format for several pipes at once.
//
//===========================================================================

ECHOSTATUS CEchoGals::SetAudioFormat
(
	PCChannelMask				pChannelMask,
	PECHOGALS_AUDIOFORMAT	pAudioFormat
)
{
	WORD			wPipeIndex = 0xffff;
	ECHOSTATUS	Status;

	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
	{
		ECHO_DEBUGPRINTF( ("\tECHOSTATUS_DSP_DEAD\n") );
		return ECHOSTATUS_DSP_DEAD;
	}

	for ( ; ; )
	{
		wPipeIndex = pChannelMask->GetIndexFromMask( ++wPipeIndex );
		if ( (WORD) ECHO_INVALID_CHANNEL == wPipeIndex )
			break;							// We be done!

		//
		// See if this pipe is open
		//
		if ( !( m_cmAudioOpen.TestIndexInMask( wPipeIndex ) ) )
		{
			ECHO_DEBUGPRINTF( ("CEchoGals::SetAudioFormat: "
									 "for pipe %d failed, pipe not open\n",
									 wPipeIndex) );
			return ECHOSTATUS_CHANNEL_NOT_OPEN;
		}

		//
		// See if the format is OK
		//
		ECHO_DEBUGPRINTF( ("CEchoGals::SetAudioFormat: "
								 "for pipe %d\n",
								 wPipeIndex) );
		Status = QueryAudioFormat( wPipeIndex, pAudioFormat );
		if ( ECHOSTATUS_OK != Status )
			return Status;

		//
		// Set the format for this pipe
		//
		Status = GetDspCommObject()->SetAudioFormat( wPipeIndex, pAudioFormat );
		if ( ECHOSTATUS_OK != Status )
			return Status;
	}

	return ECHOSTATUS_OK;

}	// ECHOSTATUS CEchoGals::SetAudioFormat - multiple pipes



//===========================================================================
//
// GetAudioFormat returns the current audio format for a pipe.
//
//===========================================================================

ECHOSTATUS CEchoGals::GetAudioFormat
(
	WORD							wPipeIndex,
	PECHOGALS_AUDIOFORMAT	pAudioFormat
)
{
	ECHO_DEBUGPRINTF( ("CEchoGals::GetAudioFormat: "
							 "for pipe %d\n",
							 wPipeIndex) );

	GetDspCommObject()->GetAudioFormat( wPipeIndex, pAudioFormat );

	return ECHOSTATUS_OK;

}	// ECHOSTATUS CEchoGals::GetAudioFormat


//===========================================================================
//
// This function does exactly what you think it does.
//
// Note that if the card is not set to internal clock (that is, the hardware
// is synced to word clock or some such), this call has no effect.
//
// Note that all of the inputs and outputs on a single card share the same 
// clock.
//
//===========================================================================

ECHOSTATUS CEchoGals::SetAudioSampleRate
(
	DWORD		dwSampleRate
)
{
	ECHOSTATUS	Status;

	ECHO_DEBUGPRINTF( ("CEchoGals::SetAudioSampleRate: "
							 "to %ld Hz\n",
							 dwSampleRate) );
						
	//
	// Check to see if the sample rate is locked
	//
	if ( 0 != (m_wFlags & ECHOGALS_FLAG_SAMPLE_RATE_LOCKED))
	{
			return ECHOSTATUS_OK;
	}
	else
	{
		Status = QueryAudioSampleRate( dwSampleRate );
		if ( ECHOSTATUS_OK != Status )
			return Status;

		if ( dwSampleRate == GetDspCommObject()->SetSampleRate( dwSampleRate ) )
		{
			m_dwSampleRate = dwSampleRate;
			return ECHOSTATUS_OK;
		}
	}
	return ECHOSTATUS_BAD_FORMAT;
	
}	// ECHOSTATUS CEchoGals::SetAudioSampleRate


//===========================================================================
//
// GetAudioSampleRate - retrieves the current sample rate for the hardware
//
//===========================================================================

ECHOSTATUS CEchoGals::GetAudioSampleRate
(
	PDWORD	pdwSampleRate
)
{
	ECHO_DEBUGPRINTF( ("CEchoGals::GetAudioSampleRate\n"));

	*pdwSampleRate = m_dwSampleRate;

	return ECHOSTATUS_OK;

}	// ECHOSTATUS CEchoGals::GetAudioSampleRate




/******************************************************************************

 Functions related to the scatter-gather list

 ******************************************************************************/


//===========================================================================
//
// Use the given CDaffyDuck object as the scatter-gather list for this pipe
//
//===========================================================================

ECHOSTATUS CEchoGals::SetDaffyDuck(WORD wPipeIndex, CDaffyDuck *pDuck)	
{
	m_DaffyDucks[wPipeIndex] = pDuck;
	
	return ECHOSTATUS_OK;
							
}	// SetDaffyDuck




//===========================================================================
//
// This method returns a pointer to the daffy duck for a pipe; the caller
// can then have direct access to the daffy duck object.
//
//===========================================================================

CDaffyDuck *CEchoGals::GetDaffyDuck(WORD wPipeIndex)
{
	ECHO_DEBUGPRINTF(("CEchoGals::GetDaffyDuck for pipe index %d\n",wPipeIndex));

	if (wPipeIndex >= GetNumPipes())
		return NULL;
	
	return m_DaffyDucks[wPipeIndex];
}	



/******************************************************************************

 Functions for starting and stopping transport

 ******************************************************************************/

//===========================================================================
//
//	Start transport for a single pipe
//
//===========================================================================

ECHOSTATUS CEchoGals::Start
(
	WORD	wPipeIndex
)
{
	CChannelMask	cmMask;

	cmMask.SetIndexInMask( wPipeIndex );
	return Start( &cmMask );

}	// ECHOSTATUS CEchoGals::Start


//===========================================================================
//
//	Start transport for a group of pipes
//
// This function includes logic to sync-start several pipes at once,
// according to the process ID specified when the pipe was opened.  This is
// included to work around a limitation of the Windows wave API so that
// programs could use multiple inputs and outputs and have them start at the
// same time.  
//
// If you don't want to use this feature, call CEchoGals::ClearFlags 
// with ECHOGALS_FLAG_SYNCH_WAVE and the pipes will start immediately.
//
//===========================================================================

ECHOSTATUS CEchoGals::Start
(
	PCChannelMask	pChannelMask
)
{
	WORD				wPipe;
	DWORD				dwPhysStartAddr;
	CChannelMask	cmStart;
	PVOID				ProcessId = NULL;
	CDspCommObject *pDCO;
	
	pDCO = GetDspCommObject();
	if ( NULL == pDCO || pDCO->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;
	
	//
	//	See if we are dealing with synchronized wave pipes.  If the sync
	// flag is set, get the process ID for this pipe to compare with
	// other pipes.
	//
	if ( GetFlags() & ECHOGALS_FLAG_SYNCH_WAVE )
	{
		wPipe = pChannelMask->GetIndexFromMask( 0 );
		ProcessId = m_ProcessId[ wPipe ];
	}
	
	//--------------------------------------------------------
	// Process each pipe in the mask
	//--------------------------------------------------------

	for (wPipe = 0; wPipe < GetNumPipes(); wPipe++)
	{
		PDWORD pdwDspCommPositions;

		//
		// Skip this pipe if it's not in the mask
		//
		if (!pChannelMask->TestIndexInMask(wPipe))
			continue;
		
		//
		// This pipe must have a CDaffyDuck object
		//	
		if (NULL == m_DaffyDucks[ wPipe ])
		{
			ECHO_DEBUGPRINTF(("CDaffyDuck::Start - trying to start pipe index %d "
									"but there is no CDaffyDuck!\n",wPipe));
			return ECHOSTATUS_CHANNEL_NOT_OPEN;		
		}
		
		//
		// If this pipe was opened in cyclic mode, make sure that the corresponding
		// CDaffyDuck has been wrapped
		//	
		if (	(0 != m_cmAudioCyclic.TestIndexInMask( wPipe ) ) &&
				(FALSE == m_DaffyDucks[wPipe]->Wrapped()) 
			)
		{
			ECHO_DEBUGPRINTF(("CEchoGals::Start called for pipe index %d - "
									"pipe was opened in cyclic mode, but the duck "
									"has not been wrapped\n",wPipe));
			return ECHOSTATUS_DUCK_NOT_WRAPPED;
		}

		//
		// Set the physical start address for the duck for this pipe
		//
		dwPhysStartAddr = m_DaffyDucks[wPipe]->GetPhysStartAddr();
		pDCO->SetAudioDuckListPhys( wPipe, dwPhysStartAddr );		
		
				
		//
		// Do different things to this pipe depending on the 
		// state
		//	
		switch (m_byPipeState[wPipe])
		{
			case PIPE_STATE_RESET :
				//
				// Clean up the DMA position stuff
				//
				pdwDspCommPositions = pDCO->GetAudioPositionPtr();
				pdwDspCommPositions[ wPipe ] = 0;
				
				//
				// If this pipe isn't synced or is in a reset state,
				// start it up
				//
				if (NULL == ProcessId)
				{
					m_byPipeState[ wPipe ] = PIPE_STATE_STARTED;
					cmStart.SetIndexInMask( wPipe );
				}
				else
				{
					//
					// This pipe is synced; upgrade to PENDING
					//
					m_byPipeState[ wPipe ] = PIPE_STATE_PENDING;
				}
				break;

				
			case PIPE_STATE_STOPPED :

				if (NULL == ProcessId)
				{
					//
					// Non-synced pipe; start 'er up!
					//
					m_byPipeState[ wPipe ] = PIPE_STATE_STARTED;
					cmStart.SetIndexInMask( wPipe );
				}
				else
				{
					//
					// Synced pipe; if this pipe is in STOP mode, 
					// upgrade it to PENDING status
					//
					m_byPipeState[ wPipe ] = PIPE_STATE_PENDING;
				}
				break;

				
			case PIPE_STATE_PENDING :							
			case PIPE_STATE_STARTED :
				break;
		}
	}
		
	//-----------------------------------------------------------------
	// Start the pipes
	//-----------------------------------------------------------------

	//
	// Don't go if all the synced pipes are not yet pending
	//
	BOOL	fAllReady = TRUE;
	for ( wPipe = 0; wPipe < GetNumPipes(); wPipe++ )
	{
		if ( 	( ProcessId == m_ProcessId[ wPipe ] ) &&
				( PIPE_STATE_STOPPED == m_byPipeState[wPipe]))
		{
			ECHO_DEBUGPRINTF(("CEchoGals::Start - can't start; pipe %d "
									"still set to state %d\n",
									wPipe,
									m_byPipeState[wPipe]));
			fAllReady = FALSE;
		}
	}
	
	//
	// All synced pipes are pending; time to go!
	//
	if (fAllReady)
	{
		for (wPipe = 0; wPipe < GetNumPipes(); wPipe++)
		{
			if ( 	(ProcessId == m_ProcessId[ wPipe ]) &&
					(PIPE_STATE_PENDING == m_byPipeState[ wPipe ]))
			{
				m_byPipeState[wPipe] = PIPE_STATE_STARTED;
				cmStart.SetIndexInMask( wPipe );
				ECHO_DEBUGPRINTF(("CEchoGals::Start - setting pipe %d to start\n",
										wPipe));
			}
		}
	}

	if ( cmStart.IsEmpty() )
		return ECHOSTATUS_OK;


	//-----------------------------------------------------------------
	// Time to go
	//-----------------------------------------------------------------
	
	return pDCO->StartTransport( &cmStart );
	
}	// ECHOSTATUS CEchoGals::Start



//===========================================================================
//
// Stop a single pipe
//
//===========================================================================

ECHOSTATUS CEchoGals::Stop
(
	WORD	wPipeIndex
)
{
	CChannelMask	cmMask;

	cmMask.SetIndexInMask( wPipeIndex );
	return( Stop( &cmMask ) );
	
}	// ECHOSTATUS CEchoGals::Stop


//===========================================================================
//
// Stop several pipes simultaneously
//
//===========================================================================

ECHOSTATUS CEchoGals::Stop
(
	PCChannelMask	pChannelMask
)
{
	INT32			i;
	ECHOSTATUS	Status;

	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;

	Status = GetDspCommObject()->StopTransport( pChannelMask );
	if ( ECHOSTATUS_OK != Status )
		return Status;

	for ( i = 0; i < GetNumPipes(); i++ )
	{									
		//
		//	Skip channel if not in mask
		//
		if ( !pChannelMask->TestIndexInMask( (WORD) i ) )
			continue;


		//
		// Don't bother if it's stopped already
		//
		if ( PIPE_STATE_STOPPED == m_byPipeState[ i ] )
			continue;

		//
		// Muck with the DMA position
		//
		UpdateDmaPos( (WORD) i );
		
		m_byPipeState[ i ] = PIPE_STATE_STOPPED;
	}

	return Status;

}	// ECHOSTATUS CEchoGals::Stop


//===========================================================================
//
// Reset transport for a single pipe
//
//===========================================================================

ECHOSTATUS CEchoGals::Reset
(
	WORD	wPipeIndex
)
{
	CChannelMask	cmMask;

	cmMask.SetIndexInMask( wPipeIndex );
	return Reset( &cmMask );
	
}	// ECHOSTATUS CEchoGals::Reset


//===========================================================================
//
// Reset transport for a group of pipes simultaneously
//
//===========================================================================

ECHOSTATUS CEchoGals::Reset
(
	PCChannelMask	pChannelMask
)
{
	WORD			i;
	ECHOSTATUS	Status;

	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;

	Status = GetDspCommObject()->ResetTransport( pChannelMask );
	if ( ECHOSTATUS_OK != Status )
		return Status;

	for ( i = 0; i < GetNumPipes(); i++ )
	{
		//
		//	Skip channel if not in mask
		//
		if ( !pChannelMask->TestIndexInMask( (WORD) i ) )
			continue;

		if ( PIPE_STATE_RESET == m_byPipeState[ i ] )
			continue;

		//
		// Muck with the DMA position
		//
		UpdateDmaPos( i );
		m_dwLastDspPos[ i ] = 0;
		GetDspCommObject()->ResetPipePosition(i);
		
		m_byPipeState[ i ] = PIPE_STATE_RESET;
	}
	
	return Status;

}	// ECHOSTATUS CEchoGals::Reset




/******************************************************************************

 Functions for handling the current DMA position for pipes; the DMA position
 is the number of bytes transported by a pipe.

 ******************************************************************************/

//===========================================================================
//
// The DSP sends back a 32 bit DMA counter for each pipe of the number of bytes 
// transported; this count is written by the DSP to the comm page without
// the driver doing anything.
//
// The driver then maintains a 64 bit counter based off of the DSP's counter.
//
// Call UpdateDmaPos to cause the driver to update the internal 64 bit DMA
// counter.
//
// The 64 bit DMA counter is in units of bytes, not samples.
//
//===========================================================================

void CEchoGals::UpdateDmaPos( WORD wPipeIndex )
{
	DWORD dwDspPos;
	DWORD dwDelta;
	
	//
	// Get the current DSP position and find out how much it
	// has moved since last time.  This is necessary to avoid
	// the 32 bit counter wrapping around.
	//
	dwDspPos = GetDspCommObject()->GetAudioPosition( wPipeIndex );
	dwDelta = dwDspPos - m_dwLastDspPos[ wPipeIndex ];
	
	//
	// Adjust the 64 bit position
	//
	m_ullDmaPos[ wPipeIndex ] += dwDelta;
	m_dwLastDspPos[ wPipeIndex ] = dwDspPos;

} // UpdateDmaPos


//===========================================================================
//
// ResetDmaPos resets the 64 bit DMA counter.
//
//===========================================================================

void CEchoGals::ResetDmaPos(WORD wPipe)
{
	m_ullDmaPos[ wPipe ] = 0;
	m_dwLastDspPos[ wPipe ] = 0;

	//
	// There may still be mappings in the daffy duck; if so,
	// tell them to reset their DMA positions starting at zero
	//
	if (NULL != m_DaffyDucks[wPipe])
		m_DaffyDucks[wPipe]->ResetStartPos();
}
 

//===========================================================================
//
// This is a very powerful feature; calling this function gives you a pointer
// to the memory location where the DSP writes the 32 bit DMA position for
// a pipe.  The DSP is constantly updating this value as it moves data.
//
// Since the DSP is constantly updating it, you can dereference this pointer
// from anywhere and read the DMA position without calling the generic driver.
// This means that with some adroit mapping, you could read the DMA position
// from user mode without calling the kernel.
//
// Remember, Peter - with great power comes great responsibility; you should 
// only read this pointer and never write to it or to anywhere around it.  You 
// could easily lock up your computer.
//
// Note that the DSP writes the position in little endian format; if you are
// on a big endian machine, you will need to byte swap the postion
// before you use it.
//
//===========================================================================

ECHOSTATUS CEchoGals::GetAudioPositionPtr
(
	WORD		wPipeIndex,
	PDWORD &	pdwPosition
)
{
	if ( NULL == GetDspCommObject() || GetDspCommObject()->IsBoardBad() )
		return ECHOSTATUS_DSP_DEAD;

	if (wPipeIndex >= GetNumPipes())
	{
		pdwPosition = NULL;
		return ECHOSTATUS_INVALID_CHANNEL;
	}
		
	PDWORD	pdwDspCommPos = GetDspCommObject()->GetAudioPositionPtr();

	pdwPosition = pdwDspCommPos + wPipeIndex;

	return ECHOSTATUS_OK;

}	// ECHOSTATUS CEchoGals::GetAudioPositionPtr


