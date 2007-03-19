// ****************************************************************************
//
//		CEchoGals.H
//
//		Include file for the CEchoGals generic driver class.
//		Set editor tabs to 3 for your viewing pleasure.
//
// 		CEchoGals is the big daddy class of the generic code.  It is the upper
//		edge of the generic code - that is, it is the interface between the
// 		operating system-specific code and the generic driver.
//
//		There are a number of terms in this file that won't make any sense unless
// 		you go read EchoGalsXface.h first.
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
#ifndef _ECHOGALSOBJECT_
#define _ECHOGALSOBJECT_

#ifdef _DEBUG
#ifdef ECHO_WDM
#pragma optimize("",off)
#endif
#endif


//
//	Each project creates this file to support the OS it is targeted for
//
#include "OsSupport.h"

//
// Include the definitions for the card family
//
#include "family.h"


//
//	Interface definitions
//
#include "EchoGalsXface.h"
#include "MixerXface.h"
#include "CLineLevel.h"
#include "CPipeOutCtrl.h"
#include "CMonitorCtrl.h"
#include "CEchoGals_mixer.h"
#include "CChannelMask.h"
#include "CDspCommObject.h"
#include "CMidiInQ.h"

//
//	Pipe states
//
#define	PIPE_STATE_RESET			0	// Pipe has been reset
#define	PIPE_STATE_STOPPED		1	// Pipe has been stopped
#define	PIPE_STATE_STARTED		2	// Pipe has been started
#define	PIPE_STATE_PENDING		3	// Pipe has pending start



//
// Prototypes to make the Mac compiler happy
//
ECHOSTATUS CheckSetting(INT32 iValue,INT32 iMin,INT32 iMax);
INT32 PanToDb( INT32 iPan );


//
//	Base class used for interfacing with the audio card.
//
class CEchoGals
{
public:

	//***********************************************************************
	//
	//	Initialization (public)
	//
	//***********************************************************************	

	//
	//	Overload new & delete so memory for this object is allocated
	//	from non-paged memory.
	//
	PVOID operator new( size_t Size );
	VOID  operator delete( PVOID pVoid ); 

	//
	// Constructor and destructor
	//
	// Note that you must call AssignResources and InitHw 
	// before you can do anything useful.
	//
	CEchoGals( PCOsSupport pOsSupport );
	virtual ~CEchoGals();

	//
	//	Validate and save resources assigned by PNP.
	//	Each card uses one IRQ vector and accesses the DSP through
	//	shared memory
	//
	virtual ECHOSTATUS AssignResources
	(
		PVOID		pvSharedMemory,		// Virtual pointer to DSP registers
		PCHAR		pszCardName				// Caller gets from registry
    );

	//
	//	Initialize the hardware using data from AssignResources,
	//	create the CDspCommObject class, return status.
	//
	virtual ECHOSTATUS InitHw();



	//***********************************************************************
	//
	//	PCI card information (public)
	//
	//***********************************************************************	

	//
	//	Return the capabilities of this card; card type, card name,
	//	# analog inputs, # analog outputs, # digital channels,
	//	# MIDI ports and supported clocks.
	//	See ECHOGALS_CAPS definition above.
	//
	virtual ECHOSTATUS GetCapabilities
	(
		PECHOGALS_CAPS	pCapabilities
	)
	{ return GetBaseCapabilities(pCapabilities); }
	
	
	CONST PCHAR GetDeviceName();

	WORD MakePipeIndex(WORD wPipe,BOOL fInput);
	
	

	//***********************************************************************
	//
	//	Mixer interface (public) - used to set volume levels and other
	// miscellaneous stuff.
	//
	//***********************************************************************	
	
	//
	// Open and close the mixer
	//
	ECHOSTATUS OpenMixer(NUINT &Cookie);
	ECHOSTATUS CloseMixer(NUINT Cookie);
	
	//
	// Is the mixer open by one or more clients?
	//
	BOOL IsMixerOpen();

	//
	//	Process mixer functions
	//
	virtual ECHOSTATUS ProcessMixerFunction
	(
		PMIXER_FUNCTION	pMixerFunction,	// Request from mixer
		INT32 &				iRtnDataSz			// # bytes returned (if any)
	);

