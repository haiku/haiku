// ****************************************************************************
//
//		MixerXface.H
//
//		Include file for mixer interfacing with the EchoGals-derived classes.
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
#ifndef _MIXERXFACE_
#define _MIXERXFACE_

#include "EchoGalsXface.h"

//
// Gain ranges
//
#define ECHOGAIN_MUTED			DSP_TO_GENERIC(-128)	// Minimum possible gain
#define ECHOGAIN_MINOUT			DSP_TO_GENERIC(-128)	// Min output gain in dB
#define ECHOGAIN_MAXOUT			DSP_TO_GENERIC(6)		// Max output gain in dB
#define ECHOGAIN_MININP			DSP_TO_GENERIC(-25) 	// Min input gain in dB
#define ECHOGAIN_MAXINP			DSP_TO_GENERIC(25)	// Max input gain in dB

#define ECHOGAIN_UPDATE		 	0xAAAAAA					// Using this value means:
																	// Re-set the gain
																	// to the DSP using the
																	// currently stored value.


//=============================================================================
//
// Most of the mixer functions have been unified into a single interface;
// you pass either a single MIXER_FUNCTION struct or an array of MIXER_FUNCTION
// structs.  Each MIXER_FUNCTION is generally used to set or get one or more
// values.
//
//=============================================================================

//
//	Structure to specify a bus, pipe, or a monitor being routed from 
// the input to the output
//
enum ECHO_CHANNEL_TYPES
{
	ECHO_BUS_OUT = 0,
	ECHO_BUS_IN,
	ECHO_PIPE_OUT,
	ECHO_PIPE_IN,
	ECHO_MONITOR,
	ECHO_NO_CHANNEL_TYPE = 0xffff,
	ECHO_CHANNEL_UNUSED = 0xffff
};

typedef struct tMIXER_AUDIO_CHANNEL
{
	WORD			wCardId;				// This field is obsolete
	WORD			wChannel;			// Depends on dwType:
											// ECHO_BUS_OUT	wChannel = output bus #
											// ECHO_BUS_IN		wChannel = input bus #
											// ECHO_PIPE_OUT	wChannel = output pipe #
											// ECHO_PIPE_IN	wChannel = input pipe #
											// ECHO_MONITOR	wChannel = input bus #
	DWORD			dwType;				// One of the above enums
} MIXER_AUDIO_CHANNEL, *PMIXER_AUDIO_CHANNEL;


//
//	Mixer Function Tags
//
// These codes are used to specify the mixer function you want to perform;
// they determine which field in the Data union to use
//
#define	MXF_GET_CAPS					1		// Get card capabilities
#define	MXF_GET_LEVEL					2		// Get level for one channel
#define	MXF_SET_LEVEL					3		// Set level for one channel
#define	MXF_GET_NOMINAL				4		// Get nominal level for one channel
#define	MXF_SET_NOMINAL				5		// Set nominal level for one channel
#define	MXF_GET_MONITOR				6		// Get monitor for one channel
#define	MXF_SET_MONITOR				7		// Set monitor for one channel
#define	MXF_GET_INPUT_CLOCK			8		// Get input clock
#define	MXF_SET_INPUT_CLOCK			9		// Set input clock for one card
#define	MXF_GET_METERS					10		// Get meters for all channels on one card
#define	MXF_GET_METERS_ON				11		// Get meters on state for one card
#define	MXF_SET_METERS_ON				12		// Set meters on state for one card
														//	Meters must only be enabled while 
														// driver for card exists; the meters are
														// written via bus mastering directly to memory
#define	MXF_GET_PROF_SPDIF			13		// Get Professional or consumer S/PDIF mode
														// for one card
#define	MXF_SET_PROF_SPDIF			14		// Set Professional or consumer S/PDIF mode
														// for one card
#define	MXF_GET_MUTE					15		// Get mute state for one channel
#define	MXF_SET_MUTE					16		// Set mute state for one channel
#define	MXF_GET_MONITOR_MUTE			19		// Get monitor mute state for one channel
#define	MXF_SET_MONITOR_MUTE			20		// Set monitor mute state for one channel
#define	MXF_GET_MONITOR_PAN			23		// Get monitor pan value for one stereo channel
#define	MXF_SET_MONITOR_PAN			24		// Set monitor pan value for one stereo channel
#define	MXF_GET_FLAGS					27		// Get driver flags
#define	MXF_SET_FLAGS					28		// Set driver flag
#define	MXF_CLEAR_FLAGS				29		// Clear driver flags
#define	MXF_GET_SAMPLERATE_LOCK		30		// Get locked sample rate for one card
#define	MXF_SET_SAMPLERATE_LOCK		31		// Set locked sample rate for one card
#define	MXF_GET_SAMPLERATE			32		// Get actual sample rate for one card
#define	MXF_GET_MIDI_IN_ACTIVITY	35		// Get MIDI in activity state
#define	MXF_GET_MIDI_OUT_ACTIVITY	36		// Get MIDI out activity state
#define	MXF_GET_DIGITAL_MODE			37		// Get digital mode
#define	MXF_SET_DIGITAL_MODE			38		// Get digital mode

