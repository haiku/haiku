// ****************************************************************************
//
//		CMtcSync.cpp
//
//		Implementation of the CMtcSync class.
//		Used to sync to MIDI time code.
//
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

#include	"CEchoGals.h"
#include	"CMtcSync.h"


/****************************************************************************

	MTC frames per second lookup

 ****************************************************************************/
 
static DWORD dwMtcFpsLookup[] = 
{
	24,
	25,
	30,	// 30 frames per second, drop frame
	30
};


/****************************************************************************

	Construction, destruction and reset

 ****************************************************************************/

CMtcSync::CMtcSync( CEchoGals *pEG )
{
	m_pEG = pEG;
	
	pEG->GetAudioSampleRate( &m_dwBaseSampleRate );

	//
	//	Buffer init
	//
	m_dwFill = 0;
	m_dwDrain = 0;
	
	//
	// Reset the sync state info
	//
	Reset();
	
}	// CMtcSync::CMtcSync()


CMtcSync::~CMtcSync()
{
}	// CMtcSync::~CMtcSync()


void CMtcSync::Reset()
{
	ECHO_DEBUGPRINTF(("\t **** CMtcSync::Reset\n"));

	m_dwFramesPerSec = 30;

	m_iSamplesPerDframe = (m_dwBaseSampleRate / m_dwFramesPerSec) * 2;
	
	m_dwLastDframeTimestamp = 0;
	m_dwLastDframe = 0;
	m_dwCurrentDframe = 0;
	m_iNumDframesSynced = 0;
	m_iNumSamplesSynced = 0;
	m_dwNextQfType = 0;
	m_dwTemp = 0;

}	// void CMtcSync::Reset()


/****************************************************************************

	Methods for storing MTC data - called at interrupt time

 ****************************************************************************/

//===========================================================================
//
// Store the upper 16 bits of the sample timestamp
//
//===========================================================================
						  
void CMtcSync::StoreTimestampHigh(DWORD dwData)
{

	m_Buffer[ m_dwFill ].dwTimestamp = dwData << 16;
	
}	// StoreTimestampHigh


//===========================================================================
//
// Store the lower 16 bits of the sample timestamp
//
//===========================================================================
						  
void CMtcSync::StoreTimestampLow(DWORD dwData)
{

	m_Buffer[ m_dwFill ].dwTimestamp |= dwData;
	
}	// StoreTimestampLow


//===========================================================================
//
// Store the MTC data byte
//
//===========================================================================

void CMtcSync::StoreMtcData(DWORD dwData)
{
	//
	//	Store it
	// 
	m_Buffer[ m_dwFill ].dwData = dwData;
	
	//
	// Increment and wrap the pointer
	//
	m_dwFill++;
	m_dwFill &= ECHO_MTC_QUEUE_SZ - 1;
	
}	// StoreMtcData


/****************************************************************************

  Sync to MIDI time code

 ****************************************************************************/
 