	//
	//	Process multiple mixer functions
	//
	virtual ECHOSTATUS ProcessMixerMultiFunction
	(
		PMIXER_MULTI_FUNCTION	pMixerFunctions,	// Request from mixer
		INT32 &						iRtnDataSz			// # bytes returned (if any)
	);

	//
	// Get all the control notifies since last time this was called
	// for this client.  dwNumNotifies is the size of the array passed in 
	// pNotifies; dwNumReturned specifies how many were actually copied in.
	//
	// Takes the cookie as a separate parameter; the one in MIXER_MULTI_NOTIFY
	// is ignored since it's not 64 bits
	//
	ECHOSTATUS GetControlChanges
	(
		PMIXER_MULTI_NOTIFY	pNotifies,
		NUINT MixerCookie
	);

	//
	// Get a bunch of useful polled stuff- audio meters,
	// clock detect bits, and pending notifies
	//	
	ECHOSTATUS GetPolledStuff
	(
		ECHO_POLLED_STUFF *pPolledStuff,
		NUINT MixerCookie
	);

	//
	// Get the digital mode
	//
	virtual BYTE GetDigitalMode()
		{ return( ( NULL == GetDspCommObject() )
						? (BYTE) DIGITAL_MODE_SPDIF_RCA
						: GetDspCommObject()->GetDigitalMode() ); }

	//
	//	Get, set and clear CEchoGals flags.
	//	See ECHOGALS_FLAG_??? definitions in EchoGalsXface.h
	//
	virtual WORD GetFlags()
	{ 
		return m_wFlags; 
	}
	virtual ECHOSTATUS SetFlags(WORD wFlags);
	virtual ECHOSTATUS ClearFlags(WORD	wFlags);

	//
	// Get/set the locked sample rate - if the sample rate is locked,
	// the card will be fixed at the locked rate.  Only applies if 
	// the card is set to internal clock.
	//
	virtual ECHOSTATUS GetAudioLockedSampleRate
	(
		DWORD		&dwSampleRate
	);

	virtual ECHOSTATUS SetAudioLockedSampleRate
	(
		DWORD		dwSampleRate
	);
	
	//
	//	Enable/disable audio metering.
	//
	ECHOSTATUS GetMetersOn( BOOL & bOn )
		{ return( ( NULL == GetDspCommObject() )
						? ECHOSTATUS_DSP_DEAD
						: GetDspCommObject()->GetMetersOn( bOn ) ); }

	ECHOSTATUS SetMetersOn( BOOL bOn )
		{ return( ( NULL == GetDspCommObject() )
						? ECHOSTATUS_DSP_DEAD
						: GetDspCommObject()->SetMetersOn( bOn ) ); }
	//
	//	Get/Set Professional or consumer S/PDIF status
	//
	virtual BOOL IsProfessionalSpdif()
		{ return( ( NULL == GetDspCommObject() )
						? FALSE
						: GetDspCommObject()->IsProfessionalSpdif() ); }
	virtual void SetProfessionalSpdif( BOOL bNewStatus );
	
	//
	// Get/Set S/PDIF out non-audio status bit
	//
	virtual BOOL IsSpdifOutNonAudio()
		{ return( ( NULL == GetDspCommObject() )
						? FALSE
						: GetDspCommObject()->IsSpdifOutNonAudio() ); }
	virtual void SetSpdifOutNonAudio(BOOL bNonAudio )
	{
		if (GetDspCommObject())
			GetDspCommObject()->SetSpdifOutNonAudio(bNonAudio);
	}
	
	

	//
	//	Set digital mode
	//
	virtual ECHOSTATUS SetDigitalMode( BYTE byDigitalMode );

	//
	// Get and set input and output clocks
	//
	virtual ECHOSTATUS SetInputClock(WORD	wClock);
	virtual ECHOSTATUS GetInputClock(WORD	&wClock);
	virtual ECHOSTATUS SetOutputClock(WORD wClock);
	virtual ECHOSTATUS GetOutputClock(WORD &wClock);
	
	//
	//	Audio Line Levels Functions:
	//	Nominal level is either -10dBV or +4dBu.
	//	Output level data is scaled by 256 and ranges from -128dB to
	//	+6dB (0xffff8000 to 0x600).
	//	Input level data is scaled by 256 and ranges from -25dB to +25dB
	//	( 0xffffe700 to 0x1900 ).
	//
	virtual ECHOSTATUS GetAudioLineLevel
	(
		PMIXER_FUNCTION	pMF
	);