#define 	MXF_GET_PAN						39		// Get & set pan
#define 	MXF_SET_PAN						40
#define	MXF_GET_OUTPUT_CLOCK			41		// Get output clock
#define	MXF_SET_OUTPUT_CLOCK			42		// Set output clock for one card
#define 	MXF_GET_CLOCK_DETECT			43		// Get the currently detected clocks
#define 	MXF_GET_DIG_IN_AUTO_MUTE	44		// Get the state of the digital input auto-mute
#define 	MXF_SET_DIG_IN_AUTO_MUTE	45		// Set the state of the digital input auto-mute
#define	MXF_GET_AUDIO_LATENCY		46		// Get the latency for a single pipe

#define	MXF_GET_PHANTOM_POWER		47		// Get phantom power state
#define  MXF_SET_PHANTOM_POWER		48		// Set phantom power state


//
// Output pipe control change - only used if you specify 
// ECHO_PIPE_OUT in the dwType field for MIXER_AUDIO_CHANNEL
//
typedef struct
{
	WORD	wBusOut;	  				// For cards without vmixer, should
						  				// be the same as wChannel in MIXER_AUDIO_CHANNEL
	union
	{
		INT32	iLevel;				// New gain in dB X 256
		INT32	iPan;					// 0 <= new pan <= MAX_MIXER_PAN,
										// 0 = full left MAX_MIXER_PAN = full right
		BOOL	bMuteOn;				// To mute or not to mute
										// MXF_GET_MONITOR_MUTE &
										// MXF_SET_MONITOR_MUTE

	} Data;
}	MIXER_PIPE_OUT, PMIXER_PIPE_OUT;


//
//	The MIXER_AUDIO_CHANNEL header has the card and input channel.
//	This structure has the output channel and the gain, mute or pan
//	state for one monitor.
// 
// Only used if you specify ECHO_MONITOR in the dwType field 
// for MIXER_AUDIO_CHANNEL.
//
typedef struct tMIXER_MONITOR
{
	WORD		wBusOut;
	union
	{
		INT32	iLevel;				// New gain in dB X 256
		INT32	iPan;					// 0 <= new pan <= MAX_MIXER_PAN,
										// 0 = full left MAX_MIXER_PAN = full right
		BOOL	bMuteOn;				// To mute or not to mute
										// MXF_GET_MONITOR_MUTE &
										// MXF_SET_MONITOR_MUTE
	} Data;
} MIXER_MONITOR, *PMIXER_MONITOR;


//
// If you specify MXF_GET_AUDIO_LATENCY, then you get the following
// structure back
//
typedef struct tECHO_AUDIO_LATENCY
{
	WORD		wPipe;
	WORD		wIsInput;
	DWORD		dwLatency;
}	ECHO_AUDIO_LATENCY;



//
//	Mixer Function Data Structure
//
typedef struct tMIXER_FUNCTION
{
	MIXER_AUDIO_CHANNEL	Channel;				// Which channel to service
	INT32						iFunction;			// What function to do
	ECHOSTATUS				RtnStatus;			// Return Result
	union
	{
		ECHOGALS_CAPS			Capabilities;	// MXF_GET_CAPS
		INT32						iNominal;		// MXF_GET_NOMINAL & MXF_SET_NOMINAL
		INT32						iLevel;			// MXF_GET_LEVEL & MXF_SET_LEVEL
		MIXER_MONITOR			Monitor;			// MXF_GET_MONITOR & MXF_SET_MONITOR
														// MXF_GET_MONITOR_MUTE & MXF_SET_MONITOR_MUTE
														// MXF_GET_MONITOR_PAN & MXF_SET_MONITOR_PAN
		WORD						wClock;			// MXF_GET_INPUT_CLOCK & MXF_SET_INPUT_CLOCK
														// MXF_GET_OUTPUT_CLOCK & MXF_SET_OUTPUT_CLOCK
		DWORD						dwClockDetectBits;
														// MXF_GET_CLOCK_DETECTs														
		ECHOGALS_METERS		Meters;			// MXF_GET_METERS
		BOOL						bMetersOn;		// MXF_GET_METERS_ON & 
														//	MXF_SET_METERS_ON
		BOOL						bProfSpdif;		// MXF_GET_PROF_SPDIF & 
														//	MXF_SET_PROF_SPDIF
		BOOL						bMuteOn;			// MXF_GET_MUTE & MXF_SET_MUTE
		BOOL						bNotifyOn;		// MXF_GET_NOTIFY_ON & 
														//	MXF_SET_NOTIFY_ON
		WORD						wFlags;			// MXF_GET_FLAGS, MXF_SET_FLAGS & 
														//	MXF_CLEAR_FLAGS (See
														// ECHOGALS_FLAG_??? in file
														//	EchoGalsXface.h)
		DWORD						dwLockedSampleRate;
														// MXF_GET_SAMPLERATE_LOCK &
														//	MXF_SET_SAMPLERATE_LOCK
		DWORD						dwSampleRate;	// MXF_GET_SAMPLERATE
		BOOL						bMidiActive;	// MXF_GET_MIDI_IN_ACTIVITY &
														//	MXF_GET_MIDI_OUT_ACTIVITY
		INT32						iDigMode;		// MXF_GET_DIGITAL_MODE & 
														//	MXF_SET_DIGITAL_MODE
														
		MIXER_PIPE_OUT			PipeOut;			// MXF_GET_LEVEL & MXF_SET_LEVEL
														// MXF_GET_MUTE & MXF_SET_MUTE
														// MXF_GET_PAN & MXF_SET_PAN

		BOOL						fDigitalInAutoMute;	// MXF_GET_DIG_IN_AUTO_MUTE
																// MXF_SET_DIG_IN_AUTO_MUTE
		ECHO_AUDIO_LATENCY	Latency;			// MXF_GET_AUDIO_LATENCY
		
		BOOL						fPhantomPower;	// Phantom power state (true == on, false == off)
	} Data;
} MIXER_FUNCTION, *PMIXER_FUNCTION;

