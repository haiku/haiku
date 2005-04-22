// ****************************************************************************
//
//		CMtcSync.h
//
//		Include file for interfacing with the CMtcSync class.
//		
//		This class manages syncing to MIDI time code.
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

//	Prevent problems with multiple includes
#ifndef _MTCQUEUEOBJECT_
#define _MTCQUEUEOBJECT_

typedef struct tMTC_DATA
{
	DWORD	dwData;			// DWORD here wastes memory, but is easier to debug...
	DWORD dwTimestamp;	// Timestamp in terms of the DSP sample count
} MTC_DATA;


//
//	CMtcSync
//
class CMtcSync
{
public:
	//
	//	Construction/destruction
	//
	CMtcSync( CEchoGals *pEG );
	~CMtcSync();
	
	//
	// Methods used to store MTC data & timestamps
	//	
	void StoreTimestampHigh(DWORD dwData);
	void StoreTimestampLow(DWORD dwData);
	void StoreMtcData(DWORD dwData);
	
	//
	// Call this to process the MTC data and adjust the sample rate
	//
	void Sync();
	
	//
	//	Reset the state of things
	//
	void Reset();
	
	//
	//  Overload new & delete so memory for this object is allocated from non-paged memory.
	//
	PVOID operator new( size_t Size );
	VOID  operator delete( PVOID pVoid ); 

protected:

	enum	// Quarter-frame message types
	{
		MTC_QF_FRAME_LSN = 0,
		MTC_QF_FRAME_MSN,
		MTC_QF_SECOND_LSN,
		MTC_QF_SECOND_MSN,
		MTC_QF_MINUTE_LSN,
		MTC_QF_MINUTE_MSN,
		MTC_QF_HOUR_LSN,
		MTC_QF_HOUR_MSN,
		
		MTC_TOLERANCE		= 64,
		MTC_DAMPING			= 98 * 0x1000 / 100,	// 95% in 12 bit fixed point
		
		MAX_DFRAME_SYNC_COUNT	= 4096,
		
		DAMPING_RATIO_LIMIT_LOW	 = 4096 - 16,
		DAMPING_RATIO_LIMIT_HIGH = 4096 + 16
	};

	//
	//	Midi buffer management
	//
	MTC_DATA 	m_Buffer[ ECHO_MTC_QUEUE_SZ ];
	DWORD			m_dwFill;
	DWORD			m_dwDrain;
	
	//
	// State information
	//
	DWORD			m_dwBaseSampleRate;
	
	DWORD			m_dwLastDframeTimestamp;	// Timestamp of last double frame 
														// in units of samples

	DWORD			m_iSamplesPerDframe;
	DWORD			m_dwFramesPerSec;
	
	DWORD			m_dwLastDframe;				// Last doubleframe number
	DWORD			m_dwCurrentDframe;			// Doubleframe number currently being assembled
														// from MTC data bytes
	
	DWORD			m_iNumDframesSynced;			// How many frames have elapsed since the
														// last sample rate adjustment
	
	INT32			m_iNumSamplesSynced;			// Sum of all actual timestamp deltas since
														// last sample rate adjustment
	DWORD			m_dwNextQfType;
	
	DWORD			m_dwTemp;
	
	//
	// Other stuff
	//
	CEchoGals 	*m_pEG;
	
	friend class CMidiInQ;
	
};		// class CMtcSync

typedef CMtcSync * PCMtcSync;

#endif

// *** CMtcSync.H ***