	virtual ECHOSTATUS SetAudioLineLevel
	(
		PMIXER_FUNCTION	pMF
	);

	virtual ECHOSTATUS GetAudioNominal
	(
		PMIXER_FUNCTION	pMF
	);

	virtual ECHOSTATUS SetAudioNominal
	(
		PMIXER_FUNCTION	pMF	
	);

	//
	//	Audio mute controls
	//
	virtual ECHOSTATUS SetAudioMute
	(
		PMIXER_FUNCTION	pMF
	);

	virtual ECHOSTATUS GetAudioMute
	(
		PMIXER_FUNCTION	pMF
	);

	//
	//	Audio Monitors Functions:
	//
	virtual ECHOSTATUS SetAudioMonitor
	(
		WORD	wBusIn,
		WORD	wBusOut,
		INT32	iGain						// New gain
	);

	virtual ECHOSTATUS GetAudioMonitor
	(
		WORD	wBusIn,
		WORD	wBusOut,
		INT32 &	iGain						// Returns current gain
	);

	virtual ECHOSTATUS SetAudioMonitorPan
	(
		WORD	wBusIn,
		WORD	wBusOut,
		INT32	iPan						// New pan (0 - MAX_MIXER_PAN)
	);

	virtual ECHOSTATUS GetAudioMonitorPan
	(
		WORD	wBusIn,
		WORD	wBusOut,
		INT32 &	iPan						// Returns current pan (0 - MAX_MIXER_PAN)
	);

	virtual ECHOSTATUS SetAudioMonitorMute
	(
		WORD	wBusIn,
		WORD	wBusOut,
		BOOL	bMute						// New state
	);

	virtual ECHOSTATUS GetAudioMonitorMute
	(
		WORD	wBusIn,
		WORD	wBusOut,
		BOOL &bMute						// Returns current state
	);
	
	
	//
	// Get and set audio pan
	//
	virtual ECHOSTATUS GetAudioPan(PMIXER_FUNCTION pMixerFunction);
	virtual ECHOSTATUS SetAudioPan(PMIXER_FUNCTION pMixerFunction);

	//
	// Get a bitmask of all the clocks the hardware is currently detecting
	//
	virtual ECHOSTATUS GetInputClockDetect(DWORD &dwClockDetectBits);
	

#ifdef PHANTOM_POWER_CONTROL
	//
	// Phantom power on/off for Gina3G
	//
	virtual void GetPhantomPower(BOOL *pfPhantom) { *pfPhantom = 0; }
	virtual void SetPhantomPower(BOOL fPhantom) {}
#endif
	
	
	//***********************************************************************
	//
	//	Audio transport (public) - playing and recording audio
	//
	//***********************************************************************	

	//
	// Set the scatter-gather list for a pipe
	//	
	ECHOSTATUS SetDaffyDuck( WORD wPipeIndex, CDaffyDuck *pDuck);
	
	//
	//	Reserve a pipe.  Please refer to the definition of 
	// ECHOGALS_OPENAUDIOPARAMETERS.
	//
	// If the fCheckHardware flag is true, then the open will fail
	// if the DSP and ASIC firmware have not been loaded (usually means
	// your external box is turned off).
	//
	// If you want to manage your own CDaffyDuck object for this pipe, 
	// then pass a pointer to a CDaffyDuck object.  If you pass NULL,
	// one will be created for you.
	//
	virtual ECHOSTATUS OpenAudio
	(
		PECHOGALS_OPENAUDIOPARAMETERS	pOpenParameters,	// Info on channel
		PWORD									pwPipeIndex,		// Ptr to pipe index
		BOOL									fCheckHardware = TRUE,
		CDaffyDuck							*pDuck = NULL
	);

	//
	//	Close a pipe
	//
	virtual ECHOSTATUS CloseAudio
	(
		PECHOGALS_CLOSEAUDIOPARAMETERS	pCloseParameters,
		BOOL										fFreeDuck = TRUE
	);