//
//	Mixer Multifunction Interface
//
//	Allow user to supply an array of commands to be performed in one call.
//	Since this is a variable length structure, user beware!
//
typedef struct tMIXER_MULTI_FUNCTION
{
	INT32				iCount;
	MIXER_FUNCTION	MixerFunction[ 1 ];
} MIXER_MULTI_FUNCTION, *PMIXER_MULTI_FUNCTION;

//
//	Use this macro to size the data structure
//
#define	ComputeMixerMultiFunctionSize(Ct)	( sizeof( MIXER_MULTI_FUNCTION ) + ( sizeof( MIXER_FUNCTION ) * ( Ct - 1 ) ) )


//
//	Notification
//
//	Mixers allow for notification whenever a change occurs.
//	Mixer notify structure contains channel and parameter(s) that
//	changed.
//

//
//	Mixer Parameter Changed definitions
//
#define	MXN_LEVEL				0	// Level changed
#define	MXN_NOMINAL				1	// Nominal level changed
#define	MXN_INPUT_CLOCK		2	// Input clock changed
#define	MXN_SPDIF				3	// S/PDIF - Professional mode changed
#define	MXN_MUTE					4	// Mute state changed
#define 	MXN_PAN					6  // Pan value changed

#define	MXN_FLAGS				12	// A driver flag changed

#define	MXN_DIGITAL_MODE		14	// Digital mode changed
#define 	MXN_OUTPUT_CLOCK		15 // Output clock changed
#define	MXN_MAX					15	// Max notify parameters

typedef struct tMIXER_NOTIFY
{
	WORD			wType;			// Same as enums used for MIXER_AUDIO_CHANNEL

	union
	{
		WORD		wBusIn;
		WORD		wPipeIn;
		WORD		wPipeOut;
	} u;

	WORD			wBusOut;			// For monitor & output pipe notifies only
	WORD			wParameter;		// One of the above MXN_*
	
} MIXER_NOTIFY, *PMIXER_NOTIFY;


typedef struct tMIXER_MULTI_NOTIFY
{
	DWORD				dwUnused;
	DWORD				dwCount;			// When passed to the generic driver,
											// dwCount holds the size of the Notifies array.
											// On returning from the driver, dwCount
											// holds the number of entries in Notifies
											// filled out by the generic driver.
	MIXER_NOTIFY	Notifies[1];	// Dynamic array; there are dwCount entries
}	MIXER_MULTI_NOTIFY, *PMIXER_MULTI_NOTIFY;

//
//	Max pan value
//
#define	MAX_MIXER_PAN		1000		// this is pan hard right


//=============================================================================
//
// After designing eighteen or nineteen consoles for this hardware, we've 
// learned that it's useful to be able to get all the following stuff at
// once.  Typically the console will run a timer that fetchs this periodically.
//
// Meters and dwClockDetectBits are exactly the same as you would get if you
// did each of those mixer functions separately.
//
// dwNumPendingNotifies is how many notifies are in the queue associated with
// the client.  You can use this number to create an array of MIXER_NOTIFY
// structures and call CEchoGals::GetControlChanges.  This way you only check
// for control changes if the controls have actually changed.
//
//=============================================================================

typedef struct tECHO_POLLED_STUFF
{
	DWORD					dwUnused;
	ECHOGALS_METERS	Meters;
	DWORD					dwClockDetectBits;
	DWORD					dwNumPendingNotifies;
} ECHO_POLLED_STUFF;

#endif

// MixerXface.h