void CMtcSync::Sync()
{
	BOOL fRateAdjusted;
	
	//------------------------------------------------------------------------
	//
	// Process each MTC_DATA in the buffer until the buffer is emtpy
	// or the sample rate has been adjusted
	//
	//------------------------------------------------------------------------
	
	fRateAdjusted = FALSE;
	while ( (FALSE == fRateAdjusted) && (m_dwFill != m_dwDrain) )
	{
		//---------------------------------------------------------------------
		//
		// Parse the quarter-frame message
		//
		//---------------------------------------------------------------------

		DWORD dwData,dwTimestamp,dwQfType,dwQfData;

		dwData = m_Buffer[ m_dwDrain ].dwData;		
		dwTimestamp = m_Buffer[ m_dwDrain ].dwTimestamp;
		
		ECHO_DEBUGPRINTF(("\n\tCMtcSync::Sync - data 0x%lx, timestamp 0x%lx\n",
								dwData,dwTimestamp));
		
		dwQfType = (dwData >> 4) & 0xf;
		dwQfData = dwData & 0xf;
		
		m_dwDrain++;
		m_dwDrain &= ECHO_MTC_QUEUE_SZ - 1;
		
		//
		// Make sure the QFs arrive in sequence
		//
		if (dwQfType != m_dwNextQfType)
		{
			ECHO_DEBUGPRINTF(("\tCMtcSync::Sync - quarter-frames out of sequence\n"));
			Reset();
			continue;			
		}
		
		//
		// Process this QF - accumulate the current dframe number
		//
		switch (dwQfType)
		{
			case MTC_QF_FRAME_LSN :
				m_dwTemp = dwQfData;
				
				ECHO_DEBUGPRINTF(("\t\tQF frame LSN %ld  m_dwTemp %ld  m_dwCurrentDframe %ld\n",
										dwQfData,m_dwTemp,m_dwCurrentDframe));
				
				break;
				
			case MTC_QF_FRAME_MSN :
				m_dwTemp |= dwQfData << 4;
				m_dwTemp &= 0x1f;
				m_dwCurrentDframe = m_dwTemp;
				
				ECHO_DEBUGPRINTF(("\t\tQF frame MSN %ld  m_dwTemp %ld  m_dwCurrentDframe %ld\n",
										dwQfData,m_dwTemp,m_dwCurrentDframe));
				break;
				
			case MTC_QF_SECOND_LSN :
				m_dwTemp = dwQfData;
				
				ECHO_DEBUGPRINTF(("\t\tQF second LSN %ld  m_dwTemp %ld  m_dwCurrentDframe %ld\n",
										dwQfData,m_dwTemp,m_dwCurrentDframe));				
				break;
				
			case MTC_QF_SECOND_MSN :
				m_dwTemp |= dwQfData << 4;
				m_dwTemp &= 0x3f;
				m_dwCurrentDframe += m_dwTemp * m_dwFramesPerSec;

				ECHO_DEBUGPRINTF(("\t\tQF second MSN %ld  m_dwTemp %ld  m_dwCurrentDframe %ld\n",
										dwQfData,m_dwTemp,m_dwCurrentDframe));
				break;
				
			case MTC_QF_MINUTE_LSN :
				m_dwTemp = dwQfData;

				ECHO_DEBUGPRINTF(("\t\tQF minute LSN %ld  m_dwTemp %ld  m_dwCurrentDframe %ld\n",
										dwQfData,m_dwTemp,m_dwCurrentDframe));				
				break;
				
			case MTC_QF_MINUTE_MSN :
				m_dwTemp |= dwQfData << 4;
				m_dwTemp &= 0x3f;
				m_dwCurrentDframe += m_dwTemp * m_dwFramesPerSec * 60;

				ECHO_DEBUGPRINTF(("\t\tQF minute MSN %ld  m_dwTemp %ld  m_dwCurrentDframe %ld\n",
										dwQfData,m_dwTemp,m_dwCurrentDframe));				
				break;
				
			case MTC_QF_HOUR_LSN :
				m_dwTemp = dwQfData;

				ECHO_DEBUGPRINTF(("\t\tQF hour LSN %ld  m_dwTemp %ld  m_dwCurrentDframe %ld\n",
										dwQfData,m_dwTemp,m_dwCurrentDframe));				
				break;
				
			case MTC_QF_HOUR_MSN :
				//
				//	Finish the dframe number
				// 
				m_dwTemp |= dwQfData << 4;
				m_dwTemp &= 0x1f;
				m_dwCurrentDframe += m_dwTemp * m_dwFramesPerSec * 3600;
				
				ECHO_DEBUGPRINTF(("\t\tQF hour MSN %ld  m_dwCurrentDframe %ld  m_dwCurrentDframe %ld\n",
										dwQfData,m_dwTemp,m_dwCurrentDframe));	
										
				//
				// Store the number of frames per second
				//
				DWORD dwFpsIndex,dwFps;
				
				dwFpsIndex = (dwQfData >> 1) & 3;
				dwFps = dwMtcFpsLookup[ dwFpsIndex ];
				m_dwFramesPerSec = dwFps;				
				m_iSamplesPerDframe = (m_dwBaseSampleRate / dwFps) * 2;				
				break;
				
			default :
				Reset();
				break;
			
		}
		
		//
		// If this is not the end of the dframe, go to the next MTC_DATA
		//
		if (MTC_QF_HOUR_MSN != dwQfType)
		{
			m_dwNextQfType = dwQfType + 1;
			continue;
		}

		//---------------------------------------------------------------------
		//
		// End of doubleframe reached; see if the card is synced or not
		//
		//---------------------------------------------------------------------
		
		INT32 iFrameDelta;
		
		ECHO_DEBUGPRINTF(("\n\n\t\t\tCMtcSync::Sync - doubleframe %ld\n",m_dwCurrentDframe));
			
			
		//
	   // Make sure the doubleframes are arriving in sequence
		//
	   // Even though we are checking that doubleframes are in sequence, with drop-frame
	   // you can sometimes have doubleframes that differ by three, so make sure the 
	   // frame diff is no more than 3
	   //
	   // Should probably check for drop-frame here and only allow diff of 3 for 
		// drop-frame, but who really cares?
		//
		iFrameDelta = (INT32) (m_dwCurrentDframe - m_dwLastDframe);
		if ((iFrameDelta > 3) || (iFrameDelta < 0))
		{
			DWORD dwDframe;
		
			ECHO_DEBUGPRINTF(("\t****** CMtcSync::Sync - double-frames out of sequence\n"));		
			
			dwDframe = m_dwCurrentDframe;
			Reset();			
			m_dwLastDframe = dwDframe;
			continue;	
		}
		
		//
		// See if the timestamps have wrapped around
		//
		if (m_dwLastDframeTimestamp > dwTimestamp)
		{
			ECHO_DEBUGPRINTF(("\tCMtcSync::Sync - timestamps have wrapped around"));		
			Reset();
			continue;
		}
		
		//
		// See if the sample rate needs to be adjusted
		//
		INT32 iTimestampDelta,iActualTotalSamples,iExpectedTotalSamples,iVariance;

		// Calculate the timestamp delta; limit the minimum and maximum		
		iTimestampDelta = (INT32) (dwTimestamp - m_dwLastDframeTimestamp);
		
		if (iTimestampDelta > ((INT32) (m_iSamplesPerDframe * 2)) )
			iTimestampDelta = m_iSamplesPerDframe * 2;
		else if (iTimestampDelta < ((INT32) (m_iSamplesPerDframe / 2)) )	
			iTimestampDelta = m_iSamplesPerDframe / 2;

		// How many sample times should have gone by?		
		iExpectedTotalSamples = (m_iNumDframesSynced + 1)	* m_iSamplesPerDframe;
		
		ECHO_DEBUGPRINTF(("\t\t\tdwTimestamp 0x%lx  m_dwLastDframeTimestamp 0x%lx\n",dwTimestamp,m_dwLastDframeTimestamp));
		ECHO_DEBUGPRINTF(("\t\t\tBase rate %ld   m_iSamplesPerDframe %ld  iTimestampDelta %ld\n",
								m_dwBaseSampleRate,m_iSamplesPerDframe,iTimestampDelta));
		
		// iVariance represents the difference between expected and actual elapsed
		// sample time										
		iActualTotalSamples = iTimestampDelta + m_iNumSamplesSynced;
		iVariance = iActualTotalSamples - iExpectedTotalSamples;
		if (iVariance < 0) iVariance = -iVariance;
		
		ECHO_DEBUGPRINTF(("\t\t\tiExpectedTotalSamples %ld   iVariance %ld\n",
								iExpectedTotalSamples,iVariance));
		
		if (iVariance > MTC_TOLERANCE)
		{
			DWORD dwSampleRate;
		
			//------------------------------------------------------------------
			//
			// The clock needs to be adjusted by this ratio
			//
			//           iExpectedTotalSamples
			//			-----------------------------
			//			     iActualTotalSamples
			//
			// This math is done in a fixed point fractional format, with 12 
			// bits after the decimal point
			//
			//------------------------------------------------------------------
			
			INT32 iRatio;
			
			iRatio = (iExpectedTotalSamples << 12) / iActualTotalSamples;
			
			//
			// To avoid wild swings in the sample rate, apply damping to the ratio; 
			// that is, only change by a small percentage of the ratio.  This is only
			// done if the ratio is not already small
			//
			if (	(iRatio < DAMPING_RATIO_LIMIT_LOW) || 
					(iRatio > DAMPING_RATIO_LIMIT_HIGH))
			{
				iRatio = (iRatio * (0x1000 - MTC_DAMPING)) >> 12;
				iRatio += MTC_DAMPING;
			}
			
			ECHO_DEBUGPRINTF(("\t\t\tiRatio 0x%08lx\n",iRatio));
			ECHO_DEBUGPRINTF(("\t\t\tiExpectedTotalSamples %ld  iActualTotalSamples %ld\n",
									iExpectedTotalSamples,iActualTotalSamples));
			ECHO_DEBUGPRINTF(("\t\t\tiTimestampDelta %ld   m_iSamplesPerDframe %ld\n",
									iTimestampDelta,m_iSamplesPerDframe));		
							

			//
			// Adjust the sample rate
			//
			dwSampleRate = m_pEG->GetDspCommObject()->GetSampleRate();
			dwSampleRate = (iRatio * dwSampleRate ) >> 12;
			
			// Limit for single or double speed mode
			if (m_dwBaseSampleRate > 50000)
			{
				if (dwSampleRate > 100000)
					dwSampleRate = 100000;
				else if (dwSampleRate < 50000)
					dwSampleRate = 50000;	
			}
			else
			{
				if (dwSampleRate > 50000)
					dwSampleRate = 50000;
				else if (dwSampleRate < MIN_MTC_1X_RATE)
					dwSampleRate = MIN_MTC_1X_RATE;	
			}
			
			ECHO_DEBUGPRINTF(("\t\t\tNew sample rate %ld\n",dwSampleRate));
	
			m_pEG->GetDspCommObject()->SetSampleRate ( dwSampleRate );	
			
			//
			// Reset the sync counts
			//			
			m_iNumDframesSynced = 0;
			m_iNumSamplesSynced = 0;
		}
		else
		{
			//------------------------------------------------------------------
			//
			// Sample rate is close enough
			//
			//------------------------------------------------------------------
			
			ECHO_DEBUGPRINTF(("\t\t\tClose enough - iVariance %ld\n",iVariance));
			
			m_iNumDframesSynced++;
			m_iNumSamplesSynced += iTimestampDelta;
			
			if (m_iNumDframesSynced > MAX_DFRAME_SYNC_COUNT)
				m_iNumDframesSynced >>= 1;
			
			ECHO_DEBUGPRINTF(("\t\t\tm_iNumDframesSynced %ld  m_iNumSamplesSynced %ld\n\n",
									m_iNumDframesSynced,m_iNumSamplesSynced));
		}

		m_dwLastDframe = m_dwCurrentDframe;
		m_dwLastDframeTimestamp = dwTimestamp;
		
		m_dwNextQfType = 0;
		
	}				
	
	
}	// Sync




/****************************************************************************

	New & delete

 ****************************************************************************/

PVOID CMtcSync::operator new( size_t Size )
{
	PVOID 		pMemory;
	ECHOSTATUS 	Status;
	
	Status = OsAllocateNonPaged(Size,&pMemory);
	
	if ( (ECHOSTATUS_OK != Status) || (NULL == pMemory ))
	{
		ECHO_DEBUGPRINTF(("CMtcSync::operator new - memory allocation failed\n"));

		pMemory = NULL;
	}
	else
	{
		memset( pMemory, 0, Size );
	}

	return pMemory;

}	// PVOID CMtcSync::operator new( size_t Size )


VOID  CMtcSync::operator delete( PVOID pVoid )
{
	if ( ECHOSTATUS_OK != OsFreeNonPaged( pVoid ) )
	{
		ECHO_DEBUGPRINTF( ("CMtcSync::operator delete memory free failed\n") );
	}
}	// VOID  CMtcSync::operator delete( PVOID pVoid )


// *** CMtcSync.cpp ***