	//
	//	Find out if the audio format is supported.
	//
	virtual ECHOSTATUS QueryAudioFormat
	(
		WORD							wPipeIndex,
		PECHOGALS_AUDIOFORMAT	pAudioFormat
	);

	//
	// Set the audio format for a single pipe
	virtual ECHOSTATUS SetAudioFormat
	(
		WORD							wPipeIndex,
		PECHOGALS_AUDIOFORMAT	pAudioFormat
	);

	//
	// Set the audio format for a bunch of pipes at once
	//
	virtual ECHOSTATUS SetAudioFormat
	(
		PCChannelMask				pChannelMask,
		PECHOGALS_AUDIOFORMAT	pAudioFormat
	);

	//
	//	Get the current audio format for a pipe
	//     
	virtual ECHOSTATUS GetAudioFormat
	(
		WORD							wPipeIndex,
		PECHOGALS_AUDIOFORMAT	pAudioFormat
	);

	//
	// Call this to find out if this card can handle a given sample rate
	//
	virtual ECHOSTATUS QueryAudioSampleRate
	(
		DWORD		dwSampleRate
	) = 0;
	
	virtual void QuerySampleRateRange(DWORD &dwMinRate,DWORD &dwMaxRate) = 0;

	//
	// I'm not going to tell you what the next two functions do; you'll just
	// have to guess.
	//
	virtual ECHOSTATUS SetAudioSampleRate
	(
		DWORD		dwSampleRate
	);
    
	virtual ECHOSTATUS GetAudioSampleRate
	(
		PDWORD	pdwSampleRate
	);


	//
	//	Start transport for several pipes at once
	//
	virtual ECHOSTATUS Start
	(
		PCChannelMask	pChannelMask
	);
	
	//
	//	Start transport for a single audio pipe
	//
	virtual ECHOSTATUS Start
	(
		WORD		wPipeIndex
	);

	//
	//	Stop transport for several pipes at once
	//
	virtual ECHOSTATUS Stop
	(
		PCChannelMask	pChannelMask
	);

	//
	//	Stop transport for a single pipe
	//
	virtual ECHOSTATUS Stop
	(
		WORD		wPipeIndex
	);

	//
	//	Stop and reset selected output and/or input audio pipes
	//
	virtual ECHOSTATUS Reset
	(
		PCChannelMask	pChannelMask
	);

	//
	//	Stop and reset single audio pipe
	//
	virtual ECHOSTATUS Reset
	(
		WORD		wPipeIndex
	);

	//
	// Get mask with active pipes - that is, those pipes that
	// are currently moving data.
	//
	virtual ECHOSTATUS GetActivePipes
	(
		PCChannelMask	pChannelMask
	);

	//
	// Get mask with open pipes
	//
	virtual ECHOSTATUS GetOpenPipes
	(
		PCChannelMask	pChannelMask
	);

	//
	//	Get a pointer that can be dereferenced to read 
	// the DSP's DMA counter for this pipe; see CEchoGals_transport.cpp 
	// for more info.
	//
	virtual ECHOSTATUS GetAudioPositionPtr
	(
		WORD		wPipeIndex,
		PDWORD 	(&pdwPosition)
	);

	//
	// Get the daffy duck pointer for a pipe
	//
	CDaffyDuck *GetDaffyDuck(WORD wPipeIndex);
	
	//
	// Reset the 64 bit DMA position for this pipe
	//
	void ResetDmaPos( WORD wPipe );
	
	//
	// Update the 64 bit DMA position for this pipe
	//
	void UpdateDmaPos( WORD wPipeIndex );
	
	//
	//	Get the 64 bit DMA position for this pipe
	// 
	void GetDmaPos( WORD wPipeIndex, PULONGLONG pPos )
	{
		UpdateDmaPos( wPipeIndex );
		*pPos = m_ullDmaPos[ wPipeIndex ];
	}
	
	//
	// Get the state of a given pipe
	//
	DWORD GetPipeState( WORD wPipeIndex )
	{
		return (DWORD) m_byPipeState[ wPipeIndex ];
	}
	
	//
	//	Verify a pipe is open and ready for business!
	//
	ECHOSTATUS VerifyAudioOpen
	(
		WORD		wPipeIndex
	);
	
	//***********************************************************************
	//
	//	MIDI (public)
	//
	//***********************************************************************	

#ifdef MIDI_SUPPORT
	
	// The context struct should be set to zero before calling OpenMidiInput
	virtual ECHOSTATUS OpenMidiInput(ECHOGALS_MIDI_IN_CONTEXT *pContext);
	virtual ECHOSTATUS CloseMidiInput(ECHOGALS_MIDI_IN_CONTEXT *pContext);

	//
	// Reset the MIDI input buffer; MIDI input remains enabled and open
	//
	virtual ECHOSTATUS ResetMidiInput(ECHOGALS_MIDI_IN_CONTEXT *pContext);

	virtual ECHOSTATUS WriteMidi
	(
		DWORD		dwExpectedCt,
		PBYTE		pBuffer,
		PDWORD	pdwActualCt
	);
	
	virtual ECHOSTATUS ReadMidiByte(	ECHOGALS_MIDI_IN_CONTEXT	*pContext,
												DWORD								&dwMidiData,
												LONGLONG							&llTimestamp);
	
	virtual void ServiceMtcSync()
	{
	}

#endif // MIDI_SUPPORT



	//***********************************************************************
	//
	//	Interrupt handler methods (public)
	//
	//***********************************************************************	
	
	//
	//	This is called from within an interrupt handler.  It starts interrupt
	//	handling.  Returns error status if this interrupt is not ours.
	// Returns ECHOSTATUS_OK if it is.
	//
	//	OS dependent code handles routing to this point.
	//
	virtual ECHOSTATUS ServiceIrq(BOOL &fMidiReceived);


	//***********************************************************************
	//
	//	Power management (public)
	//
	// Please do not do silly things like try and set a volume level or 
	// play audio after calling GoComatose; be sure and call WakeUp first!
	//
	//***********************************************************************	

	//
	// Tell the hardware to go to sleep
	//
	virtual ECHOSTATUS GoComatose();
	
	//
	// Tell the hardware to wake up
	//
	virtual ECHOSTATUS WakeUp();



protected:

	//***********************************************************************
	//
	//	Member variables (protected)
	//
	//***********************************************************************	

	PCOsSupport		m_pOsSupport;		// Ptr to OS Support methods & data
	WORD				m_wFlags;			// See ECHOGALS_FLAG_??? flags defined
												// in EchoGalsXface.h
	PVOID				m_pvSharedMemory;	// Shared memory addr assigned by PNP
	CHAR				m_szCardInstallName[ ECHO_MAXNAMELEN ];
												// Same as card except when multiple
												// boards of the same type installed.
												// Then becomes "Layla1", "Layla2" etc.
	CChannelMask	m_cmAudioOpen;		// Audio channels open mask
	CChannelMask	m_cmAudioCyclic;	// Audio use cyclic buffers mask

	DWORD				m_dwSampleRate;	// Card sample rate in Hz
	DWORD				m_dwLockedSampleRate;
												// Card sample rate when locked
	ECHOGALS_AUDIO_PIPE
						m_Pipes[ ECHO_MAXAUDIOPIPES ];
												// Keep mapping info on open pipes
	BYTE				m_byPipeState[ ECHO_MAXAUDIOPIPES ];
												// Track state of all pipes
	PVOID				m_ProcessId[ ECHO_MAXAUDIOPIPES ];
												// Caller process ID for implementing
												// synchronous wave start
	DWORD				m_dwMeterOnCount;	// Track need for meter updates by DSP
	PCDspCommObject
						m_pDspCommObject;	// Ptr to DSP communication object

	DWORD				m_dwMidiInOpen;	// Midi in channels open mask
	DWORD				m_dwMidiOutOpen; 	// Midi out channels open mask
	
	BOOL				m_fMixerDisabled;
	
	WORD				m_wAnalogOutputLatency;	// Latency in samples
	WORD				m_wAnalogInputLatency;
	WORD				m_wDigitalOutputLatency;
	WORD				m_wDigitalInputLatency;
		
														
	
	//
	//	Monitor state info
	//
	CMonitorCtrl		m_MonitorCtrl;
		
	//
	// Output pipe control
	//
	CPipeOutCtrl		m_PipeOutCtrl;
						
	//
	// Input bus mute and gain settings
	//
	CBusInLineLevel	m_BusInLineLevels[ ECHO_MAXAUDIOINPUTS ];
	
	//
	// Output bus mute and gain settings
	//
	CBusOutLineLevel	m_BusOutLineLevels[ ECHO_MAXAUDIOOUTPUTS ];
	
	//
	//
	// Linked list of mixer clients
	//
	ECHO_MIXER_CLIENT	*m_pMixerClients;


	//
	// DMA position, in bytes, for each pipe
	//
	ULONGLONG		m_ullDmaPos[ ECHO_MAXAUDIOPIPES ];
	DWORD				m_dwLastDspPos[ ECHO_MAXAUDIOPIPES ];
	
	//
	//	Pointers to daffy ducks for each pipe
	// 
	PCDaffyDuck		m_DaffyDucks[ ECHO_MAXAUDIOPIPES ];


#ifdef MIDI_SUPPORT	
	//
	// MIDI input buffer
	//
	CMidiInQ	m_MidiIn;
#endif

	
	//
	//	Macintosh compiler likes "class" after friend, PC doesn't care
	//
	friend class CLineLevel;
	friend class CBusOutLineLevel;
	friend class CBusInLineLevel;
	friend class CPipeOutCtrl;
	friend class CMonitorCtrl;
	friend class CMidiInQ;
	friend class CMtcSync;


	//***********************************************************************
	//
	//	Initialization (protected)
	//
	//***********************************************************************	

	//
	//	Init the line level classes.
	//	This MUST be called from within any derived classes as part of
	//	InitHw after the DSP is up and running!
	//
	virtual ECHOSTATUS InitLineLevels();


	//***********************************************************************
	//
	//	Information (protected)
	//
	//***********************************************************************	

	virtual ECHOSTATUS GetBaseCapabilities(PECHOGALS_CAPS pCapabilities);

	WORD GetNumPipesOut();
	WORD GetNumPipesIn();
	WORD GetNumBussesOut();
	WORD GetNumBussesIn();
	WORD GetNumBusses();
 	WORD GetNumPipes();
	WORD GetFirstDigitalBusOut();
	WORD GetFirstDigitalBusIn();

	BOOL HasVmixer();

	PCDspCommObject GetDspCommObject();
	

	//***********************************************************************
	//
	//	Audio transport (protected)
	//
	// Daffy ducks are objects that manage scatter-gather lists.
	//
	//***********************************************************************	



	//***********************************************************************
	//
	//	Mixer interface (protected)
	//
	//***********************************************************************	

	//
	// Match a mixer cookie to a mixer client
	//
	ECHO_MIXER_CLIENT *GetMixerClient(NUINT Cookie);
	
	// 
	// Adjust all the monitor levels for a particular output bus; this is
	// used to implement the master output level.
	//
	virtual ECHOSTATUS AdjustMonitorsForBusOut(WORD wBusOut);
	
	//
	// Adjust all the output pipe levels for a particular output bus; this is
	// also used to implement the master output level.
	//
	virtual ECHOSTATUS AdjustPipesOutForBusOut(WORD wBusOut,INT32 iBusOutGain);
	
	//
	// A mixer control has changed; store the notify
	//
	ECHOSTATUS MixerControlChanged
	(
		WORD	wType,								// One of the ECHO_CHANNEL_TYPES
		WORD  wParameter, 						// One of the MXN_* values
		WORD 	wCh1 = ECHO_CHANNEL_UNUSED,	// Depends on the wType value
		WORD  wCh2 = ECHO_CHANNEL_UNUSED		// Also depends on wType value
	);

#ifdef DIGITAL_INPUT_AUTO_MUTE_SUPPORT    
	//
	// Digital input auto-mute - Gina24, Layla24, and Mona only
	//
	virtual ECHOSTATUS GetDigitalInAutoMute(PMIXER_FUNCTION pMixerFunction);
	virtual ECHOSTATUS SetDigitalInAutoMute(PMIXER_FUNCTION pMixerFunction);
#endif

	//
	// Get the audio latency for a single pipe
	//
	virtual void GetAudioLatency(ECHO_AUDIO_LATENCY *pLatency);

};		// class CEchoGals

typedef CEchoGals * PCEchoGals;
													
#endif

// *** CEchoGals.h ***
